#ifndef __TZ_H
#define __TZ_H

/**
 * @defgroup    bsp_tz ARM TrustZone configuration
 * @ingroup     bsp
 * @brief       nRF53 ARM TrustZone configuration
 *
 * @{
 * @file
 * @author Alexandre Abadie <alexandre.abadie@inria.fr>
 * @copyright Inria, 2024
 * @}
 */

#include <stdlib.h>
#include <stdint.h>
#include <nrf.h>

#define NRF_APPLICATION_PERIPH_ID_DCNF_FPU                       (0U)
#define NRF_APPLICATION_PERIPH_ID_OSCILLATORS_REGULATORS         (4U)
#define NRF_APPLICATION_PERIPH_ID_CLOCK_POWER_RESET              (5U)
#define NRF_APPLICATION_PERIPH_ID_CTRLAP                         (6U)
#define NRF_APPLICATION_PERIPH_ID_SPIM0_SPIS0_TWIM0_TWIS0_UARTE0 (8U)
#define NRF_APPLICATION_PERIPH_ID_SPIM1_SPIS1_TWIM1_TWIS1_UARTE1 (9U)
#define NRF_APPLICATION_PERIPH_ID_SPIM4                          (10U)
#define NRF_APPLICATION_PERIPH_ID_SPIM2_SPIS2_TWIM2_TWIS2_UARTE2 (11U)
#define NRF_APPLICATION_PERIPH_ID_SPIM3_SPIS3_TWIM3_TWIS3_UARTE3 (12U)
#define NRF_APPLICATION_PERIPH_ID_SAADC                          (14U)
#define NRF_APPLICATION_PERIPH_ID_TIMER0                         (15U)
#define NRF_APPLICATION_PERIPH_ID_TIMER1                         (16U)
#define NRF_APPLICATION_PERIPH_ID_TIMER2                         (17U)
#define NRF_APPLICATION_PERIPH_ID_RTC0                           (20U)
#define NRF_APPLICATION_PERIPH_ID_RTC1                           (21U)
#define NRF_APPLICATION_PERIPH_ID_DPPIC                          (23U)
#define NRF_APPLICATION_PERIPH_ID_WDT0                           (24U)
#define NRF_APPLICATION_PERIPH_ID_WDT1                           (25U)
#define NRF_APPLICATION_PERIPH_ID_COMP_LPCOMP                    (26U)
#define NRF_APPLICATION_PERIPH_ID_EGU0                           (27U)
#define NRF_APPLICATION_PERIPH_ID_EGU1                           (28U)
#define NRF_APPLICATION_PERIPH_ID_EGU2                           (29U)
#define NRF_APPLICATION_PERIPH_ID_EGU3                           (30U)
#define NRF_APPLICATION_PERIPH_ID_EGU4                           (31U)
#define NRF_APPLICATION_PERIPH_ID_EGU5                           (32U)
#define NRF_APPLICATION_PERIPH_ID_PWM0                           (33U)
#define NRF_APPLICATION_PERIPH_ID_PWM1                           (34U)
#define NRF_APPLICATION_PERIPH_ID_PWM2                           (35U)
#define NRF_APPLICATION_PERIPH_ID_PWM3                           (36U)
#define NRF_APPLICATION_PERIPH_ID_PDM0                           (38U)
#define NRF_APPLICATION_PERIPH_ID_I2S0                           (40U)
#define NRF_APPLICATION_PERIPH_ID_IPC                            (42U)
#define NRF_APPLICATION_PERIPH_ID_QSPI                           (43U)
#define NRF_APPLICATION_PERIPH_ID_NFCT                           (45U)
#define NRF_APPLICATION_PERIPH_ID_MUTEX                          (48U)
#define NRF_APPLICATION_PERIPH_ID_QDEC0                          (51U)
#define NRF_APPLICATION_PERIPH_ID_QDEC1                          (52U)
#define NRF_APPLICATION_PERIPH_ID_USBD                           (54U)
#define NRF_APPLICATION_PERIPH_ID_USBREGULATOR                   (55U)
#define NRF_APPLICATION_PERIPH_ID_NVMC                           (57U)
#define NRF_APPLICATION_PERIPH_ID_P0_P1                          (66U)
#define NRF_APPLICATION_PERIPH_ID_VMC                            (129U)

void tz_configure_periph_non_secure(uint8_t periph_id);
void tz_configure_periph_dma_non_secure(uint8_t periph_id);
void tz_configure_flash_secure(uint8_t start_region, size_t length);
void tz_configure_flash_non_secure(uint8_t start_region, size_t length);
void tz_configure_ram_secure(uint8_t start_region, size_t length);
void tz_configure_ram_non_secure(uint8_t start_region, size_t length);

#endif  // __TZ_H
