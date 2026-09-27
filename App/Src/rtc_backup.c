#include "rtc_backup.h"

#include <string.h>

static bool atu_rtc_backup_bank_valid(uint8_t bank)
{
    return (bank == 1u) || (bank == 2u);
}

uint16_t atu_rtc_backup_checksum(const atu_rtc_backup_state_t *state)
{
    uint32_t sum = 0u;

    if (state == NULL) {
        return 0u;
    }

    sum += (state->frequency_hz & 0xFFFFu);
    sum += ((state->frequency_hz >> 16) & 0xFFFFu);
    sum += state->cap_mask;
    sum += state->ind_mask;
    sum += state->flags;
    sum += state->bank;

    return (uint16_t)(sum & 0xFFFFu);
}

bool atu_rtc_backup_pack(const atu_rtc_backup_state_t *state, uint32_t registers[ATU_RTC_BACKUP_REG_COUNT])
{
    uint16_t checksum = 0u;

    if ((state == NULL) || (registers == NULL) || !atu_rtc_backup_bank_valid(state->bank)) {
        return false;
    }

    checksum = atu_rtc_backup_checksum(state);
    registers[0] = ATU_RTC_BACKUP_MAGIC;
    registers[1] = state->frequency_hz;
    registers[2] = ((uint32_t)state->cap_mask) |
                   ((uint32_t)state->ind_mask << 8) |
                   ((uint32_t)(state->flags & 0x03u) << 16) |
                   ((uint32_t)state->bank << 24);
    registers[3] = (((uint32_t)ATU_RTC_BACKUP_VERSION & 0xFFFFu) << 16) | checksum;
    return true;
}

bool atu_rtc_backup_unpack(const uint32_t registers[ATU_RTC_BACKUP_REG_COUNT], atu_rtc_backup_state_t *state)
{
    uint16_t checksum = 0u;

    if ((registers == NULL) || (state == NULL)) {
        return false;
    }

    if ((registers[0] != ATU_RTC_BACKUP_MAGIC) ||
        (((registers[3] >> 16) & 0xFFFFu) != ATU_RTC_BACKUP_VERSION)) {
        return false;
    }

    memset(state, 0, sizeof(*state));
    state->frequency_hz = registers[1];
    state->cap_mask = (uint8_t)(registers[2] & 0xFFu);
    state->ind_mask = (uint8_t)((registers[2] >> 8) & 0xFFu);
    state->flags = (uint8_t)((registers[2] >> 16) & 0x03u);
    state->bank = (uint8_t)((registers[2] >> 24) & 0xFFu);

    if (!atu_rtc_backup_bank_valid(state->bank)) {
        return false;
    }

    checksum = atu_rtc_backup_checksum(state);
    return checksum == (uint16_t)(registers[3] & 0xFFFFu);
}

bool atu_rtc_backup_store(const atu_rtc_backup_ops_t *ops, const atu_rtc_backup_state_t *state)
{
    uint32_t registers[ATU_RTC_BACKUP_REG_COUNT];
    uint32_t index = 0u;

    if ((ops == NULL) || (state == NULL) || (ops->write == NULL) || (ops->register_count < ATU_RTC_BACKUP_REG_COUNT)) {
        return false;
    }

    if (!atu_rtc_backup_pack(state, registers)) {
        return false;
    }

    for (index = 0u; index < ATU_RTC_BACKUP_REG_COUNT; ++index) {
        if (!ops->write(ops->context, index, registers[index])) {
            return false;
        }
    }

    return true;
}

bool atu_rtc_backup_load(const atu_rtc_backup_ops_t *ops, atu_rtc_backup_state_t *state)
{
    uint32_t registers[ATU_RTC_BACKUP_REG_COUNT];
    uint32_t index = 0u;

    if ((ops == NULL) || (state == NULL) || (ops->read == NULL) || (ops->register_count < ATU_RTC_BACKUP_REG_COUNT)) {
        return false;
    }

    for (index = 0u; index < ATU_RTC_BACKUP_REG_COUNT; ++index) {
        registers[index] = ops->read(ops->context, index);
    }

    return atu_rtc_backup_unpack(registers, state);
}
