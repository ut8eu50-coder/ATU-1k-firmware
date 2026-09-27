#include "menu.h"

#include <stdio.h>
#include <string.h>

static bool atu_menu_is_increment_event(atu_button_event_t event)
{
    return ((event.button == ATU_BUTTON_L_PLUS) || (event.button == ATU_BUTTON_L_MINUS)) &&
           ((event.type == ATU_BUTTON_EVENT_PRESSED) || (event.type == ATU_BUTTON_EVENT_REPEAT));
}

static bool atu_menu_is_navigation_event(atu_button_event_t event)
{
    return ((event.button == ATU_BUTTON_C_PLUS) || (event.button == ATU_BUTTON_C_MINUS)) &&
           ((event.type == ATU_BUTTON_EVENT_PRESSED) || (event.type == ATU_BUTTON_EVENT_REPEAT));
}

static uint16_t atu_menu_scale_bar_next(uint16_t current, bool increment)
{
    static const uint16_t values[] = {100u, 400u, 800u, 1200u, 1600u};
    size_t index = 0u;

    for (index = 0u; index < (sizeof(values) / sizeof(values[0])); ++index) {
        if (values[index] == current) {
            break;
        }
    }

    if (index >= (sizeof(values) / sizeof(values[0]))) {
        index = 0u;
    }

    if (increment) {
        if (index + 1u < (sizeof(values) / sizeof(values[0]))) {
            ++index;
        }
    } else if (index > 0u) {
        --index;
    }

    return values[index];
}

static void atu_menu_prepare_result(atu_menu_result_t *result, const atu_menu_state_t *state, atu_menu_action_t action)
{
    result->action = action;
    result->item = state->selected_item;
    result->is_open = state->open;
    result->awaiting_confirmation = state->awaiting_reset_confirmation;
    result->settings = state->draft_settings;
}

void atu_menu_init(atu_menu_state_t *state, const atu_settings_t *settings)
{
    if (state == NULL) {
        return;
    }

    memset(state, 0, sizeof(*state));
    state->selected_item = ATU_MENU_ITEM_MINIMUM_SWR;
    if (settings != NULL) {
        state->draft_settings = *settings;
    } else {
        atu_settings_set_defaults(&state->draft_settings);
    }
}

atu_menu_result_t atu_menu_handle_event(atu_menu_state_t *state,
                                        const atu_settings_t *persisted_settings,
                                        atu_button_event_t event)
{
    atu_menu_result_t result;
    bool increment = true;

    memset(&result, 0, sizeof(result));
    if (state == NULL) {
        return result;
    }

    if ((event.button == ATU_BUTTON_MENU) && (event.type == ATU_BUTTON_EVENT_PRESSED)) {
        if (!state->open) {
            state->open = true;
            state->awaiting_reset_confirmation = false;
            if (persisted_settings != NULL) {
                state->draft_settings = *persisted_settings;
                atu_settings_sanitize(&state->draft_settings);
            }
            atu_menu_prepare_result(&result, state, ATU_MENU_ACTION_OPENED);
            return result;
        }

        state->open = false;
        state->awaiting_reset_confirmation = false;
        atu_settings_sanitize(&state->draft_settings);
        atu_menu_prepare_result(&result, state, ATU_MENU_ACTION_SAVE_AND_EXIT);
        return result;
    }

    if (!state->open) {
        atu_menu_prepare_result(&result, state, ATU_MENU_ACTION_NONE);
        return result;
    }

    if (atu_menu_is_navigation_event(event)) {
        state->awaiting_reset_confirmation = false;
        if (event.button == ATU_BUTTON_C_PLUS) {
            state->selected_item = (atu_menu_item_t)((state->selected_item + 1u) % ATU_MENU_ITEM_COUNT);
        } else if (state->selected_item == ATU_MENU_ITEM_MINIMUM_SWR) {
            state->selected_item = (atu_menu_item_t)(ATU_MENU_ITEM_COUNT - 1u);
        } else {
            state->selected_item = (atu_menu_item_t)(state->selected_item - 1u);
        }
        atu_menu_prepare_result(&result, state, ATU_MENU_ACTION_NONE);
        return result;
    }

    if (atu_menu_is_increment_event(event)) {
        increment = (event.button == ATU_BUTTON_L_PLUS);
        state->awaiting_reset_confirmation = false;

        switch (state->selected_item) {
        case ATU_MENU_ITEM_MINIMUM_SWR:
            if (increment && (state->draft_settings.minimum_swr_tenths < 20u)) {
                state->draft_settings.minimum_swr_tenths++;
            } else if (!increment && (state->draft_settings.minimum_swr_tenths > 10u)) {
                state->draft_settings.minimum_swr_tenths--;
            }
            break;

        case ATU_MENU_ITEM_DELAY_RELAY:
            if (increment && (state->draft_settings.relay_delay_ms < RELAY_DELAY_MAX)) {
                state->draft_settings.relay_delay_ms++;
            } else if (!increment && (state->draft_settings.relay_delay_ms > RELAY_DELAY_MIN)) {
                state->draft_settings.relay_delay_ms--;
            }
            break;

        case ATU_MENU_ITEM_BRIGHTNESS:
            if (increment && (state->draft_settings.brightness_percent < 100u)) {
                state->draft_settings.brightness_percent++;
            } else if (!increment && (state->draft_settings.brightness_percent > 0u)) {
                state->draft_settings.brightness_percent--;
            }
            break;

        case ATU_MENU_ITEM_MEMORY_BANK:
            state->draft_settings.selected_bank = increment ? 2u : 1u;
            atu_menu_prepare_result(&result, state, ATU_MENU_ACTION_BANK_CHANGED);
            return result;

        case ATU_MENU_ITEM_POWER_MAX:
            if (increment && (state->draft_settings.power_max_watts < 100u)) {
                state->draft_settings.power_max_watts++;
            } else if (!increment && (state->draft_settings.power_max_watts > 0u)) {
                state->draft_settings.power_max_watts--;
            }
            break;

        case ATU_MENU_ITEM_SCALE_BAR:
            state->draft_settings.scale_bar_watts = atu_menu_scale_bar_next(state->draft_settings.scale_bar_watts, increment);
            break;

        default:
            break;
        }

        atu_menu_prepare_result(&result, state, ATU_MENU_ACTION_NONE);
        return result;
    }

    if ((event.button == ATU_BUTTON_CIN_OUT) && (event.type == ATU_BUTTON_EVENT_PRESSED)) {
        switch (state->selected_item) {
        case ATU_MENU_ITEM_ADC_SWAP:
            state->draft_settings.adc_swap = (state->draft_settings.adc_swap == 0u) ? 1u : 0u;
            break;

        case ATU_MENU_ITEM_RESET_ACTIVE_BANK:
            if (!state->awaiting_reset_confirmation) {
                state->awaiting_reset_confirmation = true;
                atu_menu_prepare_result(&result, state, ATU_MENU_ACTION_RESET_REQUESTED);
                return result;
            }

            state->awaiting_reset_confirmation = false;
            atu_menu_prepare_result(&result, state, ATU_MENU_ACTION_RESET_CONFIRMED);
            return result;

        case ATU_MENU_ITEM_DEBUG_MODE:
            state->draft_settings.debug_mode = (state->draft_settings.debug_mode == 0u) ? 1u : 0u;
            break;

        default:
            break;
        }
    }

    atu_menu_prepare_result(&result, state, ATU_MENU_ACTION_NONE);
    return result;
}

const char *atu_menu_item_name(atu_menu_item_t item)
{
    static const char *const names[ATU_MENU_ITEM_COUNT] = {
        "Minimum SWR",
        "Delay Relay",
        "Brightness",
        "Memory Bank",
        "Power Max",
        "ADC Swap",
        "Scale Bar",
        "All Reset LC Data",
        "About",
        "Debug Mode"
    };

    return (item < ATU_MENU_ITEM_COUNT) ? names[item] : "";
}

void atu_menu_format_value(const atu_menu_state_t *state, atu_menu_item_t item, char *buffer, uint32_t buffer_size)
{
    const atu_settings_t *settings = NULL;

    if ((state == NULL) || (buffer == NULL) || (buffer_size == 0u)) {
        return;
    }

    settings = &state->draft_settings;

    switch (item) {
    case ATU_MENU_ITEM_MINIMUM_SWR:
        (void)snprintf(buffer, (size_t)buffer_size, "%u.%u",
                       settings->minimum_swr_tenths / 10u,
                       settings->minimum_swr_tenths % 10u);
        break;

    case ATU_MENU_ITEM_DELAY_RELAY:
        (void)snprintf(buffer, (size_t)buffer_size, "%u ms", settings->relay_delay_ms);
        break;

    case ATU_MENU_ITEM_BRIGHTNESS:
        (void)snprintf(buffer, (size_t)buffer_size, "%u%%", settings->brightness_percent);
        break;

    case ATU_MENU_ITEM_MEMORY_BANK:
        (void)snprintf(buffer, (size_t)buffer_size, "%u", settings->selected_bank);
        break;

    case ATU_MENU_ITEM_POWER_MAX:
        (void)snprintf(buffer, (size_t)buffer_size, "%u W", settings->power_max_watts);
        break;

    case ATU_MENU_ITEM_ADC_SWAP:
        (void)snprintf(buffer, (size_t)buffer_size, "%s", (settings->adc_swap != 0u) ? "ON" : "OFF");
        break;

    case ATU_MENU_ITEM_SCALE_BAR:
        (void)snprintf(buffer, (size_t)buffer_size, "%u W", settings->scale_bar_watts);
        break;

    case ATU_MENU_ITEM_RESET_ACTIVE_BANK:
        (void)snprintf(buffer, (size_t)buffer_size, "%s", state->awaiting_reset_confirmation ? "Confirm?" : "Press Cin/out");
        break;

    case ATU_MENU_ITEM_ABOUT:
        (void)snprintf(buffer, (size_t)buffer_size, "%s %s %s",
                       ATU_FIRMWARE_VERSION,
                       ATU_FIRMWARE_BUILD_DATE,
                       ATU_FIRMWARE_AUTHOR);
        break;

    case ATU_MENU_ITEM_DEBUG_MODE:
        (void)snprintf(buffer, (size_t)buffer_size, "%s", (settings->debug_mode != 0u) ? "ON" : "OFF");
        break;

    default:
        buffer[0] = '\0';
        break;
    }
}
