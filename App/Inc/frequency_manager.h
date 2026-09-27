#ifndef ATU_FREQUENCY_MANAGER_H
#define ATU_FREQUENCY_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

#include "cat_parser.h"
#include "storage.h"

/**
 * @file frequency_manager.h
 * @brief Frequency source arbitration and preset lookup
 */

typedef enum {
    ATU_FREQUENCY_SOURCE_CAT = FREQ_SOURCE_CAT,
    ATU_FREQUENCY_SOURCE_COUNTER = FREQ_SOURCE_COUNTER,
    ATU_FREQUENCY_SOURCE_TCI = FREQ_SOURCE_TCI,
    ATU_FREQUENCY_SOURCE_MANUAL = FREQ_SOURCE_MANUAL
} atu_frequency_source_t;

typedef enum {
    ATU_FREQUENCY_UI_MANUAL = 0,
    ATU_FREQUENCY_UI_PRESET_FOUND,
    ATU_FREQUENCY_UI_PRESET_MISSING
} atu_frequency_ui_state_t;

typedef struct {
    bool usb_present;
    bool cat_available;
    uint32_t cat_frequency_hz;
    uint32_t cat_age_ms;
    bool counter_available;
    uint32_t counter_frequency_hz;
    uint8_t active_bank;
} atu_frequency_inputs_t;

typedef struct {
    bool frequency_changed;
    bool source_changed;
    bool preset_found;
    bool apply_preset;
    bool supported_band;
    uint32_t active_frequency_hz;
    atu_frequency_source_t source;
    atu_frequency_ui_state_t ui_state;
    preset_t preset;
} atu_frequency_result_t;

typedef struct {
    bool initialized;
    bool have_frequency;
    uint32_t last_frequency_hz;
    atu_frequency_source_t last_source;
    uint8_t last_bank;
} atu_frequency_manager_t;

void atu_frequency_manager_init(atu_frequency_manager_t *manager);
atu_storage_status_t atu_frequency_manager_update(atu_frequency_manager_t *manager,
                                                  const atu_fram_ops_t *ops,
                                                  const atu_frequency_inputs_t *inputs,
                                                  atu_frequency_result_t *result);

#endif /* ATU_FREQUENCY_MANAGER_H */
