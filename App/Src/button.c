#include "button.h"

#include <string.h>

static bool atu_button_repeats(atu_button_id_t button)
{
    return (button == ATU_BUTTON_C_MINUS) ||
           (button == ATU_BUTTON_C_PLUS) ||
           (button == ATU_BUTTON_L_MINUS) ||
           (button == ATU_BUTTON_L_PLUS);
}

static void atu_button_push_event(atu_button_event_t *events,
                                  size_t max_events,
                                  size_t *event_count,
                                  atu_button_id_t button,
                                  atu_button_event_type_t type)
{
    if ((events == NULL) || (event_count == NULL) || (*event_count >= max_events)) {
        return;
    }

    events[*event_count].button = button;
    events[*event_count].type = type;
    *event_count += 1u;
}

void atu_button_init(atu_button_state_t *state)
{
    if (state == NULL) {
        return;
    }

    memset(state, 0, sizeof(*state));
    state->initialized = 1u;
}

size_t atu_button_poll(atu_button_state_t *state,
                       uint32_t pressed_mask,
                       uint32_t now_ms,
                       atu_button_event_t *events,
                       size_t max_events)
{
    size_t event_count = 0u;
    uint8_t index = 0u;

    if ((state == NULL) || (state->initialized == 0u)) {
        return 0u;
    }

    for (index = 0u; index < ATU_BUTTON_COUNT; ++index) {
        uint8_t sampled_pressed = ((pressed_mask & (1u << index)) != 0u) ? 1u : 0u;

        if (sampled_pressed == state->stable_state[index]) {
            state->sample_count[index] = 0u;
        } else {
            if (state->sample_count[index] < ATU_BUTTON_DEBOUNCE_SAMPLES) {
                state->sample_count[index]++;
            }

            if (state->sample_count[index] >= ATU_BUTTON_DEBOUNCE_SAMPLES) {
                state->stable_state[index] = sampled_pressed;
                state->sample_count[index] = 0u;

                if (sampled_pressed != 0u) {
                    state->pressed_since_ms[index] = now_ms;
                    state->last_repeat_ms[index] = now_ms + ATU_BUTTON_HOLD_DELAY_MS - ATU_BUTTON_REPEAT_MS;
                    state->long_fired[index] = 0u;
                    atu_button_push_event(events, max_events, &event_count, (atu_button_id_t)index, ATU_BUTTON_EVENT_PRESSED);
                } else {
                    if (((atu_button_id_t)index == ATU_BUTTON_TUNE) && (state->long_fired[index] == 0u)) {
                        atu_button_push_event(events, max_events, &event_count, (atu_button_id_t)index, ATU_BUTTON_EVENT_SHORT_PRESS);
                    } else {
                        atu_button_push_event(events, max_events, &event_count, (atu_button_id_t)index, ATU_BUTTON_EVENT_RELEASED);
                    }
                }
            }
        }

        if (state->stable_state[index] == 0u) {
            continue;
        }

        if (((atu_button_id_t)index == ATU_BUTTON_TUNE) &&
            (state->long_fired[index] == 0u) &&
            ((now_ms - state->pressed_since_ms[index]) >= ATU_BUTTON_HOLD_DELAY_MS)) {
            state->long_fired[index] = 1u;
            atu_button_push_event(events, max_events, &event_count, (atu_button_id_t)index, ATU_BUTTON_EVENT_LONG_PRESS);
        }

        if (atu_button_repeats((atu_button_id_t)index) &&
            ((now_ms - state->pressed_since_ms[index]) >= ATU_BUTTON_HOLD_DELAY_MS) &&
            ((now_ms - state->last_repeat_ms[index]) >= ATU_BUTTON_REPEAT_MS)) {
            state->last_repeat_ms[index] = now_ms;
            atu_button_push_event(events, max_events, &event_count, (atu_button_id_t)index, ATU_BUTTON_EVENT_REPEAT);
        }
    }

    return event_count;
}
