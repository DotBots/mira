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
#include <stdio.h>

#include "mr_device.h"
#include "mr_radio.h"
#include "mr_timer_hf.h"
#include "mr_ipc.h"
#include "scheduler.h"
#include "mira.h"
#include "packet.h"

//=========================== defines ==========================================

#define MIRA_APP_TIMER_DEV 1

typedef struct {
    bool dummy;
} gateway_vars_t;

//=========================== variables ========================================

gateway_vars_t gateway_vars = { 0 };

extern schedule_t schedule_minuscule, schedule_tiny, schedule_huge;
schedule_t       *schedule_app = &schedule_huge;

extern volatile __attribute__((section(".shared_data"))) ipc_shared_data_t ipc_shared_data;

//=========================== prototypes =======================================

void mira_event_callback(mr_event_t event, mr_event_data_t event_data);

//=========================== main =============================================

int main(void) {
    printf("Hello Mira Gateway Net Core %016llX\n", mr_device_id());
    mr_timer_hf_init(MIRA_APP_TIMER_DEV);

    mira_init(MIRA_GATEWAY, MIRA_NET_ID_DEFAULT, schedule_app, &mira_event_callback);

    mr_timer_hf_set_periodic_us(MIRA_APP_TIMER_DEV, 2, mr_scheduler_get_duration_us(), &mira_event_loop);

    // TODO: communicate with the application core via IPC
    // Network core must remain on
    ipc_shared_data.net_ready = true;

    while (1) {
        __SEV();
        __WFE();
        __WFE();
    }
}

//=========================== callbacks ========================================

void mira_event_callback(mr_event_t event, mr_event_data_t event_data) {
    (void)event_data;
    uint32_t now_ts_s = mr_timer_hf_now(MIRA_APP_TIMER_DEV) / 1000 / 1000;
    switch (event) {
        case MIRA_NEW_PACKET:
        {
            break;
        }
        case MIRA_NODE_JOINED:
            printf("%d New node joined: %016llX  (%d nodes connected)\n", now_ts_s, event_data.data.node_info.node_id, mira_gateway_count_nodes());
            break;
        case MIRA_NODE_LEFT:
            printf("%d Node left: %016llX, reason: %u  (%d nodes connected)\n", now_ts_s, event_data.data.node_info.node_id, event_data.tag, mira_gateway_count_nodes());
            break;
        case MIRA_ERROR:
            printf("Error, reason: %u\n", event_data.tag);
            break;
        default:
            break;
    }
}

//=========================== private ========================================
