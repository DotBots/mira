/**
 * @file
 * @ingroup     mira
 *
 * @brief       Implementation of the mira protocol
 *
 * @author Geovane Fedrecheski <geovane.fedrecheski@inria.fr>
 *
 * @copyright Inria, 2025
 */

#include <nrf.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "mr_device.h"
#include "models.h"
#include "packet.h"
#include "mac.h"
#include "scheduler.h"
#include "association.h"
#include "queue.h"
#include "bloom.h"
#include "mira.h"

//=========================== defines ==========================================

typedef struct {
    mr_node_type_t node_type;
    mr_event_cb_t  app_event_callback;
} mira_vars_t;

//=========================== variables ========================================

static mira_vars_t _mira_vars = { 0 };

//=========================== prototypes =======================================

static void event_callback(mr_event_t event, mr_event_data_t event_data);

//=========================== public ===========================================
// in this library, user-facing functions begin with mira_*, while internal functions begin with mr_*

// -------- common --------

void mira_init(mr_node_type_t node_type, uint16_t net_id, schedule_t *app_schedule, mr_event_cb_t app_event_callback) {
    _mira_vars.node_type          = node_type;
    _mira_vars.app_event_callback = app_event_callback;

    mr_assoc_init(net_id, event_callback);
    mr_scheduler_init(app_schedule);
    mr_mac_init(event_callback);
    if (node_type == MIRA_GATEWAY) {
        mr_bloom_gateway_init();
    }
}

void mira_tx(uint8_t *packet, uint8_t length) {
    mr_queue_add(packet, length);
}

mr_node_type_t mira_get_node_type(void) {
    return _mira_vars.node_type;
}

void mira_set_node_type(mr_node_type_t node_type) {
    _mira_vars.node_type = node_type;
}

// -------- gateway ----------

size_t mira_gateway_get_nodes(uint64_t *nodes) {
    return mr_scheduler_gateway_get_nodes(nodes);
}

size_t mira_gateway_count_nodes(void) {
    return mr_scheduler_gateway_get_nodes_count();
}

// -------- node ----------

void mira_node_tx_payload(uint8_t *payload, uint8_t payload_len) {
    uint8_t packet[MIRA_PACKET_MAX_SIZE] = { 0 };
    uint8_t len                          = mr_build_packet_data(packet, mira_node_gateway_id(), payload, payload_len);
    mr_queue_add(packet, len);
}

bool mira_node_is_connected(void) {
    return mr_assoc_is_joined();
}

uint64_t mira_node_gateway_id(void) {
    return mr_mac_get_synced_gateway();
}

//=========================== iternal api =====================================

void mr_handle_packet(uint8_t *packet, uint8_t length) {
    mr_packet_header_t *header = (mr_packet_header_t *)packet;

    if (header->dst != mr_device_id() && header->dst != MIRA_BROADCAST_ADDRESS && header->type != MIRA_PACKET_BEACON) {
        // ignore packets that are not for me, and not broadcast, and not a beacon
        return;
    }

    if (mira_get_node_type() == MIRA_GATEWAY) {
        if (header->network_id != mr_assoc_get_network_id()) {
            // ignore packets from other networks
            return;
        }

        bool from_joined_node = mr_assoc_gateway_node_is_joined(header->src);

        switch (header->type) {
            case MIRA_PACKET_JOIN_REQUEST:
            {
                if (from_joined_node) {
                    // already joined, ignore
                    return;
                }
                // try to assign a cell to the node
                // the asn-based keep-alive is also initialized, so no need to call mr_assoc_gateway_keep_node_alive
                int16_t cell_id = mr_scheduler_gateway_assign_next_available_uplink_cell(header->src, mr_mac_get_asn());
                if (cell_id >= 0) {
                    // at the packet level, max_nodes is limited to 256 (using uint8_t cell_id)
                    mr_queue_set_join_response(header->src, (uint8_t)cell_id);
                    mr_bloom_gateway_set_dirty();
                    _mira_vars.app_event_callback(MIRA_NODE_JOINED, (mr_event_data_t){ .data.node_info.node_id = header->src });
                } else {
                    _mira_vars.app_event_callback(MIRA_ERROR, (mr_event_data_t){ .tag = MIRA_GATEWAY_FULL });
                }
                break;
            }
            case MIRA_PACKET_DATA:
            {
                if (!from_joined_node) {
                    // ignore packets from nodes that are not joined
                    return;
                }
                // send the packet to the application
                mr_event_data_t event_data = {
                    .data.new_packet = {
                        .len         = length,
                        .header      = header,
                        .payload     = packet + sizeof(mr_packet_header_t),
                        .payload_len = length - sizeof(mr_packet_header_t) }
                };
                _mira_vars.app_event_callback(MIRA_NEW_PACKET, event_data);
                mr_assoc_gateway_keep_node_alive(header->src, mr_mac_get_asn());  // keep track of when the last packet was received
                break;
            }
            case MIRA_PACKET_KEEPALIVE:
            {
                if (!from_joined_node) {
                    // ignore packets from nodes that are not joined
                    return;
                }
                mr_assoc_gateway_keep_node_alive(header->src, mr_mac_get_asn());  // keep track of when the last packet was received
                mr_event_data_t event_data = {
                    .data.node_info = { .node_id = header->src }
                };
                _mira_vars.app_event_callback(MIRA_KEEPALIVE, event_data);
                break;
            }
            default:
                break;
        }

    } else if (mira_get_node_type() == MIRA_NODE) {
        if (!mr_assoc_node_matches_network_id(header->network_id)) {
            // ignore packet with non-matching network id
            return;
        }

        bool from_my_joined_gateway = header->src == mr_mac_get_synced_gateway() && mr_assoc_get_state() == JOIN_STATE_JOINED;

        switch (header->type) {
            case MIRA_PACKET_BEACON:
                mr_assoc_handle_beacon(packet, length, MIRA_FIXED_SCAN_CHANNEL, mr_mac_get_asn());
                break;
            case MIRA_PACKET_JOIN_RESPONSE:
            {
                if (mr_assoc_get_state() != JOIN_STATE_JOINING) {
                    // ignore if not in the JOINING state
                    return;
                }
                if (header->dst != mr_device_id()) {
                    // ignore if not for me
                    return;
                }
                // the first byte after the header contains the cell_id
                uint8_t cell_id = packet[sizeof(mr_packet_header_t)];
                if (mr_scheduler_node_assign_myself_to_cell(cell_id)) {
                    mr_assoc_node_handle_joined(header->src);
                } else {
                    _mira_vars.app_event_callback(MIRA_ERROR, (mr_event_data_t){ 0 });
                }
                break;
            }
            case MIRA_PACKET_DATA:
            {
                if (!from_my_joined_gateway) {
                    // ignore data packets from other gateways
                    return;
                }
                // send the packet to the application
                mr_event_data_t event_data = {
                    .data.new_packet = {
                        .len         = length,
                        .header      = header,
                        .payload     = packet + sizeof(mr_packet_header_t),
                        .payload_len = length - sizeof(mr_packet_header_t) }
                };
                _mira_vars.app_event_callback(MIRA_NEW_PACKET, event_data);
                mr_assoc_node_keep_gateway_alive(mr_mac_get_asn());
                break;
            }
            case MIRA_PACKET_KEEPALIVE:
                if (!from_my_joined_gateway) {
                    // ignore keep-alives from other gateways
                    return;
                }
                mr_assoc_node_keep_gateway_alive(mr_mac_get_asn());
                break;
            default:
                break;
        }
    }
}

void mira_event_loop(void) {
    // process the event loop
    switch (mira_get_node_type()) {
        case MIRA_GATEWAY:
            mr_bloom_gateway_event_loop();
            break;
        case MIRA_NODE:
            break;
    }
}

//=========================== callbacks ===========================================

static void event_callback(mr_event_t event, mr_event_data_t event_data) {
    // handle some events internally
    switch (event) {
        case MIRA_NODE_LEFT:
            mr_bloom_gateway_set_dirty();
            break;
        default:
            break;
    }

    // forward the event to the application callback
    if (_mira_vars.app_event_callback) {
        _mira_vars.app_event_callback(event, event_data);
    }
}
