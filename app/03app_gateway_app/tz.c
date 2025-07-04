#include <stdint.h>
#include <stdlib.h>

#include <nrf.h>
#include "tz.h"

void tz_configure_periph_non_secure(uint8_t periph_id) {
    NRF_SPU_S->PERIPHID[periph_id].PERM = SPU_PERIPHID_PERM_SECATTR_NonSecure << SPU_PERIPHID_PERM_SECATTR_Pos;
}

void tz_configure_periph_dma_non_secure(uint8_t periph_id) {
    NRF_SPU_S->PERIPHID[periph_id].PERM |= (SPU_PERIPHID_PERM_DMA_NoSeparateAttribute << SPU_PERIPHID_PERM_DMA_Pos |
                                            SPU_PERIPHID_PERM_DMASEC_NonSecure << SPU_PERIPHID_PERM_DMASEC_Pos);
}

static void _configure_flash(uint8_t start, size_t length, uint8_t secattr) {
    for (uint8_t region = start; region < start + length; region++) {
        NRF_SPU_S->FLASHREGION[region].PERM = (SPU_FLASHREGION_PERM_READ_Enable << SPU_FLASHREGION_PERM_READ_Pos |
                                               SPU_FLASHREGION_PERM_WRITE_Enable << SPU_FLASHREGION_PERM_WRITE_Pos |
                                               SPU_FLASHREGION_PERM_EXECUTE_Enable << SPU_FLASHREGION_PERM_EXECUTE_Pos |
                                               secattr << SPU_FLASHREGION_PERM_SECATTR_Pos);
    }
}

static void _configure_ram(uint8_t start, size_t length, uint8_t secattr) {
    for (uint8_t region = start; region < start + length; region++) {
        NRF_SPU_S->RAMREGION[region].PERM = (SPU_RAMREGION_PERM_READ_Enable << SPU_RAMREGION_PERM_READ_Pos |
                                             SPU_RAMREGION_PERM_WRITE_Enable << SPU_RAMREGION_PERM_WRITE_Pos |
                                             SPU_RAMREGION_PERM_EXECUTE_Enable << SPU_RAMREGION_PERM_EXECUTE_Pos |
                                             secattr << SPU_RAMREGION_PERM_SECATTR_Pos);
    }
}

void tz_configure_flash_secure(uint8_t start_region, size_t length) {
    _configure_flash(start_region, length, SPU_FLASHREGION_PERM_SECATTR_Secure);
}

void tz_configure_flash_non_secure(uint8_t start_region, size_t length) {
    _configure_flash(start_region, length, SPU_FLASHREGION_PERM_SECATTR_Non_Secure);
}

void tz_configure_ram_secure(uint8_t start_region, size_t length) {
    _configure_ram(start_region, length, SPU_RAMREGION_PERM_SECATTR_Secure);
}

void tz_configure_ram_non_secure(uint8_t start_region, size_t length) {
    _configure_ram(start_region, length, SPU_RAMREGION_PERM_SECATTR_Non_Secure);
}
