#include "debug_log.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static size_t atu_debug_log_bytes_used(const atu_debug_log_t *log)
{
    if (log->head >= log->tail) {
        return log->head - log->tail;
    }

    return (log->capacity - log->tail) + log->head;
}

static size_t atu_debug_log_bytes_free(const atu_debug_log_t *log)
{
    return (log->capacity - atu_debug_log_bytes_used(log)) - 1u;
}

static bool atu_debug_log_push(atu_debug_log_t *log, const char *text, size_t length)
{
    size_t index = 0u;

    if ((log == NULL) || (text == NULL) || (length == 0u)) {
        return false;
    }

    if (length > atu_debug_log_bytes_free(log)) {
        log->dropped_messages++;
        return false;
    }

    for (index = 0u; index < length; ++index) {
        log->buffer[log->head] = text[index];
        log->head = (log->head + 1u) % log->capacity;
    }

    return true;
}

void atu_debug_log_init(atu_debug_log_t *log,
                        char *buffer,
                        size_t capacity,
                        atu_debug_log_write_fn write_fn,
                        void *context)
{
    if ((log == NULL) || (buffer == NULL) || (capacity < 2u)) {
        return;
    }

    memset(log, 0, sizeof(*log));
    log->buffer = buffer;
    log->capacity = capacity;
    log->write = write_fn;
    log->context = context;
}

void atu_debug_log_set_enabled(atu_debug_log_t *log, bool enabled)
{
    if (log == NULL) {
        return;
    }

    log->enabled = enabled;
}

bool atu_debug_log_printf(atu_debug_log_t *log, const char *format, ...)
{
    char line[192];
    int written = 0;
    va_list args;

    if ((log == NULL) || (!log->enabled) || (format == NULL)) {
        return false;
    }

    written = snprintf(line, sizeof(line), "%s ", ATU_DEBUG_LOG_PREFIX);
    if ((written < 0) || ((size_t)written >= sizeof(line))) {
        return false;
    }

    va_start(args, format);
    written += vsnprintf(&line[written], sizeof(line) - (size_t)written, format, args);
    va_end(args);

    if (written < 0) {
        return false;
    }

    if ((size_t)written >= (sizeof(line) - 1u)) {
        written = (int)(sizeof(line) - 2u);
    }

    line[written++] = '\n';
    line[written] = '\0';

    return atu_debug_log_push(log, line, (size_t)written);
}

bool atu_debug_log_write_bytes(atu_debug_log_t *log, const char *label, const uint8_t *data, size_t length)
{
    char line[224];
    size_t offset = 0u;
    size_t index = 0u;

    if ((log == NULL) || (!log->enabled) || (label == NULL) || (data == NULL)) {
        return false;
    }

    offset = (size_t)snprintf(line, sizeof(line), "%s %s:", ATU_DEBUG_LOG_PREFIX, label);
    if (offset >= sizeof(line)) {
        return false;
    }

    for (index = 0u; (index < length) && ((offset + 4u) < sizeof(line)); ++index) {
        offset += (size_t)snprintf(&line[offset], sizeof(line) - offset, " %02X", data[index]);
    }

    if ((offset + 1u) >= sizeof(line)) {
        return false;
    }

    line[offset++] = '\n';
    line[offset] = '\0';

    return atu_debug_log_push(log, line, offset);
}

size_t atu_debug_log_flush(atu_debug_log_t *log, size_t max_bytes)
{
    uint8_t chunk[64];
    size_t bytes_to_send = 0u;
    size_t index = 0u;
    size_t written = 0u;

    if ((log == NULL) || (log->write == NULL) || (max_bytes == 0u)) {
        return 0u;
    }

    while ((log->tail != log->head) && (written < max_bytes)) {
        bytes_to_send = 0u;
        while ((log->tail != log->head) &&
               (bytes_to_send < sizeof(chunk)) &&
               ((written + bytes_to_send) < max_bytes)) {
            chunk[bytes_to_send++] = (uint8_t)log->buffer[log->tail];
            log->tail = (log->tail + 1u) % log->capacity;
        }

        if (bytes_to_send == 0u) {
            break;
        }

        index = log->write(log->context, chunk, bytes_to_send);
        written += index;

        if (index < bytes_to_send) {
            size_t rewind = bytes_to_send - index;
            while (rewind > 0u) {
                log->tail = (log->tail == 0u) ? (log->capacity - 1u) : (log->tail - 1u);
                --rewind;
            }
            break;
        }
    }

    return written;
}
