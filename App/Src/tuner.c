#include "tuner.h"

#include <float.h>
#include <string.h>

static uint8_t atu_tuner_clip_mask(int16_t value)
{
    if (value < 0) {
        return 0u;
    }

    if (value > 255) {
        return 255u;
    }

    return (uint8_t)value;
}

static void atu_tuner_restore_pa_output(atu_tuner_t *tuner)
{
    if ((tuner->ops.set_pa_output != NULL) && tuner->active) {
        tuner->ops.set_pa_output(tuner->ops.context, tuner->request.pa_output_enabled);
    }
}

static void atu_tuner_apply_final_network(atu_tuner_t *tuner)
{
    if (tuner->ops.apply_network == NULL) {
        return;
    }

    if (tuner->best_swr < FLT_MAX) {
        (void)tuner->ops.apply_network(tuner->ops.context,
                                       tuner->best_cap_mask,
                                       tuner->best_ind_mask,
                                       tuner->best_topology,
                                       tuner->request.bypass_enabled);
    } else {
        (void)tuner->ops.apply_network(tuner->ops.context,
                                       tuner->request.initial_cap_mask,
                                       tuner->request.initial_ind_mask,
                                       tuner->request.initial_topology,
                                       tuner->request.bypass_enabled);
    }
}

static void atu_tuner_finish(atu_tuner_t *tuner, atu_tuner_status_t status)
{
    tuner->status = status;
    tuner->result.status = status;
    tuner->result.cap_mask = (tuner->best_swr < FLT_MAX) ? tuner->best_cap_mask : tuner->request.initial_cap_mask;
    tuner->result.ind_mask = (tuner->best_swr < FLT_MAX) ? tuner->best_ind_mask : tuner->request.initial_ind_mask;
    tuner->result.topology = (tuner->best_swr < FLT_MAX) ? tuner->best_topology : tuner->request.initial_topology;
    tuner->result.best_swr = (tuner->best_swr < FLT_MAX) ? tuner->best_swr : 0.0f;

    atu_tuner_apply_final_network(tuner);
    atu_tuner_restore_pa_output(tuner);

    tuner->phase = ATU_TUNER_PHASE_COMPLETE;
    tuner->active = false;
}

static bool atu_tuner_schedule_candidate(atu_tuner_t *tuner,
                                         uint32_t now_ms,
                                         uint8_t cap_mask,
                                         uint8_t ind_mask,
                                         uint8_t topology)
{
    if (tuner->ops.apply_network == NULL) {
        return false;
    }

    if (!tuner->ops.apply_network(tuner->ops.context, cap_mask, ind_mask, topology, tuner->request.bypass_enabled)) {
        return false;
    }

    tuner->current_cap_mask = cap_mask;
    tuner->current_ind_mask = ind_mask;
    tuner->current_topology = topology;
    tuner->ready_at_ms = now_ms + tuner->settings.relay_delay_ms;
    return true;
}

static bool atu_tuner_capture_measurement(atu_tuner_t *tuner,
                                          atu_tuner_measurement_t *measurement,
                                          atu_tuner_status_t *stop_status)
{
    if ((measurement == NULL) || (stop_status == NULL) || (tuner->ops.measure == NULL)) {
        return false;
    }

    if (!tuner->ops.measure(tuner->ops.context, measurement) || !measurement->valid) {
        *stop_status = ATU_TUNER_STATUS_FAILED;
        return false;
    }

    tuner->current_swr = measurement->swr;
    if ((float)tuner->settings.power_max_watts < measurement->forward_power_watts) {
        *stop_status = ATU_TUNER_STATUS_POWER_LIMIT;
        return false;
    }

    if (measurement->swr < tuner->best_swr) {
        tuner->best_swr = measurement->swr;
        tuner->best_cap_mask = tuner->current_cap_mask;
        tuner->best_ind_mask = tuner->current_ind_mask;
        tuner->best_topology = tuner->current_topology;
    }

    if (measurement->swr <= tuner->settings.minimum_swr) {
        *stop_status = ATU_TUNER_STATUS_SUCCESS;
        return false;
    }

    *stop_status = ATU_TUNER_STATUS_RUNNING;
    return true;
}

static bool atu_tuner_prepare_fine_search(atu_tuner_t *tuner, uint32_t now_ms, int16_t delta)
{
    tuner->phase = ATU_TUNER_PHASE_FINE_SEARCH;
    tuner->fine_cap_start = (int16_t)tuner->coarse_cap_mask - delta;
    tuner->fine_cap_end = (int16_t)tuner->coarse_cap_mask + delta;
    tuner->fine_ind_start = (int16_t)tuner->coarse_ind_mask - delta;
    tuner->fine_ind_end = (int16_t)tuner->coarse_ind_mask + delta;
    tuner->fine_cap_cursor = tuner->fine_cap_start;
    tuner->fine_ind_cursor = tuner->fine_ind_start;

    return atu_tuner_schedule_candidate(tuner,
                                        now_ms,
                                        atu_tuner_clip_mask(tuner->fine_cap_cursor),
                                        atu_tuner_clip_mask(tuner->fine_ind_cursor),
                                        tuner->current_topology);
}

static bool atu_tuner_advance_fine_search(atu_tuner_t *tuner, uint32_t now_ms)
{
    if (tuner->fine_ind_cursor < tuner->fine_ind_end) {
        tuner->fine_ind_cursor++;
    } else {
        tuner->fine_ind_cursor = tuner->fine_ind_start;
        tuner->fine_cap_cursor++;
    }

    if (tuner->fine_cap_cursor > tuner->fine_cap_end) {
        atu_tuner_finish(tuner, (tuner->best_swr < FLT_MAX) ? ATU_TUNER_STATUS_SUCCESS : ATU_TUNER_STATUS_FAILED);
        return false;
    }

    return atu_tuner_schedule_candidate(tuner,
                                        now_ms,
                                        atu_tuner_clip_mask(tuner->fine_cap_cursor),
                                        atu_tuner_clip_mask(tuner->fine_ind_cursor),
                                        tuner->current_topology);
}

void atu_tuner_init(atu_tuner_t *tuner,
                    const atu_tuner_platform_ops_t *ops,
                    const atu_tuner_settings_t *settings)
{
    if ((tuner == NULL) || (ops == NULL) || (settings == NULL)) {
        return;
    }

    memset(tuner, 0, sizeof(*tuner));
    tuner->ops = *ops;
    tuner->settings = *settings;
    tuner->phase = ATU_TUNER_PHASE_IDLE;
    tuner->status = ATU_TUNER_STATUS_IDLE;
    tuner->best_swr = FLT_MAX;
}

atu_tuner_status_t atu_tuner_start(atu_tuner_t *tuner,
                                   const atu_tuner_start_request_t *request,
                                   uint32_t now_ms)
{
    if ((tuner == NULL) || (request == NULL) || (tuner->ops.apply_network == NULL) || (tuner->ops.measure == NULL)) {
        return ATU_TUNER_STATUS_FAILED;
    }

    if (tuner->active) {
        return tuner->status;
    }

    memset(&tuner->result, 0, sizeof(tuner->result));
    tuner->request = *request;
    tuner->status = ATU_TUNER_STATUS_RUNNING;
    tuner->phase = request->has_preset ? ATU_TUNER_PHASE_PRESET_BASELINE : ATU_TUNER_PHASE_TOPOLOGY_IN;
    tuner->started_at_ms = now_ms;
    tuner->active = true;
    tuner->best_swr = FLT_MAX;
    tuner->search_reference_swr = FLT_MAX;
    tuner->current_swr = 0.0f;
    tuner->result.used_preset_path = request->has_preset;

    if (tuner->ops.set_pa_output != NULL) {
        tuner->ops.set_pa_output(tuner->ops.context, false);
    }

    if (request->has_preset) {
        tuner->coarse_cap_mask = request->preset.cap_mask;
        tuner->coarse_ind_mask = request->preset.ind_mask;
        if (!atu_tuner_schedule_candidate(tuner,
                                          now_ms,
                                          request->preset.cap_mask,
                                          request->preset.ind_mask,
                                          (uint8_t)(request->preset.flags & 0x01u))) {
            atu_tuner_finish(tuner, ATU_TUNER_STATUS_FAILED);
        }
    } else if (!atu_tuner_schedule_candidate(tuner, now_ms, TEST_CAPACITOR_MASK, 0u, TOPOLOGY_IN)) {
        atu_tuner_finish(tuner, ATU_TUNER_STATUS_FAILED);
    }

    return tuner->status;
}

atu_tuner_status_t atu_tuner_process(atu_tuner_t *tuner, uint32_t now_ms)
{
    atu_tuner_measurement_t measurement;
    atu_tuner_status_t stop_status = ATU_TUNER_STATUS_RUNNING;
    uint8_t candidate_mask = 0u;
    bool keep_running = true;

    if ((tuner == NULL) || !tuner->active) {
        return (tuner != NULL) ? tuner->status : ATU_TUNER_STATUS_IDLE;
    }

    if ((now_ms - tuner->started_at_ms) >= tuner->settings.timeout_ms) {
        atu_tuner_finish(tuner, ATU_TUNER_STATUS_TIMEOUT);
        return tuner->status;
    }

    if (now_ms < tuner->ready_at_ms) {
        return tuner->status;
    }

    memset(&measurement, 0, sizeof(measurement));
    keep_running = atu_tuner_capture_measurement(tuner, &measurement, &stop_status);
    if (!keep_running) {
        atu_tuner_finish(tuner, stop_status);
        return tuner->status;
    }

    switch (tuner->phase) {
    case ATU_TUNER_PHASE_PRESET_BASELINE:
        tuner->search_reference_swr = measurement.swr;
        tuner->current_topology = (uint8_t)(tuner->request.preset.flags & 0x01u);
        if (!atu_tuner_prepare_fine_search(tuner, now_ms, 2)) {
            atu_tuner_finish(tuner, ATU_TUNER_STATUS_FAILED);
        }
        break;

    case ATU_TUNER_PHASE_TOPOLOGY_IN:
        tuner->topology_in_swr = measurement.swr;
        if (!atu_tuner_schedule_candidate(tuner, now_ms, TEST_CAPACITOR_MASK, 0u, TOPOLOGY_OUT)) {
            atu_tuner_finish(tuner, ATU_TUNER_STATUS_FAILED);
            break;
        }
        tuner->phase = ATU_TUNER_PHASE_TOPOLOGY_OUT;
        break;

    case ATU_TUNER_PHASE_TOPOLOGY_OUT:
        tuner->topology_out_swr = measurement.swr;
        tuner->current_topology = (tuner->topology_out_swr < tuner->topology_in_swr) ? TOPOLOGY_OUT : TOPOLOGY_IN;
        tuner->search_reference_swr = (tuner->topology_out_swr < tuner->topology_in_swr) ? tuner->topology_out_swr : tuner->topology_in_swr;
        tuner->coarse_cap_mask = TEST_CAPACITOR_MASK;
        tuner->coarse_ind_mask = 0u;
        tuner->coarse_bit_index = 7;
        candidate_mask = (uint8_t)(tuner->coarse_ind_mask | (uint8_t)(1u << tuner->coarse_bit_index));
        if (!atu_tuner_schedule_candidate(tuner, now_ms, tuner->coarse_cap_mask, candidate_mask, tuner->current_topology)) {
            atu_tuner_finish(tuner, ATU_TUNER_STATUS_FAILED);
            break;
        }
        tuner->phase = ATU_TUNER_PHASE_COARSE_L;
        break;

    case ATU_TUNER_PHASE_COARSE_L:
        candidate_mask = (uint8_t)(tuner->coarse_ind_mask | (uint8_t)(1u << tuner->coarse_bit_index));
        if (measurement.swr < tuner->search_reference_swr) {
            tuner->coarse_ind_mask = candidate_mask;
            tuner->search_reference_swr = measurement.swr;
        }

        tuner->coarse_bit_index--;
        if (tuner->coarse_bit_index >= 0) {
            candidate_mask = (uint8_t)(tuner->coarse_ind_mask | (uint8_t)(1u << tuner->coarse_bit_index));
            if (!atu_tuner_schedule_candidate(tuner, now_ms, tuner->coarse_cap_mask, candidate_mask, tuner->current_topology)) {
                atu_tuner_finish(tuner, ATU_TUNER_STATUS_FAILED);
            }
        } else {
            tuner->coarse_bit_index = 7;
            candidate_mask = (uint8_t)(tuner->coarse_cap_mask | (uint8_t)(1u << tuner->coarse_bit_index));
            if (!atu_tuner_schedule_candidate(tuner, now_ms, candidate_mask, tuner->coarse_ind_mask, tuner->current_topology)) {
                atu_tuner_finish(tuner, ATU_TUNER_STATUS_FAILED);
                break;
            }
            tuner->phase = ATU_TUNER_PHASE_COARSE_C;
        }
        break;

    case ATU_TUNER_PHASE_COARSE_C:
        candidate_mask = (uint8_t)(tuner->coarse_cap_mask | (uint8_t)(1u << tuner->coarse_bit_index));
        if (measurement.swr < tuner->search_reference_swr) {
            tuner->coarse_cap_mask = candidate_mask;
            tuner->search_reference_swr = measurement.swr;
        }

        tuner->coarse_bit_index--;
        if (tuner->coarse_bit_index >= 0) {
            candidate_mask = (uint8_t)(tuner->coarse_cap_mask | (uint8_t)(1u << tuner->coarse_bit_index));
            if (!atu_tuner_schedule_candidate(tuner, now_ms, candidate_mask, tuner->coarse_ind_mask, tuner->current_topology)) {
                atu_tuner_finish(tuner, ATU_TUNER_STATUS_FAILED);
            }
        } else if (!atu_tuner_prepare_fine_search(tuner, now_ms, 3)) {
            atu_tuner_finish(tuner, ATU_TUNER_STATUS_FAILED);
        }
        break;

    case ATU_TUNER_PHASE_FINE_SEARCH:
        if (!atu_tuner_advance_fine_search(tuner, now_ms)) {
            return tuner->status;
        }
        break;

    default:
        atu_tuner_finish(tuner, ATU_TUNER_STATUS_FAILED);
        break;
    }

    return tuner->status;
}

void atu_tuner_abort(atu_tuner_t *tuner, uint32_t now_ms)
{
    (void)now_ms;

    if ((tuner == NULL) || !tuner->active) {
        return;
    }

    atu_tuner_finish(tuner, ATU_TUNER_STATUS_FAILED);
}

bool atu_tuner_is_active(const atu_tuner_t *tuner)
{
    return (tuner != NULL) && tuner->active;
}

void atu_tuner_get_progress(const atu_tuner_t *tuner, uint32_t now_ms, atu_tuner_progress_t *progress)
{
    if ((tuner == NULL) || (progress == NULL)) {
        return;
    }

    memset(progress, 0, sizeof(*progress));
    progress->phase = tuner->phase;
    progress->status = tuner->status;
    progress->elapsed_ms = now_ms - tuner->started_at_ms;
    progress->cap_mask = tuner->current_cap_mask;
    progress->ind_mask = tuner->current_ind_mask;
    progress->topology = tuner->current_topology;
    progress->current_swr = tuner->current_swr;
    progress->best_swr = (tuner->best_swr < FLT_MAX) ? tuner->best_swr : 0.0f;
}

void atu_tuner_export_preset(const atu_tuner_t *tuner, bool bypass_enabled, preset_t *preset)
{
    if ((tuner == NULL) || (preset == NULL)) {
        return;
    }

    preset->cap_mask = (tuner->best_swr < FLT_MAX) ? tuner->best_cap_mask : tuner->request.initial_cap_mask;
    preset->ind_mask = (tuner->best_swr < FLT_MAX) ? tuner->best_ind_mask : tuner->request.initial_ind_mask;
    preset->flags = (uint8_t)(((tuner->best_swr < FLT_MAX) ? tuner->best_topology : tuner->request.initial_topology) & 0x01u);
    if (bypass_enabled) {
        preset->flags |= 0x02u;
    }
}
