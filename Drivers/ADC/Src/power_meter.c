#include "power_meter.h"

#include <math.h>
#include <stddef.h>

static float atu_power_meter_average_u16(const uint16_t *samples, size_t count)
{
    uint32_t sum = 0u;
    size_t index = 0u;

    if ((samples == NULL) || (count == 0u)) {
        return 0.0f;
    }

    for (index = 0u; index < count; ++index) {
        sum += samples[index];
    }

    return (float)sum / (float)count;
}

static float atu_power_meter_adc_to_detector_voltage(float adc_code, float vdda)
{
    return (adc_code / ADC_MAX_CODE) * vdda;
}

void atu_power_meter_init_config(atu_power_meter_config_t *config)
{
    if (config == NULL) {
        return;
    }

    config->adc_swap = false;
    config->scale_bar_watts = 100u;
    config->noise_floor_counts = MIN_ADC_FWD;
}

float atu_power_meter_calculate_swr(float forward_power_watts, float reverse_power_watts)
{
    float gamma = 0.0f;

    if (forward_power_watts <= 0.0f) {
        return 1.0f;
    }

    if (reverse_power_watts < 0.0f) {
        reverse_power_watts = 0.0f;
    }

    if (reverse_power_watts >= forward_power_watts) {
        return 99.0f;
    }

    gamma = sqrtf(reverse_power_watts / forward_power_watts);
    if (gamma >= 1.0f) {
        return 99.0f;
    }

    return (1.0f + gamma) / (1.0f - gamma);
}

atu_power_meter_status_t atu_power_meter_process_samples(const uint16_t *adc_buffer,
                                                         size_t sample_count,
                                                         const atu_power_meter_config_t *config,
                                                         atu_power_meter_reading_t *reading)
{
    float logical_fwd = 0.0f;
    float logical_rev = 0.0f;
    float vref = 0.0f;
    float full_scale_rf_voltage = sqrtf(1600.0f * 50.0f);
    float rf_forward_voltage = 0.0f;
    float rf_reverse_voltage = 0.0f;
    float raw_fwd = 0.0f;
    float raw_rev = 0.0f;
    const uint16_t *fwd_samples = adc_buffer;
    const uint16_t *rev_samples = adc_buffer + SAMPLES_PER_CHANNEL;
    const uint16_t *vref_samples = adc_buffer + (2u * SAMPLES_PER_CHANNEL);

    if ((adc_buffer == NULL) || (config == NULL) || (reading == NULL)) {
        return ATU_POWER_METER_BAD_ARGUMENT;
    }

    if (sample_count != ADC_BUF_SIZE) {
        return ATU_POWER_METER_BAD_ARGUMENT;
    }

    raw_fwd = atu_power_meter_average_u16(fwd_samples, SAMPLES_PER_CHANNEL);
    raw_rev = atu_power_meter_average_u16(rev_samples, SAMPLES_PER_CHANNEL);
    vref = atu_power_meter_average_u16(vref_samples, SAMPLES_PER_CHANNEL);

    if (config->adc_swap) {
        logical_fwd = raw_rev;
        logical_rev = raw_fwd;
    } else {
        logical_fwd = raw_fwd;
        logical_rev = raw_rev;
    }

    if (vref <= 0.0f) {
        return ATU_POWER_METER_INVALID;
    }

    reading->raw_fwd = (uint16_t)(logical_fwd + 0.5f);
    reading->raw_rev = (uint16_t)(logical_rev + 0.5f);
    reading->raw_vrefint = (uint16_t)(vref + 0.5f);
    reading->vdda_volts = ADC_VDDA_NOMINAL * ((float)(*ADC_VREFINT_CAL_ADDR) / vref);
    reading->detector_fwd_volts = atu_power_meter_adc_to_detector_voltage(logical_fwd, reading->vdda_volts);
    reading->detector_rev_volts = atu_power_meter_adc_to_detector_voltage(logical_rev, reading->vdda_volts);

    if ((logical_fwd >= ADC_MAX_CODE) || (logical_rev >= ADC_MAX_CODE)) {
        reading->forward_power_watts = 1600.0f;
        reading->reverse_power_watts = 1600.0f;
        reading->net_power_watts = 0.0f;
        reading->swr = 99.0f;
        return ATU_POWER_METER_OVERRANGE;
    }

    if (logical_fwd <= (float)config->noise_floor_counts) {
        reading->forward_power_watts = 0.0f;
        reading->reverse_power_watts = 0.0f;
        reading->net_power_watts = 0.0f;
        reading->swr = 1.0f;
        return ATU_POWER_METER_NO_FORWARD_POWER;
    }

    rf_forward_voltage = (reading->detector_fwd_volts / reading->vdda_volts) * full_scale_rf_voltage;
    rf_reverse_voltage = (reading->detector_rev_volts / reading->vdda_volts) * full_scale_rf_voltage;

    reading->forward_power_watts = (rf_forward_voltage * rf_forward_voltage) / 50.0f;
    reading->reverse_power_watts = (rf_reverse_voltage * rf_reverse_voltage) / 50.0f;

    if (reading->reverse_power_watts > reading->forward_power_watts) {
        reading->reverse_power_watts = reading->forward_power_watts;
    }

    reading->net_power_watts = reading->forward_power_watts - reading->reverse_power_watts;
    if (reading->net_power_watts < 0.0f) {
        reading->net_power_watts = 0.0f;
    }

    reading->swr = atu_power_meter_calculate_swr(reading->forward_power_watts, reading->reverse_power_watts);
    return ATU_POWER_METER_OK;
}
