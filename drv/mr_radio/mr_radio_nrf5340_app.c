/**
 * @file
 * @ingroup bsp_radio
 *
 * @brief  nrf5340-app-specific definition of the "radio" bsp module.
 *
 * @author Said Alvarado-Marin <said-alexander.alvarado-marin@inria.fr>
 *
 * @copyright Inria, 2022
 */
#include <nrf.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "mr_clock.h"
#include "ipc.h"
#include "mr_radio.h"
#include "mr_timer_hf.h"
#include "tz.h"

//=========================== variables ========================================

static mr_radio_cb_t _radio_callback = NULL;

extern volatile __attribute__((section(".shared_data"))) ipc_shared_data_t ipc_shared_data;

//=========================== public ===========================================

void mr_radio_init(mr_radio_cb_t callback, mr_radio_mode_t mode) {
    mr_hfclk_init();

    // APPMUTEX (address at 0x41030000 => periph ID is 48)
    mr_tz_enable_network_periph(NRF_NETWORK_PERIPH_ID_APPMUTEX);

    // Define RAMREGION 2 (0x20004000 to 0x20005FFF, e.g 8KiB) as non secure. It's used to share data between cores
    mr_configure_ram_non_secure(2, 1);

    NRF_IPC_S->INTENSET                          = 1 << MR_IPC_CHAN_RADIO_RX;
    NRF_IPC_S->SEND_CNF[MR_IPC_CHAN_REQ]         = 1 << MR_IPC_CHAN_REQ;
    NRF_IPC_S->RECEIVE_CNF[MR_IPC_CHAN_RADIO_RX] = 1 << MR_IPC_CHAN_RADIO_RX;

    NVIC_EnableIRQ(IPC_IRQn);
    NVIC_ClearPendingIRQ(IPC_IRQn);
    NVIC_SetPriority(IPC_IRQn, IPC_IRQ_PRIORITY);

    // Start the network core
    release_network_core();

    if (callback) {
        _radio_callback = callback;
    }

    ipc_shared_data.radio.mode = mode;
    mr_ipc_network_call(MR_IPC_RADIO_INIT_REQ);
}

void mr_radio_set_frequency(uint8_t freq) {
    ipc_shared_data.radio.frequency = freq;
    mr_ipc_network_call(MR_IPC_RADIO_FREQ_REQ);
}

void mr_radio_set_channel(uint8_t channel) {
    ipc_shared_data.radio.channel = channel;
    mr_ipc_network_call(MR_IPC_RADIO_CHAN_REQ);
}

void mr_radio_set_network_address(uint32_t addr) {
    ipc_shared_data.radio.addr = addr;
    mr_ipc_network_call(MR_IPC_RADIO_ADDR_REQ);
}

void mr_radio_tx(const uint8_t *tx_buffer, uint8_t length) {
    ipc_shared_data.radio.tx_pdu.length = length;
    memcpy((void *)ipc_shared_data.radio.tx_pdu.buffer, tx_buffer, length);
    mr_ipc_network_call(MR_IPC_RADIO_TX_REQ);
}

void mr_radio_rx(void) {
    mr_ipc_network_call(MR_IPC_RADIO_RX_REQ);
}

int8_t mr_radio_rssi(void) {
    mr_ipc_network_call(MR_IPC_RADIO_RSSI_REQ);
    return ipc_shared_data.radio.rssi;
}

void mr_radio_disable(void) {
    mr_ipc_network_call(MR_IPC_RADIO_DIS_REQ);
}

//=========================== interrupt handlers ===============================

void IPC_IRQHandler(void) {
    if (NRF_IPC_S->EVENTS_RECEIVE[MR_IPC_CHAN_RADIO_RX]) {
        NRF_IPC_S->EVENTS_RECEIVE[MR_IPC_CHAN_RADIO_RX] = 0;
        if (_radio_callback) {
            mutex_lock();
            _radio_callback((uint8_t *)ipc_shared_data.radio.rx_pdu.buffer, ipc_shared_data.radio.rx_pdu.length);
            mutex_unlock();
        }
    }
}
