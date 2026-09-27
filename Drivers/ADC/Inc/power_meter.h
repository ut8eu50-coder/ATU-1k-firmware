#ifndef ATU_POWER_METER_H
#define ATU_POWER_METER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "config.h"

/**
 * @file power_meter.h
 * @brief ADC power, net power, and SWR processing helpers
 */

typedef enum {
    ATU_POWER_METER_OK = 0,
    ATU_POWER_METER_BAD_ARGUMENT,
    ATU_POWER_METER_NO_FORWARD_POWER,
    ATU_POWER_METER_OVERRANGE,
    ATU_POWER_METER_INVALID
} atu_power_meter_status_t;

typedef struct {
    bool adc_swap;
    uint16_t scale_bar_watts;
    uint16_t noise_floor_counts;
} atu_power_meter_config_t;

typedef struct {
    uint16_t raw_fwd;
    uint16_t raw_rev;
    uint16_t raw_vrefint;
    float vdda_volts;
    float detector_fwd_volts;
    float detector_rev_volts;
    float forward_power_watts;
    float reverse_power_watts;
    float net_power_watts;
    float swr;
} atu_power_meter_reading_t;

void atu_power_meter_init_config(atu_power_meter_config_t *config);
atu_power_meter_status_t atu_power_meter_process_samples(const uint16_t *adc_buffer,
                                                         size_t sample_count,
                                                         const atu_power_meter_config_t *config,
                                                         atu_power_meter_reading_t *reading);
float atu_power_meter_calculate_swr(float forward_power_watts, float reverse_power_watts);

#endif /* ATU_POWER_METER_H */
