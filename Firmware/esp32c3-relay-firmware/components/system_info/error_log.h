/*
 * Error Log Component - API
 *
 * Story 5.2: Error Log & Diagnostics
 * Maintains circular buffer of last 10 errors for troubleshooting
 */

#ifndef ERROR_LOG_H
#define ERROR_LOG_H

#include <stdint.h>
#include <stdbool.h>

// Error codes per Story 5.2 AC2
typedef enum {
    ERROR_CODE_NVS_CRC_FAIL,
    ERROR_CODE_NVS_READ_FAIL,
    ERROR_CODE_NVS_WRITE_FAIL,
    ERROR_CODE_BLE_INIT_FAIL,
    ERROR_CODE_WATCHDOG_RESET,
    ERROR_CODE_PANIC_RESET,
    ERROR_CODE_RELAY_FAIL,
    ERROR_CODE_UNKNOWN
} error_code_t;

// Error record structure
typedef struct {
    uint32_t timestamp_ms;      // Uptime in milliseconds when error occurred
    error_code_t error_code;    // Error code enum
    char message[64];           // Human-readable error message
} error_record_t;

/**
 * Initialize error log component
 * Must be called early in app_main()
 */
void error_log_init(void);

/**
 * Log an error to the circular buffer
 *
 * @param error_code  Error code enum value
 * @param message     Human-readable error message (max 63 chars)
 */
void error_log_add(error_code_t error_code, const char *message);

/**
 * Get error history from buffer (newest first)
 *
 * @param records     Array to fill with error records
 * @param max_records Maximum number of records to retrieve (typically 10)
 * @return Number of records actually retrieved
 */
int error_log_get_history(error_record_t *records, int max_records);

/**
 * Get total error count in buffer
 *
 * @return Number of errors currently stored (0-10)
 */
int error_log_get_count(void);

/**
 * Get last error from buffer
 *
 * @param record  Pointer to record structure to fill
 * @return true if error retrieved, false if buffer empty
 */
bool error_log_get_last(error_record_t *record);

/**
 * Convert error code to string
 *
 * @param error_code  Error code enum value
 * @return Static string representation
 */
const char* error_code_to_string(error_code_t error_code);

/**
 * Clear error log (for testing)
 */
void error_log_clear(void);

#endif // ERROR_LOG_H
