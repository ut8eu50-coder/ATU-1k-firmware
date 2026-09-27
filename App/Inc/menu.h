#ifndef ATU_MENU_H
#define ATU_MENU_H

#include <stdbool.h>
#include <stdint.h>

#include "button.h"
#include "storage.h"

/**
 * @file menu.h
 * @brief Event-driven settings menu model
 */

typedef enum {
    ATU_MENU_ITEM_MINIMUM_SWR = 0,
    ATU_MENU_ITEM_DELAY_RELAY,
    ATU_MENU_ITEM_BRIGHTNESS,
    ATU_MENU_ITEM_MEMORY_BANK,
    ATU_MENU_ITEM_POWER_MAX,
    ATU_MENU_ITEM_ADC_SWAP,
    ATU_MENU_ITEM_SCALE_BAR,
    ATU_MENU_ITEM_RESET_ACTIVE_BANK,
    ATU_MENU_ITEM_ABOUT,
    ATU_MENU_ITEM_DEBUG_MODE,
    ATU_MENU_ITEM_COUNT
} atu_menu_item_t;

typedef enum {
    ATU_MENU_ACTION_NONE = 0,
    ATU_MENU_ACTION_OPENED,
    ATU_MENU_ACTION_SAVE_AND_EXIT,
    ATU_MENU_ACTION_BANK_CHANGED,
    ATU_MENU_ACTION_RESET_REQUESTED,
    ATU_MENU_ACTION_RESET_CONFIRMED
} atu_menu_action_t;

typedef struct {
    atu_menu_action_t action;
    atu_menu_item_t item;
    bool is_open;
    bool awaiting_confirmation;
    atu_settings_t settings;
} atu_menu_result_t;

typedef struct {
    bool open;
    bool awaiting_reset_confirmation;
    atu_menu_item_t selected_item;
    atu_settings_t draft_settings;
} atu_menu_state_t;

void atu_menu_init(atu_menu_state_t *state, const atu_settings_t *settings);
atu_menu_result_t atu_menu_handle_event(atu_menu_state_t *state,
                                        const atu_settings_t *persisted_settings,
                                        atu_button_event_t event);

const char *atu_menu_item_name(atu_menu_item_t item);
void atu_menu_format_value(const atu_menu_state_t *state, atu_menu_item_t item, char *buffer, uint32_t buffer_size);

#endif /* ATU_MENU_H */
