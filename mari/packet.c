/**
 * @brief       Build a mari packets
 *
 * @author Geovane Fedrecheski <geovane.fedrecheski@inria.fr>
 *
 * @copyright Inria, 2024
 */
#include <stdint.h>
#include <string.h>

#include "mr_device.h"
#include "scheduler.h"
#include "association.h"
#include "packet.h"

//=========================== prototypes =======================================

static size_t _set_header(uint8_t *buffer, uint64_t dst, mr_packet_type_t packet_type);

//=========================== public ===========================================

size_t mr_build_packet_data(uint8_t *buffer, uint64_t dst, uint8_t *data, size_t data_len) {
    size_t header_len = _set_header(buffer, dst, MARI_PACKET_DATA);
    memcpy(buffer + header_len, data, data_len);
    return header_len + data_len;
}

size_t mr_build_packet_keepalive(uint8_t *buffer, uint64_t dst) {
    return _set_header(buffer, dst, MARI_PACKET_KEEPALIVE);
}

size_t mr_build_packet_join_request(uint8_t *buffer, uint64_t dst) {
    return _set_header(buffer, dst, MARI_PACKET_JOIN_REQUEST);
}

size_t mr_build_packet_join_response(uint8_t *buffer, uint64_t dst) {
    return _set_header(buffer, dst, MARI_PACKET_JOIN_RESPONSE);
}

size_t mr_build_packet_beacon(uint8_t *buffer, uint16_t net_id, uint64_t asn, uint8_t remaining_capacity, uint8_t active_schedule_id) {
    mr_beacon_packet_header_t beacon = {
        .version            = MARI_PROTOCOL_VERSION,
        .type               = MARI_PACKET_BEACON,
        .network_id         = net_id,
        .asn                = asn,
        .src                = mr_device_id(),
        .remaining_capacity = remaining_capacity,
        .active_schedule_id = active_schedule_id,
    };
    memcpy(buffer, &beacon, sizeof(mr_beacon_packet_header_t));
    return sizeof(mr_beacon_packet_header_t);
}

size_t mr_build_uart_packet_gateway_info(uint8_t *buffer) {
    uint64_t  device_id   = mr_device_id();
    uint16_t  net_id      = mr_assoc_get_network_id();
    uint16_t  schedule_id = mr_scheduler_get_active_schedule_id();
    uint64_t *sched_usage = mr_scheduler_get_schedule_usage();

    memcpy(buffer, &device_id, sizeof(uint64_t));
    memcpy(buffer + sizeof(uint64_t), &net_id, sizeof(uint16_t));
    memcpy(buffer + sizeof(uint64_t) + sizeof(uint16_t), &schedule_id, sizeof(uint16_t));
    memcpy(buffer + sizeof(uint64_t) + sizeof(uint16_t) + sizeof(uint16_t), sched_usage, sizeof(uint64_t) * MARI_STATS_SCHED_USAGE_SIZE);

    return sizeof(uint64_t) + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint64_t) * MARI_STATS_SCHED_USAGE_SIZE;
}

//=========================== private ==========================================

static size_t _set_header(uint8_t *buffer, uint64_t dst, mr_packet_type_t packet_type) {
    uint64_t src = mr_device_id();

    mr_packet_header_t header = {
        .version    = MARI_PROTOCOL_VERSION,
        .type       = packet_type,
        .network_id = mr_assoc_get_network_id(),
        .dst        = dst,
        .src        = src,
    };
    memcpy(buffer, &header, sizeof(mr_packet_header_t));
    return sizeof(mr_packet_header_t);
}
