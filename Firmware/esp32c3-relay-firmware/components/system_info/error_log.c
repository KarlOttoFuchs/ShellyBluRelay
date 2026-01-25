/*
 * Error Log Component - Implementation
 *
 * Story 5.2: Error Log & Diagnostics
 */

#include "error_log.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>

static const char *TAG = "ERROR_LOG";

// Circular buffer for last 10 errors (Story 5.2 AC1)
#define ERROR_BUFFER_SIZE 10
static error_record_t error_buffer[ERROR_BUFFER_SIZE];
static uint8_t error_write_index = 0;
static uint8_t error_count = 0;  // 0-10

void error_log_init(void)
{
    memset(error_buffer, 0, sizeof(error_buffer));
    error_write_index = 0;
    error_count = 0;
    ESP_LOGI(TAG, "Error log initialized (capacity: %d)", ERROR_BUFFER_SIZE);
}

void error_log_add(error_code_t error_code, const char *message)
{
    if (message == NULL) {
        message = "No message provided";
    }

    error_record_t *record = &error_buffer[error_write_index];
    record->timestamp_ms = esp_timer_get_time() / 1000;  // µs to ms
    record->error_code = error_code;
    strncpy(record->message, message, sizeof(record->message) - 1);
    record->message[sizeof(record->message) - 1] = '\0';

    // Advance write pointer (circular)
    error_write_index = (error_write_index + 1) % ERROR_BUFFER_SIZE;
    if (error_count < ERROR_BUFFER_SIZE) {
        error_count++;
    }

    ESP_LOGW(TAG, "Error logged: [%s] %s",
             error_code_to_string(error_code), message);
}

int error_log_get_history(error_record_t *records, int max_records)
{
    if (records == NULL || max_records <= 0) {
        return 0;
    }

    int count = (error_count < max_records) ? error_count : max_records;

    // Copy errors in reverse chronological order (newest first)
    for (int i = 0; i < count; i++) {
        int buffer_index = (error_write_index - 1 - i + ERROR_BUFFER_SIZE) % ERROR_BUFFER_SIZE;
        records[i] = error_buffer[buffer_index];
    }

    return count;
}

int error_log_get_count(void)
{
    return error_count;
}

bool error_log_get_last(error_record_t *record)
{
    if (error_count == 0 || record == NULL) {
        return false;
    }

    int last_index = (error_write_index - 1 + ERROR_BUFFER_SIZE) % ERROR_BUFFER_SIZE;
    *record = error_buffer[last_index];
    return true;
}

const char* error_code_to_string(error_code_t error_code)
{
    switch (error_code) {
        case ERROR_CODE_NVS_CRC_FAIL:    return "NVS_CRC_FAIL";
        case ERROR_CODE_NVS_READ_FAIL:   return "NVS_READ_FAIL";
        case ERROR_CODE_NVS_WRITE_FAIL:  return "NVS_WRITE_FAIL";
        case ERROR_CODE_BLE_INIT_FAIL:   return "BLE_INIT_FAIL";
        case ERROR_CODE_WATCHDOG_RESET:  return "WATCHDOG_RESET";
        case ERROR_CODE_PANIC_RESET:     return "PANIC_RESET";
        case ERROR_CODE_RELAY_FAIL:      return "RELAY_FAIL";
        default:                         return "UNKNOWN";
    }
}

void error_log_clear(void)
{
    error_count = 0;
    error_write_index = 0;
    memset(error_buffer, 0, sizeof(error_buffer));
    ESP_LOGI(TAG, "Error log cleared");
}
