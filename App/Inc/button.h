#ifndef ATU_BUTTON_H
#define ATU_BUTTON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @file button.h
 * @brief Button debounce and repeat helpers
 */

#define ATU_BUTTON_COUNT               8u
#define ATU_BUTTON_DEBOUNCE_SAMPLES    7u
#define ATU_BUTTON_HOLD_DELAY_MS       500u
#define ATU_BUTTON_REPEAT_MS           500u

typedef enum {
    ATU_BUTTON_MENU = 0,
    ATU_BUTTON_BYPASS,
    ATU_BUTTON_TUNE,
    ATU_BUTTON_C_MINUS,
    ATU_BUTTON_L_MINUS,
    ATU_BUTTON_CIN_OUT,
    ATU_BUTTON_C_PLUS,
    ATU_BUTTON_L_PLUS
} atu_button_id_t;

typedef enum {
    ATU_BUTTON_EVENT_NONE = 0,
    ATU_BUTTON_EVENT_PRESSED,
    ATU_BUTTON_EVENT_RELEASED,
    ATU_BUTTON_EVENT_REPEAT,
    ATU_BUTTON_EVENT_SHORT_PRESS,
    ATU_BUTTON_EVENT_LONG_PRESS
} atu_button_event_type_t;

typedef struct {
    atu_button_id_t button;
    atu_button_event_type_t type;
} atu_button_event_t;

typedef struct {
    uint8_t stable_state[ATU_BUTTON_COUNT];
    uint8_t sample_count[ATU_BUTTON_COUNT];
    uint32_t pressed_since_ms[ATU_BUTTON_COUNT];
    uint32_t last_repeat_ms[ATU_BUTTON_COUNT];
    uint8_t long_fired[ATU_BUTTON_COUNT];
    uint8_t initialized;
} atu_button_state_t;

void atu_button_init(atu_button_state_t *state);
size_t atu_button_poll(atu_button_state_t *state,
                       uint32_t pressed_mask,
                       uint32_t now_ms,
                       atu_button_event_t *events,
                       size_t max_events);

#endif /* ATU_BUTTON_H */
