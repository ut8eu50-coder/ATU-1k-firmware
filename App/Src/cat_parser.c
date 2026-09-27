#include "cat_parser.h"

#include <ctype.h>
#include <string.h>

static bool atu_cat_parse_ascii_frequency(const uint8_t *frame, size_t length, uint32_t *frequency_hz)
{
    uint32_t value = 0u;
    size_t index = 0u;

    if ((frame == NULL) || (frequency_hz == NULL) || (length < 4u)) {
        return false;
    }

    if ((frame[0] != 'F') || (frame[1] != 'A') || (frame[length - 1u] != ';')) {
        return false;
    }

    for (index = 2u; index < (length - 1u); ++index) {
        if (!isdigit(frame[index])) {
            return false;
        }

        value = (value * 10u) + (uint32_t)(frame[index] - '0');
    }

    if ((length != 12u) && (length != 14u)) {
        return false;
    }

    *frequency_hz = value;
    return true;
}

static bool atu_cat_parse_civ_frame(const uint8_t *frame, size_t length, uint32_t *frequency_hz)
{
    if ((frame == NULL) || (frequency_hz == NULL) || (length < 11u)) {
        return false;
    }

    if ((frame[0] != 0xFEu) || (frame[1] != 0xFEu) || (frame[4] != 0x03u) || (frame[10] != 0xFDu)) {
        return false;
    }

    return atu_cat_parser_decode_civ_frequency(frame[9], frame[8], frame[7], frame[6], frequency_hz);
}

void atu_cat_parser_init(atu_cat_parser_t *parser, atu_cat_mode_t mode)
{
    if (parser == NULL) {
        return;
    }

    memset(parser, 0, sizeof(*parser));
    parser->mode = mode;
}

void atu_cat_parser_set_mode(atu_cat_parser_t *parser, atu_cat_mode_t mode)
{
    if (parser == NULL) {
        return;
    }

    parser->mode = mode;
    parser->ascii_length = 0u;
    parser->civ_length = 0u;
}

size_t atu_cat_parser_poll_command(atu_cat_mode_t mode, uint8_t *buffer, size_t buffer_size)
{
    static const uint8_t command[] = {'F', 'A', ';'};

    if ((buffer == NULL) || (buffer_size < sizeof(command))) {
        return 0u;
    }

    if ((mode != ATU_CAT_MODE_YAESU) && (mode != ATU_CAT_MODE_KENWOOD)) {
        return 0u;
    }

    memcpy(buffer, command, sizeof(command));
    return sizeof(command);
}

bool atu_cat_parser_is_fresh(const atu_cat_parser_t *parser, uint32_t now_ms)
{
    if ((parser == NULL) || (parser->last_update_ms == 0u)) {
        return false;
    }

    return (now_ms - parser->last_update_ms) <= ATU_CAT_TIMEOUT_MS;
}

bool atu_cat_parser_process(atu_cat_parser_t *parser,
                            const uint8_t *data,
                            size_t length,
                            uint32_t now_ms,
                            atu_cat_parse_result_t *result)
{
    size_t index = 0u;

    if ((parser == NULL) || (data == NULL) || (result == NULL)) {
        return false;
    }

    memset(result, 0, sizeof(*result));

    for (index = 0u; index < length; ++index) {
        if ((parser->mode == ATU_CAT_MODE_YAESU) || (parser->mode == ATU_CAT_MODE_KENWOOD)) {
            uint8_t byte = data[index];

            if (parser->ascii_length >= sizeof(parser->ascii_buffer)) {
                parser->ascii_length = 0u;
            }

            parser->ascii_buffer[parser->ascii_length++] = byte;
            if (byte == ';') {
                uint32_t frequency_hz = 0u;
                if (atu_cat_parse_ascii_frequency(parser->ascii_buffer, parser->ascii_length, &frequency_hz)) {
                    parser->frequency_hz = frequency_hz;
                    parser->last_update_ms = now_ms;
                    result->updated = true;
                    result->frequency_valid = true;
                    result->frequency_hz = frequency_hz;
                }
                parser->ascii_length = 0u;
            }
        } else if (parser->mode == ATU_CAT_MODE_ICOM_CIV) {
            uint8_t byte = data[index];

            if (parser->civ_length >= sizeof(parser->civ_buffer)) {
                memmove(parser->civ_buffer, &parser->civ_buffer[1], sizeof(parser->civ_buffer) - 1u);
                parser->civ_length = sizeof(parser->civ_buffer) - 1u;
            }

            parser->civ_buffer[parser->civ_length++] = byte;

            while (parser->civ_length >= 11u) {
                size_t offset = 0u;
                bool found = false;

                for (offset = 0u; offset <= (parser->civ_length - 11u); ++offset) {
                    uint32_t frequency_hz = 0u;

                    if ((parser->civ_buffer[offset] != 0xFEu) || (parser->civ_buffer[offset + 1u] != 0xFEu)) {
                        continue;
                    }

                    found = true;
                    if (atu_cat_parse_civ_frame(&parser->civ_buffer[offset], parser->civ_length - offset, &frequency_hz)) {
                        parser->frequency_hz = frequency_hz;
                        parser->last_update_ms = now_ms;
                        result->updated = true;
                        result->frequency_valid = true;
                        result->frequency_hz = frequency_hz;

                        memmove(parser->civ_buffer, &parser->civ_buffer[offset + 11u], parser->civ_length - (offset + 11u));
                        parser->civ_length -= (offset + 11u);
                        break;
                    }

                    memmove(parser->civ_buffer, &parser->civ_buffer[offset + 1u], parser->civ_length - (offset + 1u));
                    parser->civ_length -= (offset + 1u);
                    break;
                }

                if (!found || result->updated) {
                    break;
                }
            }
        }
    }

    return result->updated;
}

bool atu_cat_parser_decode_civ_frequency(const uint8_t bcd0,
                                         const uint8_t bcd1,
                                         const uint8_t bcd2,
                                         const uint8_t bcd3,
                                         uint32_t *frequency_hz)
{
    const uint8_t bytes[4] = {bcd0, bcd1, bcd2, bcd3};
    uint32_t multiplier = 1u;
    uint32_t value = 0u;
    size_t index = 0u;

    if (frequency_hz == NULL) {
        return false;
    }

    for (index = 0u; index < 4u; ++index) {
        uint8_t low = (uint8_t)(bytes[index] & 0x0Fu);
        uint8_t high = (uint8_t)((bytes[index] >> 4) & 0x0Fu);

        if ((low > 9u) || (high > 9u)) {
            return false;
        }

        value += ((uint32_t)low * multiplier);
        multiplier *= 10u;
        value += ((uint32_t)high * multiplier);
        multiplier *= 10u;
    }

    if ((value < 100000u) || (value > 60000000u)) {
        return false;
    }

    *frequency_hz = value;
    return true;
}
