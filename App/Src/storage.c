#include "storage.h"

#include <string.h>

/* ========== Internal Types ========== */

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint16_t version;
    uint16_t checksum;
    atu_settings_t settings;
} atu_storage_header_t;

typedef struct __attribute__((packed)) {
    uint8_t cap_mask;
    uint8_t ind_mask;
    uint8_t flags;
    uint8_t marker;
} atu_storage_preset_record_t;

/* ========== Internal Helpers ========== */

static bool atu_storage_bank_valid(uint8_t bank)
{
    return (bank == 1u) || (bank == 2u);
}

static uint16_t atu_storage_bank_base(uint8_t bank)
{
    return (bank == 1u) ? ATU_FRAM_BANK1_BASE : ATU_FRAM_BANK2_BASE;
}

static bool atu_storage_ops_valid(const atu_fram_ops_t *ops)
{
    return (ops != NULL) && (ops->read != NULL) && (ops->write != NULL);
}

static uint8_t atu_storage_scale_bar_default(void)
{
    return 0u;
}

static const uint16_t *atu_storage_scale_bar_options(size_t *count)
{
    static const uint16_t options[] = {100u, 400u, 800u, 1200u, 1600u};

    if (count != NULL) {
        *count = sizeof(options) / sizeof(options[0]);
    }

    return options;
}

static uint16_t atu_storage_normalize_scale_bar(uint16_t value)
{
    size_t option_count = 0u;
    const uint16_t *options = atu_storage_scale_bar_options(&option_count);
    size_t index = 0u;

    for (index = 0u; index < option_count; ++index) {
        if (options[index] == value) {
            return value;
        }
    }

    return options[atu_storage_scale_bar_default()];
}

/* ========== Settings Helpers ========== */

void atu_settings_set_defaults(atu_settings_t *settings)
{
    if (settings == NULL) {
        return;
    }

    memset(settings, 0, sizeof(*settings));
    settings->minimum_swr_tenths = 15u;
    settings->relay_delay_ms = RELAY_DELAY_DEFAULT;
    settings->brightness_percent = 100u;
    settings->selected_bank = 1u;
    settings->power_max_watts = 100u;
    settings->adc_swap = 0u;
    settings->scale_bar_watts = 100u;
    settings->debug_mode = 0u;
}

void atu_settings_sanitize(atu_settings_t *settings)
{
    if (settings == NULL) {
        return;
    }

    if (settings->minimum_swr_tenths < 10u) {
        settings->minimum_swr_tenths = 10u;
    } else if (settings->minimum_swr_tenths > 20u) {
        settings->minimum_swr_tenths = 20u;
    }

    if (settings->relay_delay_ms > RELAY_DELAY_MAX) {
        settings->relay_delay_ms = RELAY_DELAY_MAX;
    }

    if (settings->brightness_percent > 100u) {
        settings->brightness_percent = 100u;
    }

    if (!atu_storage_bank_valid(settings->selected_bank)) {
        settings->selected_bank = 1u;
    }

    if (settings->power_max_watts > 100u) {
        settings->power_max_watts = 100u;
    }

    settings->adc_swap = (settings->adc_swap != 0u) ? 1u : 0u;
    settings->debug_mode = (settings->debug_mode != 0u) ? 1u : 0u;
    settings->scale_bar_watts = atu_storage_normalize_scale_bar(settings->scale_bar_watts);
}

float atu_settings_minimum_swr(const atu_settings_t *settings)
{
    if (settings == NULL) {
        return SWR_THRESHOLD_DEFAULT;
    }

    return ((float)settings->minimum_swr_tenths) / 10.0f;
}

/* ========== Pure Helpers ========== */

uint16_t atu_storage_checksum16(const void *data, size_t length)
{
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t sum = 0u;
    size_t index = 0u;

    if (bytes == NULL) {
        return 0u;
    }

    for (index = 0u; index < length; ++index) {
        sum = (sum + bytes[index]) & 0xFFFFu;
    }

    return (uint16_t)sum;
}

bool atu_storage_find_band(uint32_t frequency_hz, atu_preset_location_t *location)
{
    uint16_t slot_base = 0u;
    uint8_t band_index = 0u;

    for (band_index = 0u; band_index < NUM_BANDS; ++band_index) {
        uint32_t min_khz = bands[band_index].freq_min_khz;
        uint32_t max_khz = bands[band_index].freq_max_khz;
        uint32_t step_khz = bands[band_index].step_khz;
        uint32_t slot_count = ((max_khz - min_khz) / step_khz) + 1u;
        uint32_t min_hz = min_khz * 1000u;
        uint32_t max_hz = (max_khz * 1000u) + 999u;
        uint32_t step_hz = step_khz * 1000u;

        if ((frequency_hz >= min_hz) && (frequency_hz <= max_hz)) {
            uint32_t rounded_offset_hz = (frequency_hz - min_hz) + (step_hz / 2u);
            uint16_t band_slot = (uint16_t)(rounded_offset_hz / step_hz);

            if ((uint32_t)band_slot >= slot_count) {
                band_slot = (uint16_t)(slot_count - 1u);
            }

            if (location != NULL) {
                location->band_id = band_index;
                location->band_slot = band_slot;
                location->global_slot = (uint16_t)(slot_base + band_slot);
                location->fram_address = 0u;
            }

            return true;
        }

        slot_base = (uint16_t)(slot_base + (uint16_t)slot_count);
    }

    return false;
}

bool atu_storage_preset_address(uint8_t bank, uint16_t global_slot, uint16_t *address)
{
    uint32_t computed = 0u;

    if ((!atu_storage_bank_valid(bank)) || (global_slot >= ATU_FRAM_PRESET_SLOT_COUNT) || (address == NULL)) {
        return false;
    }

    computed = (uint32_t)atu_storage_bank_base(bank) + ((uint32_t)global_slot * ATU_FRAM_PRESET_RECORD_SIZE);
    if ((computed + ATU_FRAM_PRESET_RECORD_SIZE) > ATU_FRAM_TOTAL_SIZE) {
        return false;
    }

    *address = (uint16_t)computed;
    return true;
}

/* ========== FRAM Helpers ========== */

atu_storage_status_t atu_storage_load_settings(const atu_fram_ops_t *ops, atu_settings_t *settings)
{
    atu_storage_header_t header;

    if ((settings == NULL) || !atu_storage_ops_valid(ops)) {
        return ATU_STORAGE_BAD_ARGUMENT;
    }

    if (!ops->read(ops->context, ATU_FRAM_HEADER_BASE, &header, sizeof(header))) {
        atu_settings_set_defaults(settings);
        return ATU_STORAGE_IO_ERROR;
    }

    if ((header.magic != ATU_FRAM_HEADER_MAGIC) ||
        (header.version != ATU_FRAM_HEADER_VERSION) ||
        (header.checksum != atu_storage_checksum16(&header.settings, sizeof(header.settings)))) {
        atu_settings_set_defaults(settings);
        return ATU_STORAGE_INVALID_DATA;
    }

    *settings = header.settings;
    atu_settings_sanitize(settings);
    return ATU_STORAGE_OK;
}

atu_storage_status_t atu_storage_save_settings(const atu_fram_ops_t *ops, const atu_settings_t *settings)
{
    atu_storage_header_t header;
    atu_settings_t sanitized;

    if ((settings == NULL) || !atu_storage_ops_valid(ops)) {
        return ATU_STORAGE_BAD_ARGUMENT;
    }

    memset(&header, 0, sizeof(header));
    sanitized = *settings;
    atu_settings_sanitize(&sanitized);
    header.magic = ATU_FRAM_HEADER_MAGIC;
    header.version = ATU_FRAM_HEADER_VERSION;
    header.settings = sanitized;
    header.checksum = atu_storage_checksum16(&header.settings, sizeof(header.settings));

    if (!ops->write(ops->context, ATU_FRAM_HEADER_BASE, &header, sizeof(header))) {
        return ATU_STORAGE_IO_ERROR;
    }

    return ATU_STORAGE_OK;
}

atu_storage_status_t atu_storage_load_preset(const atu_fram_ops_t *ops,
                                             uint8_t bank,
                                             uint32_t frequency_hz,
                                             preset_t *preset)
{
    atu_preset_location_t location;
    atu_storage_preset_record_t record;
    uint16_t address = 0u;

    if ((preset == NULL) || !atu_storage_ops_valid(ops)) {
        return ATU_STORAGE_BAD_ARGUMENT;
    }

    if (!atu_storage_find_band(frequency_hz, &location)) {
        return ATU_STORAGE_NOT_FOUND;
    }

    if (!atu_storage_preset_address(bank, location.global_slot, &address)) {
        return ATU_STORAGE_BAD_ARGUMENT;
    }

    if (!ops->read(ops->context, address, &record, sizeof(record))) {
        return ATU_STORAGE_IO_ERROR;
    }

    if (record.marker != ATU_PRESET_VALID_MARKER) {
        return ATU_STORAGE_NOT_FOUND;
    }

    preset->cap_mask = record.cap_mask;
    preset->ind_mask = record.ind_mask;
    preset->flags = (uint8_t)(record.flags & 0x03u);
    return ATU_STORAGE_OK;
}

atu_storage_status_t atu_storage_save_preset(const atu_fram_ops_t *ops,
                                             uint8_t bank,
                                             uint32_t frequency_hz,
                                             const preset_t *preset)
{
    atu_preset_location_t location;
    atu_storage_preset_record_t record;
    uint16_t address = 0u;

    if ((preset == NULL) || !atu_storage_ops_valid(ops)) {
        return ATU_STORAGE_BAD_ARGUMENT;
    }

    if (!atu_storage_find_band(frequency_hz, &location)) {
        return ATU_STORAGE_NOT_FOUND;
    }

    if (!atu_storage_preset_address(bank, location.global_slot, &address)) {
        return ATU_STORAGE_BAD_ARGUMENT;
    }

    memset(&record, 0xFF, sizeof(record));
    record.cap_mask = preset->cap_mask;
    record.ind_mask = preset->ind_mask;
    record.flags = (uint8_t)(preset->flags & 0x03u);
    record.marker = ATU_PRESET_VALID_MARKER;

    if (!ops->write(ops->context, address, &record, sizeof(record))) {
        return ATU_STORAGE_IO_ERROR;
    }

    return ATU_STORAGE_OK;
}

atu_storage_status_t atu_storage_clear_bank(const atu_fram_ops_t *ops, uint8_t bank)
{
    uint8_t blank[ATU_FRAM_PRESET_RECORD_SIZE];
    uint16_t slot = 0u;
    uint16_t address = 0u;

    if (!atu_storage_ops_valid(ops) || !atu_storage_bank_valid(bank)) {
        return ATU_STORAGE_BAD_ARGUMENT;
    }

    memset(blank, 0xFF, sizeof(blank));

    for (slot = 0u; slot < ATU_FRAM_PRESET_SLOT_COUNT; ++slot) {
        if (!atu_storage_preset_address(bank, slot, &address)) {
            return ATU_STORAGE_BAD_ARGUMENT;
        }

        if (!ops->write(ops->context, address, blank, sizeof(blank))) {
            return ATU_STORAGE_IO_ERROR;
        }
    }

    return ATU_STORAGE_OK;
}
