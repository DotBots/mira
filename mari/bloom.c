/**
 * @file
 * @ingroup     bloom
 *
 * @brief       Bloom filter
 *
 * @author Geovane Fedrecheski <geovane.fedrecheski@inria.fr>
 *
 * @copyright Inria, 2025
 */

#include <nrf.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "bloom.h"
#include "scheduler.h"

//=========================== defines ==========================================

typedef struct {
    // used by the gateway
    bool    is_dirty;                   // true if the bloom filter needs to be re-computed
    bool    is_available;               // true if the bloom filter is being computed
    uint8_t bloom[MARI_BLOOM_M_BYTES];  // bloom filter output
} bloom_vars_t;

//=========================== variables ========================================

static bloom_vars_t bloom_vars = { 0 };

//=========================== prototypes =======================================

//=========================== public ===========================================

// FNV-1a 64-bit hash
uint64_t mr_bloom_hash_fnv1a64(uint64_t input) {
    uint64_t hash = 0xcbf29ce484222325ULL;
    for (int b = 0; b < 8; b++) {
        uint8_t byte = (input >> (56 - b * 8)) & 0xFF;
        hash ^= byte;
        hash *= 0x100000001b3ULL;
    }
    return hash;
}

// -------- gateway ---------

void mr_bloom_gateway_init(void) {
    bloom_vars.is_dirty     = false;
    bloom_vars.is_available = false;
}

void mr_bloom_gateway_set_dirty(void) {
    bloom_vars.is_dirty = true;
}

void mr_bloom_gateway_set_clean(void) {
    bloom_vars.is_dirty = false;
}

bool mr_bloom_gateway_is_dirty(void) {
    return bloom_vars.is_dirty;
}

bool mr_bloom_gateway_is_available(void) {
    return bloom_vars.is_available;
}

uint8_t mr_bloom_gateway_copy(uint8_t *output) {
    memcpy(output, bloom_vars.bloom, MARI_BLOOM_M_BYTES);
    return MARI_BLOOM_M_BYTES;
}

void mr_bloom_gateway_compute(void) {
    bloom_vars.is_available = false;
    memset(bloom_vars.bloom, 0, MARI_BLOOM_M_BYTES);

    schedule_t *schedule_ptr = mr_scheduler_get_active_schedule_ptr();

    for (size_t i = 0; i < schedule_ptr->n_cells; i++) {
        cell_t *cell = &schedule_ptr->cells[i];
        if (cell->type != SLOT_TYPE_UPLINK) {
            continue;  // skip non-uplink cells
        }
        if (cell->assigned_node_id == NULL) {
            continue;  // skip empty cells
        }

        uint64_t h1 = cell->bloom_h1;
        uint64_t h2 = cell->bloom_h2;

        for (int k = 0; k < MARI_BLOOM_K_HASHES; k++) {
            uint64_t idx = (h1 + k * h2) & (MARI_BLOOM_M_BITS - 1);  // Fast bitmask instead of division
            bloom_vars.bloom[idx / 8] |= (1 << (idx % 8));
        }
    }
    bloom_vars.is_available = true;
}

void mr_bloom_gateway_event_loop(void) {
    // check if the bloom filter needs to be re-computed
    if (mr_bloom_gateway_is_dirty()) {
        mr_bloom_gateway_compute();
        mr_bloom_gateway_set_clean();
    }
}

// -------- node ---------

bool mr_bloom_node_contains(uint64_t node_id, const uint8_t *bloom) {
    uint64_t h1 = mr_bloom_hash_fnv1a64(node_id);
    uint64_t h2 = mr_bloom_hash_fnv1a64(node_id ^ MARI_BLOOM_FNV1A_H2_SALT);

    for (int k = 0; k < MARI_BLOOM_K_HASHES; k++) {
        uint64_t idx = (h1 + k * h2) & (MARI_BLOOM_M_BITS - 1);  // Fast bitmask instead of division
        if ((bloom[idx / 8] & (1 << (idx % 8))) == 0) {
            return false;
        }
    }
    return true;
}

//=========================== private ==========================================
