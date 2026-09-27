#ifndef ATU_TUNER_H
#define ATU_TUNER_H

#include <stdbool.h>
#include <stdint.h>

#include "config.h"

/**
 * @file tuner.h
 * @brief Non-blocking tuner state machine
 */

typedef enum {
    ATU_TUNER_STATUS_IDLE = 0,
    ATU_TUNER_STATUS_RUNNING,
    ATU_TUNER_STATUS_SUCCESS,
    ATU_TUNER_STATUS_TIMEOUT,
    ATU_TUNER_STATUS_POWER_LIMIT,
    ATU_TUNER_STATUS_FAILED
} atu_tuner_status_t;

typedef enum {
    ATU_TUNER_PHASE_IDLE = 0,
    ATU_TUNER_PHASE_PRESET_BASELINE,
    ATU_TUNER_PHASE_TOPOLOGY_IN,
    ATU_TUNER_PHASE_TOPOLOGY_OUT,
    ATU_TUNER_PHASE_COARSE_L,
    ATU_TUNER_PHASE_COARSE_C,
    ATU_TUNER_PHASE_FINE_SEARCH,
    ATU_TUNER_PHASE_COMPLETE
} atu_tuner_phase_t;

typedef struct {
    bool valid;
    float swr;
    float forward_power_watts;
} atu_tuner_measurement_t;

typedef struct {
    void *context;
    bool (*apply_network)(void *context, uint8_t cap_mask, uint8_t ind_mask, uint8_t topology, bool bypass_enabled);
    bool (*measure)(void *context, atu_tuner_measurement_t *measurement);
    void (*set_pa_output)(void *context, bool enabled);
} atu_tuner_platform_ops_t;

typedef struct {
    float minimum_swr;
    uint16_t power_max_watts;
    uint16_t relay_delay_ms;
    uint32_t timeout_ms;
} atu_tuner_settings_t;

typedef struct {
    uint32_t frequency_hz;
    bool has_preset;
    preset_t preset;
    uint8_t initial_cap_mask;
    uint8_t initial_ind_mask;
    uint8_t initial_topology;
    bool bypass_enabled;
    bool pa_output_enabled;
} atu_tuner_start_request_t;

typedef struct {
    atu_tuner_phase_t phase;
    atu_tuner_status_t status;
    uint32_t elapsed_ms;
    uint8_t cap_mask;
    uint8_t ind_mask;
    uint8_t topology;
    float current_swr;
    float best_swr;
} atu_tuner_progress_t;

typedef struct {
    atu_tuner_status_t status;
    uint8_t cap_mask;
    uint8_t ind_mask;
    uint8_t topology;
    float best_swr;
    bool used_preset_path;
} atu_tuner_result_t;

typedef struct {
    atu_tuner_platform_ops_t ops;
    atu_tuner_settings_t settings;
    atu_tuner_start_request_t request;
    atu_tuner_result_t result;
    atu_tuner_phase_t phase;
    atu_tuner_status_t status;
    uint32_t started_at_ms;
    uint32_t ready_at_ms;
    bool active;
    float current_swr;
    float best_swr;
    float search_reference_swr;
    float topology_in_swr;
    float topology_out_swr;
    uint8_t current_cap_mask;
    uint8_t current_ind_mask;
    uint8_t current_topology;
    bool effective_bypass_enabled;
    uint8_t best_cap_mask;
    uint8_t best_ind_mask;
    uint8_t best_topology;
    uint8_t coarse_cap_mask;
    uint8_t coarse_ind_mask;
    int8_t coarse_bit_index;
    int16_t fine_cap_start;
    int16_t fine_cap_end;
    int16_t fine_ind_start;
    int16_t fine_ind_end;
    int16_t fine_cap_cursor;
    int16_t fine_ind_cursor;
} atu_tuner_t;

void atu_tuner_init(atu_tuner_t *tuner,
                    const atu_tuner_platform_ops_t *ops,
                    const atu_tuner_settings_t *settings);
atu_tuner_status_t atu_tuner_start(atu_tuner_t *tuner,
                                   const atu_tuner_start_request_t *request,
                                   uint32_t now_ms);
atu_tuner_status_t atu_tuner_process(atu_tuner_t *tuner, uint32_t now_ms);
void atu_tuner_abort(atu_tuner_t *tuner, uint32_t now_ms);
bool atu_tuner_is_active(const atu_tuner_t *tuner);
void atu_tuner_get_progress(const atu_tuner_t *tuner, uint32_t now_ms, atu_tuner_progress_t *progress);
void atu_tuner_export_preset(const atu_tuner_t *tuner, bool bypass_enabled, preset_t *preset);

#endif /* ATU_TUNER_H */
