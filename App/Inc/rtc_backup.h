#ifndef ATU_RTC_BACKUP_H
#define ATU_RTC_BACKUP_H

#include <stdbool.h>
#include <stdint.h>

#include "config.h"

/**
 * @file rtc_backup.h
 * @brief Backup-domain helpers for restoring last frequency and relay state
 */

#define ATU_RTC_BACKUP_MAGIC      0xA71A4B50u
#define ATU_RTC_BACKUP_VERSION    0x0001u
#define ATU_RTC_BACKUP_REG_COUNT  4u

typedef struct {
    uint32_t frequency_hz;
    uint8_t cap_mask;
    uint8_t ind_mask;
    uint8_t flags;           /* bit0 topology, bit1 bypass */
    uint8_t bank;
} atu_rtc_backup_state_t;

typedef struct {
    void *context;
    uint32_t register_count;
    uint32_t (*read)(void *context, uint32_t index);
    bool (*write)(void *context, uint32_t index, uint32_t value);
} atu_rtc_backup_ops_t;

uint16_t atu_rtc_backup_checksum(const atu_rtc_backup_state_t *state);
bool atu_rtc_backup_pack(const atu_rtc_backup_state_t *state, uint32_t registers[ATU_RTC_BACKUP_REG_COUNT]);
bool atu_rtc_backup_unpack(const uint32_t registers[ATU_RTC_BACKUP_REG_COUNT], atu_rtc_backup_state_t *state);
bool atu_rtc_backup_store(const atu_rtc_backup_ops_t *ops, const atu_rtc_backup_state_t *state);
bool atu_rtc_backup_load(const atu_rtc_backup_ops_t *ops, atu_rtc_backup_state_t *state);

#endif /* ATU_RTC_BACKUP_H */
