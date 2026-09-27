#include "frequency_manager.h"

#include <string.h>

void atu_frequency_manager_init(atu_frequency_manager_t *manager)
{
    if (manager == NULL) {
        return;
    }

    memset(manager, 0, sizeof(*manager));
    manager->initialized = true;
    manager->last_source = ATU_FREQUENCY_SOURCE_MANUAL;
}

atu_storage_status_t atu_frequency_manager_update(atu_frequency_manager_t *manager,
                                                  const atu_fram_ops_t *ops,
                                                  const atu_frequency_inputs_t *inputs,
                                                  atu_frequency_result_t *result)
{
    atu_storage_status_t status = ATU_STORAGE_NOT_FOUND;
    bool can_use_cat = false;
    bool can_use_counter = false;

    if ((manager == NULL) || (inputs == NULL) || (result == NULL)) {
        return ATU_STORAGE_BAD_ARGUMENT;
    }

    memset(result, 0, sizeof(*result));
    result->source = ATU_FREQUENCY_SOURCE_MANUAL;
    result->ui_state = ATU_FREQUENCY_UI_MANUAL;

    can_use_cat = inputs->usb_present && inputs->cat_available && (inputs->cat_age_ms <= ATU_CAT_TIMEOUT_MS);
    can_use_counter = (!inputs->usb_present) && inputs->counter_available;

    if (can_use_cat) {
        result->source = ATU_FREQUENCY_SOURCE_CAT;
        result->active_frequency_hz = inputs->cat_frequency_hz;
    } else if (can_use_counter) {
        result->source = ATU_FREQUENCY_SOURCE_COUNTER;
        result->active_frequency_hz = inputs->counter_frequency_hz;
    }

    result->source_changed = (result->source != manager->last_source);
    result->frequency_changed = (!manager->have_frequency) || (result->active_frequency_hz != manager->last_frequency_hz);

    if (result->source == ATU_FREQUENCY_SOURCE_MANUAL) {
        manager->last_source = result->source;
        manager->have_frequency = false;
        manager->last_frequency_hz = 0u;
        return ATU_STORAGE_NOT_FOUND;
    }

    result->supported_band = atu_storage_find_band(result->active_frequency_hz, NULL);
    if (!result->supported_band) {
        result->source = ATU_FREQUENCY_SOURCE_MANUAL;
        result->ui_state = ATU_FREQUENCY_UI_MANUAL;
        manager->last_source = result->source;
        manager->have_frequency = false;
        manager->last_frequency_hz = result->active_frequency_hz;
        return ATU_STORAGE_NOT_FOUND;
    }

    status = atu_storage_load_preset(ops, inputs->active_bank, result->active_frequency_hz, &result->preset);
    if (status == ATU_STORAGE_OK) {
        result->preset_found = true;
        result->apply_preset = result->frequency_changed || result->source_changed;
        result->ui_state = ATU_FREQUENCY_UI_PRESET_FOUND;
    } else {
        result->preset_found = false;
        result->ui_state = ATU_FREQUENCY_UI_PRESET_MISSING;
    }

    manager->last_source = result->source;
    manager->last_frequency_hz = result->active_frequency_hz;
    manager->have_frequency = true;

    return status;
}
