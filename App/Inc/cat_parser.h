#ifndef ATU_CAT_PARSER_H
#define ATU_CAT_PARSER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "config.h"

/**
 * @file cat_parser.h
 * @brief Streaming CAT frequency parser
 */

#define ATU_CAT_TIMEOUT_MS     5000u
#define ATU_CAT_POLL_MS        100u

typedef enum {
    ATU_CAT_MODE_YAESU = CAT_MODE_YAESU,
    ATU_CAT_MODE_KENWOOD = CAT_MODE_KENWOOD,
    ATU_CAT_MODE_ICOM_CIV = CAT_MODE_ICOM_CIV,
    ATU_CAT_MODE_TCI = CAT_MODE_TCI_TCP
} atu_cat_mode_t;

typedef struct {
    atu_cat_mode_t mode;
    uint32_t frequency_hz;
    uint32_t last_update_ms;
    uint8_t ascii_buffer[24];
    size_t ascii_length;
    uint8_t civ_buffer[32];
    size_t civ_length;
} atu_cat_parser_t;

typedef struct {
    bool updated;
    bool frequency_valid;
    uint32_t frequency_hz;
} atu_cat_parse_result_t;

void atu_cat_parser_init(atu_cat_parser_t *parser, atu_cat_mode_t mode);
void atu_cat_parser_set_mode(atu_cat_parser_t *parser, atu_cat_mode_t mode);
size_t atu_cat_parser_poll_command(atu_cat_mode_t mode, uint8_t *buffer, size_t buffer_size);
bool atu_cat_parser_is_fresh(const atu_cat_parser_t *parser, uint32_t now_ms);
bool atu_cat_parser_process(atu_cat_parser_t *parser,
                            const uint8_t *data,
                            size_t length,
                            uint32_t now_ms,
                            atu_cat_parse_result_t *result);
bool atu_cat_parser_decode_civ_frequency(const uint8_t bcd0,
                                         const uint8_t bcd1,
                                         const uint8_t bcd2,
                                         const uint8_t bcd3,
                                         uint32_t *frequency_hz);

#endif /* ATU_CAT_PARSER_H */
