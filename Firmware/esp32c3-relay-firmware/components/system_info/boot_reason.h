/*
 * Boot Reason Tracking API
 *
 * Story 4B.4: Provides boot reason information for diagnostics
 */

#ifndef BOOT_REASON_H
#define BOOT_REASON_H

#include "esp_system.h"

/**
 * Initialize boot reason tracking
 *
 * Must be called early in app_main() to capture the reset reason
 * before any other operations that might affect it.
 */
void boot_reason_init(void);

/**
 * Get human-readable boot reason string
 *
 * Returns the reason for the most recent boot/reset as a human-readable string.
 * Must be called after boot_reason_init().
 *
 * Possible return values (per AC4):
 * - "POWER_ON"   - Normal power-on reset
 * - "SOFTWARE"   - Software-triggered reset (esp_restart())
 * - "PANIC"      - Firmware crash/panic
 * - "WATCHDOG"   - Task or interrupt watchdog timeout
 * - "BROWNOUT"   - Voltage brownout reset
 * - "DEEPSLEEP"  - Wake from deep sleep
 * - "UNKNOWN"    - Other/unrecognized reset reason
 *
 * @return Static string describing the boot reason (do not free)
 */
const char* get_boot_reason_string(void);

/**
 * Get raw ESP-IDF reset reason
 *
 * @return esp_reset_reason_t enum value
 */
esp_reset_reason_t get_boot_reason(void);

#endif // BOOT_REASON_H
