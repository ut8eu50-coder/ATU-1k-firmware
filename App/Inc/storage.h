#ifndef ATU_STORAGE_H
#define ATU_STORAGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "config.h"

/**
 * @file storage.h
 * @brief FRAM-backed settings and preset layout helpers
 */

/* ========== FRAM Layout ========== */

#define ATU_FRAM_TOTAL_SIZE            8192u
#define ATU_FRAM_HEADER_BASE           0x0000u
#define ATU_FRAM_HEADER_SIZE           0x0040u
#define ATU_FRAM_BANK1_BASE            0x0100u
#define ATU_FRAM_PRESET_RECORD_SIZE    4u
#define ATU_FRAM_PRESET_SLOT_COUNT     176u
#define ATU_FRAM_PRESET_BANK_SIZE      (ATU_FRAM_PRESET_SLOT_COUNT * ATU_FRAM_PRESET_RECORD_SIZE)
#define ATU_FRAM_BANK2_BASE            (ATU_FRAM_BANK1_BASE + ATU_FRAM_PRESET_BANK_SIZE)
#define ATU_FRAM_FUTURE_BASE           (ATU_FRAM_BANK2_BASE + ATU_FRAM_PRESET_BANK_SIZE)

#define ATU_FRAM_HEADER_MAGIC          0x41545531u
#define ATU_FRAM_HEADER_VERSION        0x0001u
#define ATU_PRESET_VALID_MARKER        0xA5u

/* ========== Storage Types ========== */

typedef enum {
    ATU_STORAGE_OK = 0,
    ATU_STORAGE_BAD_ARGUMENT,
    ATU_STORAGE_IO_ERROR,
    ATU_STORAGE_UNAVAILABLE,
    ATU_STORAGE_NOT_FOUND,
    ATU_STORAGE_INVALID_DATA
} atu_storage_status_t;

typedef struct __attribute__((packed)) {
    uint8_t minimum_swr_tenths;    /* 10..20 => 1.0..2.0 */
    uint16_t relay_delay_ms;       /* 0..100 */
    uint8_t brightness_percent;    /* 0..100 */
    uint8_t selected_bank;         /* 1..2 */
    uint8_t power_max_watts;       /* 0..100 */
    uint8_t adc_swap;              /* 0/1 */
    uint16_t scale_bar_watts;      /* 100/400/800/1200/1600 */
    uint8_t debug_mode;            /* 0/1 */
    uint8_t reserved[4];
} atu_settings_t;

typedef struct {
    uint8_t band_id;
    uint16_t band_slot;
    uint16_t global_slot;
    uint16_t fram_address;
} atu_preset_location_t;

typedef struct {
    void *context;
    bool (*read)(void *context, uint16_t address, void *buffer, size_t length);
    bool (*write)(void *context, uint16_t address, const void *buffer, size_t length);
} atu_fram_ops_t;

/* ========== Settings Helpers ========== */

void atu_settings_set_defaults(atu_settings_t *settings);
void atu_settings_sanitize(atu_settings_t *settings);
float atu_settings_minimum_swr(const atu_settings_t *settings);

/* ========== Pure Mapping Helpers ========== */

uint16_t atu_storage_checksum16(const void *data, size_t length);
bool atu_storage_find_band(uint32_t frequency_hz, atu_preset_location_t *location);
bool atu_storage_preset_address(uint8_t bank, uint16_t global_slot, uint16_t *address);

/* ========== FRAM Helpers ========== */

atu_storage_status_t atu_storage_load_settings(const atu_fram_ops_t *ops, atu_settings_t *settings);
atu_storage_status_t atu_storage_save_settings(const atu_fram_ops_t *ops, const atu_settings_t *settings);

atu_storage_status_t atu_storage_load_preset(const atu_fram_ops_t *ops,
                                             uint8_t bank,
                                             uint32_t frequency_hz,
                                             preset_t *preset);
atu_storage_status_t atu_storage_save_preset(const atu_fram_ops_t *ops,
                                             uint8_t bank,
                                             uint32_t frequency_hz,
                                             const preset_t *preset);
atu_storage_status_t atu_storage_clear_bank(const atu_fram_ops_t *ops, uint8_t bank);

#endif /* ATU_STORAGE_H */
