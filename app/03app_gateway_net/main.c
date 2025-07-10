/**
 * @file
 * @ingroup     app
 *
 * @brief       Mira Gateway application (radio side)
 *
 * @author Geovane Fedrecheski <geovane.fedrecheski@inria.fr>
 * @author Alexandre Abadie <alexandre.abadie@inria.fr>
 *
 * @copyright Inria, 2025-now
 */
#include <nrf.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ipc.h"

#include "mr_device.h"
#include "mr_timer_hf.h"
#include "association.h"
#include "scheduler.h"
#include "mira.h"
#include "packet.h"

//=========================== defines ==========================================

#define MIRA_APP_TIMER_DEV 1

typedef struct {
    bool uart_to_radio_packet_ready;
} gateway_vars_t;

//=========================== variables ========================================

gateway_vars_t _app_vars = { 0 };

extern schedule_t schedule_minuscule, schedule_tiny, schedule_huge;
schedule_t       *schedule_app = &schedule_huge;

volatile __attribute__((section(".shared_data"))) ipc_shared_data_t ipc_shared_data;

static void _mira_event_callback(mr_event_t event, mr_event_data_t event_data) {
    (void)event_data;
    uint32_t now_ts_s = mr_timer_hf_now(MIRA_APP_TIMER_DEV) / 1000 / 1000;
    switch (event) {
        case MIRA_NEW_PACKET:
        {
            ipc_shared_data.radio_to_uart_len = event_data.data.new_packet.len + 1;
            ipc_shared_data.radio_to_uart[0]  = MIRA_EDGE_DATA;
            memcpy((void *)ipc_shared_data.radio_to_uart + 1, event_data.data.new_packet.header, event_data.data.new_packet.len);
            break;
        }
        case MIRA_KEEPALIVE:
            ipc_shared_data.radio_to_uart_len = 1 + sizeof(uint64_t);
            ipc_shared_data.radio_to_uart[0]  = MIRA_EDGE_KEEPALIVE;
            memcpy((void *)ipc_shared_data.radio_to_uart + 1, &event_data.data.node_info.node_id, sizeof(uint64_t));
            break;
        case MIRA_NODE_JOINED:
        {
            printf("%d + Node %016llX joined  (%d nodes connected)\n", now_ts_s, event_data.data.node_info.node_id, mira_gateway_count_nodes());
            ipc_shared_data.radio_to_uart_len = 1 + sizeof(uint64_t);
            ipc_shared_data.radio_to_uart[0]  = MIRA_EDGE_NODE_JOINED;
            memcpy((void *)ipc_shared_data.radio_to_uart + 1, &event_data.data.node_info.node_id, sizeof(uint64_t));

            break;
        }
        case MIRA_NODE_LEFT:
        {
            printf("%d - Node %016llX left, reason: %u  (%d nodes connected)\n", now_ts_s, event_data.data.node_info.node_id, event_data.tag, mira_gateway_count_nodes());
            ipc_shared_data.radio_to_uart_len = 1 + sizeof(uint64_t);
            ipc_shared_data.radio_to_uart[0]  = MIRA_EDGE_NODE_LEFT;
            memcpy((void *)ipc_shared_data.radio_to_uart + 1, &event_data.data.node_info.node_id, sizeof(uint64_t));
            break;
        }
        case MIRA_ERROR:
            printf("Error, reason: %u\n", event_data.tag);
            break;
        default:
            return;
    }

    NRF_IPC_NS->TASKS_SEND[IPC_CHAN_RADIO_TO_UART] = 1;
}

static void _to_uart_gateway_loop(void) {
    ipc_shared_data.radio_to_uart[0]               = MIRA_EDGE_GATEWAY_INFO;
    size_t len                                     = mr_build_uart_packet_gateway_info((uint8_t *)(ipc_shared_data.radio_to_uart + 1));
    ipc_shared_data.radio_to_uart_len              = 1 + len;
    NRF_IPC_NS->TASKS_SEND[IPC_CHAN_RADIO_TO_UART] = 1;
}

static void _init_ipc(void) {
    NRF_IPC_NS->INTENSET                            = (1 << IPC_CHAN_UART_TO_RADIO);
    NRF_IPC_NS->SEND_CNF[IPC_CHAN_RADIO_TO_UART]    = (1 << IPC_CHAN_RADIO_TO_UART);
    NRF_IPC_NS->RECEIVE_CNF[IPC_CHAN_UART_TO_RADIO] = (1 << IPC_CHAN_UART_TO_RADIO);

    NVIC_EnableIRQ(IPC_IRQn);
    NVIC_ClearPendingIRQ(IPC_IRQn);
    NVIC_SetPriority(IPC_IRQn, IPC_IRQ_PRIORITY);
}

int main(void) {
    printf("Hello Mira Gateway Net Core %016llX\n", mr_device_id());
    mr_timer_hf_init(MIRA_APP_TIMER_DEV);
    _init_ipc();

    mira_init(MIRA_GATEWAY, MIRA_NET_ID_DEFAULT, schedule_app, &_mira_event_callback);

    // mr_timer_hf_set_periodic_us(MIRA_APP_TIMER_DEV, 2, mr_scheduler_get_duration_us(), &mira_event_loop);
    mr_timer_hf_set_periodic_us(MIRA_APP_TIMER_DEV, 3, mr_scheduler_get_duration_us() * 10, &_to_uart_gateway_loop);

    // Unlock the application core
    ipc_shared_data.net_ready = true;

    while (1) {
        __WFE();

        mira_event_loop();

        if (_app_vars.uart_to_radio_packet_ready) {
            _app_vars.uart_to_radio_packet_ready = false;
            uint8_t packet_type                  = ipc_shared_data.uart_to_radio_tx[0];
            if (packet_type != 0x01) {
                printf("Invalid UART packet type: %02X\n", packet_type);
                continue;
            }

            uint8_t *mira_frame     = (uint8_t *)ipc_shared_data.uart_to_radio_tx + 1;
            uint8_t  mira_frame_len = ipc_shared_data.uart_to_radio_len - 1;

            mr_packet_header_t *header = (mr_packet_header_t *)mira_frame;
            header->src                = mr_device_id();
            header->network_id         = mr_assoc_get_network_id();

            mira_tx(mira_frame, mira_frame_len);
        }
    }
}

void IPC_IRQHandler(void) {
    if (NRF_IPC_NS->EVENTS_RECEIVE[IPC_CHAN_UART_TO_RADIO]) {
        NRF_IPC_NS->EVENTS_RECEIVE[IPC_CHAN_UART_TO_RADIO] = 0;
        _app_vars.uart_to_radio_packet_ready               = true;
    }
}
