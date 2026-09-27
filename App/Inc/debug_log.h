#ifndef ATU_DEBUG_LOG_H
#define ATU_DEBUG_LOG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @file debug_log.h
 * @brief Bounded debug logging over shared USB CDC transport
 */

#define ATU_DEBUG_LOG_PREFIX           "#DEBUG:"
#define ATU_DEBUG_LOG_BAUD_ADVERTISED  115200u

typedef size_t (*atu_debug_log_write_fn)(void *context, const uint8_t *data, size_t length);

typedef struct {
    char *buffer;
    size_t capacity;
    size_t head;
    size_t tail;
    size_t dropped_messages;
    bool enabled;
    atu_debug_log_write_fn write;
    void *context;
} atu_debug_log_t;

void atu_debug_log_init(atu_debug_log_t *log,
                        char *buffer,
                        size_t capacity,
                        atu_debug_log_write_fn write_fn,
                        void *context);
void atu_debug_log_set_enabled(atu_debug_log_t *log, bool enabled);
bool atu_debug_log_printf(atu_debug_log_t *log, const char *format, ...);
bool atu_debug_log_write_bytes(atu_debug_log_t *log, const char *label, const uint8_t *data, size_t length);
size_t atu_debug_log_flush(atu_debug_log_t *log, size_t max_bytes);

#endif /* ATU_DEBUG_LOG_H */
