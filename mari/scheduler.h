#ifndef __SCHEDULER_H
#define __SCHEDULER_H

/**
 * @defgroup    net_scheduler      SCHEDULER radio driver
 * @ingroup     drv
 * @brief       Driver for the TSCH scheduler
 *
 * @{
 * @file
 * @author Geovane Fedrecheski <geovane.fedrecheski@inria.fr>
 * @copyright Inria, 2024-now
 * @}
 */

#include <stdint.h>
#include <stdbool.h>
#include <nrf.h>

#include "models.h"

//=========================== defines ==========================================

#define MARI_STATS_SCHED_USAGE_SIZE 4  // supports schedules with up to 256 cells

//=========================== prototypes ==========================================

/**
 * @brief Initializes the scheduler
 *
 * If a schedule is not set, the first schedule in the available_schedules array is used.
 *
 * @param[in] schedule         Schedule to be used.
 */
void mr_scheduler_init(schedule_t *application_schedule);

/**
 * @brief Advances the schedule by one cell/slot.
 *
 * @return A configuration for the TSCH radio driver to follow in the next slot.
 */
mr_slot_info_t mr_scheduler_tick(uint64_t asn);

/**
 * @brief Activates a given schedule.
 *
 * This can be used at runtime to change the schedule, for example after receiving a beacon with a different schedule id.
 *
 * @param[in] schedule_id         Schedule ID
 *
 * @return true if the schedule was successfully set, false otherwise
 */
bool mr_scheduler_set_schedule(uint8_t schedule_id);

uint32_t mr_scheduler_get_duration_us(void);

int16_t mr_scheduler_gateway_assign_next_available_uplink_cell(uint64_t node_id, uint64_t asn);

bool mr_scheduler_node_assign_myself_to_cell(uint16_t cell_index);

void mr_scheduler_node_deassign_myself_from_schedule(void);

void mr_scheduler_gateway_decrease_nodes_counter(void);

uint8_t mr_scheduler_gateway_remaining_capacity(void);

uint8_t mr_scheduler_gateway_get_nodes_count(void);

uint8_t mr_scheduler_gateway_get_nodes(uint64_t *nodes);

schedule_t *mr_scheduler_get_active_schedule_ptr(void);

uint8_t mr_scheduler_get_active_schedule_slot_count(void);

cell_t mr_scheduler_node_peek_slot(uint64_t asn);

void mr_scheduler_stats_register_used_slot(bool used);

uint64_t *mr_scheduler_get_schedule_usage(void);

/**
 * @brief Computes the channel to be used in a given slot.
 *
 * @param[in] slot_type         Type of slot
 * @param[in] asn               Absolute Slot Number
 * @param[in] channel_offset    Channel offset
 *
 * @return Channel to be used in the given slot
 *
 */
uint8_t mr_scheduler_get_channel(slot_type_t slot_type, uint64_t asn, uint8_t channel_offset);

uint8_t mr_scheduler_get_active_schedule_id(void);

#endif
