/**
 * @file
 * @ingroup bsp_timer_hf
 *
 * @brief  nRF52833-specific definition of the "timer hf" bsp module.
 *
 * @author Alexandre Abadie <alexandre.abadie@inria.fr>
 *
 * @copyright Inria, 2022
 */
#include <nrf.h>
#include <nrf_peripherals.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "mr_clock.h"
#include "mr_timer_hf.h"

//=========================== define ===========================================

#define TIMER_IRQ_PRIORITY (2U)  ///< Timer interrupt priority
#define TIMER_MAX_CHANNELS (6U)

typedef struct {
    NRF_TIMER_Type *p;
    IRQn_Type       irq;
    uint8_t         cc_num;
} timer_hf_conf_t;

typedef struct {
    uint32_t      period_us;  ///< Period in ticks between each callback
    bool          one_shot;   ///< Whether this is a one shot callback
    timer_hf_cb_t callback;   ///< Pointer to the callback function
} timer_hf_callback_t;

typedef struct {
    timer_hf_callback_t timer_callback[TIMER_MAX_CHANNELS];  ///< List of timer callback structs
    bool                running;                             ///< Whether the delay timer is running
} timer_hf_vars_t;

//=========================== variables ========================================

static const timer_hf_conf_t _devs[TIMER_COUNT] = {
#if defined(NRF5340_XXAA)
    {
#if defined(NRF_NETWORK) || defined(NRF_TRUSTZONE_NONSECURE)
        .p = NRF_TIMER0_NS,
#else
        .p = NRF_TIMER0_S,
#endif
        .irq    = TIMER0_IRQn,
        .cc_num = TIMER0_CC_NUM - 1,
    },
    {
#if defined(NRF_NETWORK) || defined(NRF_TRUSTZONE_NONSECURE)
        .p = NRF_TIMER1_NS,
#else
        .p = NRF_TIMER1_S,
#endif
        .irq    = TIMER1_IRQn,
        .cc_num = TIMER1_CC_NUM - 1,
    },
    {
#if defined(NRF_NETWORK) || defined(NRF_TRUSTZONE_NONSECURE)
        .p = NRF_TIMER2_NS,
#else
        .p = NRF_TIMER2_S,
#endif
        .irq    = TIMER2_IRQn,
        .cc_num = TIMER2_CC_NUM - 1,
    },
#else
    {
        .p      = NRF_TIMER0,
        .irq    = TIMER0_IRQn,
        .cc_num = TIMER0_CC_NUM - 1,
    },
    {
        .p      = NRF_TIMER1,
        .irq    = TIMER1_IRQn,
        .cc_num = TIMER1_CC_NUM - 1,
    },
    {
        .p      = NRF_TIMER2,
        .irq    = TIMER2_IRQn,
        .cc_num = TIMER2_CC_NUM - 1,
    },
    {
        .p      = NRF_TIMER3,
        .irq    = TIMER3_IRQn,
        .cc_num = TIMER3_CC_NUM - 1,
    },
    {
        .p      = NRF_TIMER4,
        .irq    = TIMER4_IRQn,
        .cc_num = TIMER4_CC_NUM - 1,
    },
#endif
};

static timer_hf_vars_t _timer_hf_vars[TIMER_COUNT] = { 0 };

//=========================== public ===========================================

void mr_timer_hf_init(timer_hf_t timer) {
    // No delay is running after initialization
    _timer_hf_vars[timer].running = false;

#if !defined(USE_SWARMIT)
    // Configure and start High Frequency clock
    mr_hfclk_init();
#endif

    // Configure the timer
    _devs[timer].p->TASKS_CLEAR = 1;
    _devs[timer].p->PRESCALER   = 4;  // Run TIMER at 1MHz
    _devs[timer].p->BITMODE     = (TIMER_BITMODE_BITMODE_32Bit << TIMER_BITMODE_BITMODE_Pos);
    _devs[timer].p->INTENSET    = (1 << (TIMER_INTENSET_COMPARE0_Pos + _devs[timer].cc_num));
    NVIC_SetPriority(_devs[timer].irq, TIMER_IRQ_PRIORITY);
    NVIC_EnableIRQ(_devs[timer].irq);

    // Start the timer
    _devs[timer].p->TASKS_START = 1;
}

uint32_t mr_timer_hf_now(timer_hf_t timer) {
    _devs[timer].p->TASKS_CAPTURE[_devs[timer].cc_num] = 1;
    return _devs[timer].p->CC[_devs[timer].cc_num];
}

void mr_timer_hf_set_periodic_us(timer_hf_t timer, uint8_t channel, uint32_t us, timer_hf_cb_t cb) {
    assert(channel >= 0 && channel < _devs[timer].cc_num + 1);  // Make sure the required channel is correct
    assert(cb);                                                 // Make sure the callback function is valid

    _timer_hf_vars[timer].timer_callback[channel].period_us = us;
    _timer_hf_vars[timer].timer_callback[channel].one_shot  = false;
    _timer_hf_vars[timer].timer_callback[channel].callback  = cb;
    _devs[timer].p->INTENSET                                = (1 << (TIMER_INTENSET_COMPARE0_Pos + channel));
    _devs[timer].p->TASKS_CAPTURE[channel]                  = 1;
    _devs[timer].p->CC[channel] += _timer_hf_vars[timer].timer_callback[channel].period_us;
}

void mr_timer_hf_adjust_periodic_us(timer_hf_t timer, uint8_t channel, int32_t adjust_us) {
    assert(channel >= 0 && channel < _devs[timer].cc_num + 1);  // Make sure the required channel is correct

    // Only update the CC register, so that the adjust applies only to the current "tick"
    _devs[timer].p->CC[channel] += adjust_us;
}

void mr_timer_hf_set_oneshot_us(timer_hf_t timer, uint8_t channel, uint32_t us, timer_hf_cb_t cb) {
    assert(channel >= 0 && channel < _devs[timer].cc_num + 1);  // Make sure the required channel is correct
    assert(cb);                                                 // Make sure the callback function is valid

    _timer_hf_vars[timer].timer_callback[channel].period_us = us;
    _timer_hf_vars[timer].timer_callback[channel].one_shot  = true;
    _timer_hf_vars[timer].timer_callback[channel].callback  = cb;
    _devs[timer].p->INTENSET                                = (1 << (TIMER_INTENSET_COMPARE0_Pos + channel));
    _devs[timer].p->TASKS_CAPTURE[channel]                  = 1;
    _devs[timer].p->CC[channel] += _timer_hf_vars[timer].timer_callback[channel].period_us;
}

void mr_timer_hf_set_oneshot_with_ref_us(timer_hf_t timer, uint8_t channel, uint32_t base_us, uint32_t us, timer_hf_cb_t cb) {
    // assert(channel >= 0 && channel < _devs[timer].cc_num + 1);  // Make sure the required channel is correct
    // assert(cb);                                                 // Make sure the callback function is valid

    uint32_t now                                            = mr_timer_hf_now(timer);
    uint32_t period_us                                      = us + (now - base_us);
    _timer_hf_vars[timer].timer_callback[channel].period_us = period_us;
    _timer_hf_vars[timer].timer_callback[channel].one_shot  = true;
    _timer_hf_vars[timer].timer_callback[channel].callback  = cb;
    _devs[timer].p->INTENSET                                = (1 << (TIMER_INTENSET_COMPARE0_Pos + channel));
    _devs[timer].p->TASKS_CAPTURE[channel]                  = 1;
    _devs[timer].p->CC[channel] += _timer_hf_vars[timer].timer_callback[channel].period_us;
}

void mr_timer_hf_set_oneshot_with_ref_diff_us(timer_hf_t timer, uint8_t channel, uint32_t base_us, uint32_t us, timer_hf_cb_t cb) {
    // assert(channel >= 0 && channel < _devs[timer].cc_num + 1);  // Make sure the required channel is correct
    // assert(cb);                                                 // Make sure the callback function is valid

    uint32_t now                                            = mr_timer_hf_now(timer);
    uint32_t period_us                                      = us - (now - base_us);
    _timer_hf_vars[timer].timer_callback[channel].period_us = period_us;
    _timer_hf_vars[timer].timer_callback[channel].one_shot  = true;
    _timer_hf_vars[timer].timer_callback[channel].callback  = cb;
    _devs[timer].p->INTENSET                                = (1 << (TIMER_INTENSET_COMPARE0_Pos + channel));
    _devs[timer].p->TASKS_CAPTURE[channel]                  = 1;
    _devs[timer].p->CC[channel] += _timer_hf_vars[timer].timer_callback[channel].period_us;
}

void mr_timer_hf_cancel(timer_hf_t timer, uint8_t channel) {
    assert(channel >= 0 && channel < _devs[timer].cc_num + 1);

    // clear the variables
    _timer_hf_vars[timer].timer_callback[channel].period_us = 0;
    _timer_hf_vars[timer].timer_callback[channel].callback  = NULL;

    // disable the interrupt and clear the event flag
    _devs[timer].p->INTENCLR                = (1 << (TIMER_INTENCLR_COMPARE0_Pos + channel));
    _devs[timer].p->EVENTS_COMPARE[channel] = 0;
    _devs[timer].p->CC[channel]             = 0;
}

void mr_timer_hf_set_oneshot_ms(timer_hf_t timer, uint8_t channel, uint32_t ms, timer_hf_cb_t cb) {
    mr_timer_hf_set_oneshot_us(timer, channel, ms * 1000UL, cb);
}

void mr_timer_hf_set_oneshot_s(timer_hf_t timer, uint8_t channel, uint32_t s, timer_hf_cb_t cb) {
    mr_timer_hf_set_oneshot_us(timer, channel, s * 1000UL * 1000UL, cb);
}

void mr_timer_hf_delay_us(timer_hf_t timer, uint32_t us) {
    _devs[timer].p->TASKS_CAPTURE[_devs[timer].cc_num] = 1;
    _devs[timer].p->CC[_devs[timer].cc_num] += us;
    _timer_hf_vars[timer].running = true;
    while (_timer_hf_vars[timer].running) {
        __WFE();
        __SEV();
        __WFE();
    }
}

void mr_timer_hf_delay_ms(timer_hf_t timer, uint32_t ms) {
    mr_timer_hf_delay_us(timer, ms * 1000UL);
}

void mr_timer_hf_delay_s(timer_hf_t timer, uint32_t s) {
    mr_timer_hf_delay_us(timer, s * 1000UL * 1000UL);
}

//=========================== interrupt ========================================

static void _timer_hf_isr(timer_hf_t timer) {
    if (_devs[timer].p->EVENTS_COMPARE[_devs[timer].cc_num] == 1) {
        _devs[timer].p->EVENTS_COMPARE[_devs[timer].cc_num] = 0;
        _timer_hf_vars[timer].running                       = false;
        __SEV();
    }

    for (uint8_t channel = 0; channel < _devs[timer].cc_num; ++channel) {
        if (_devs[timer].p->EVENTS_COMPARE[channel] == 1) {
            _devs[timer].p->EVENTS_COMPARE[channel] = 0;
            if (_timer_hf_vars[timer].timer_callback[channel].one_shot) {
                _devs[timer].p->INTENCLR = (1 << (TIMER_INTENCLR_COMPARE0_Pos + channel));
            } else {
                _devs[timer].p->CC[channel] += _timer_hf_vars[timer].timer_callback[channel].period_us;
            }
            if (_timer_hf_vars[timer].timer_callback[channel].callback) {
                _timer_hf_vars[timer].timer_callback[channel].callback();
            }
        }
    }
}

void TIMER0_IRQHandler(void) {
    _timer_hf_isr(0);
}

void TIMER1_IRQHandler(void) {
    _timer_hf_isr(1);
}

void TIMER2_IRQHandler(void) {
    _timer_hf_isr(2);
}

#if !defined(NRF5340_XXAA)
void TIMER3_IRQHandler(void) {
    _timer_hf_isr(3);
}

void TIMER4_IRQHandler(void) {
    _timer_hf_isr(4);
}
#endif
