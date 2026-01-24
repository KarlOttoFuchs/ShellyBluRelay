/*
 * Learning Mode - Public API
 *
 * Story 3.2: Implement 30-Second Learning Mode
 *
 * Handles the 30-second sensor registration learning mode.
 * Captures first BTHome sensor event and registers it in NVS.
 */

#ifndef LEARNING_MODE_H
#define LEARNING_MODE_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

// Learning mode duration in milliseconds (30 seconds per FR12)
#define LEARNING_MODE_DURATION_MS  30000

// Default timer for registered sensor (10 seconds per Story 3.2 spec)
#define LEARNING_DEFAULT_TIMER_SECONDS  10

/**
 * Initialize learning mode subsystem
 *
 * Sets up the timer for learning mode timeout.
 * Must be called once at startup.
 *
 * @return ESP_OK on success
 */
esp_err_t learning_mode_init(void);

/**
 * Enter learning mode
 *
 * Starts 30-second countdown timer and sets LED to fast blink.
 * Logs "Entering learning mode for 30 seconds".
 * No-op if already in learning mode.
 */
void enter_learning_mode(void);

/**
 * Exit learning mode
 *
 * Stops the countdown timer and clears learning mode state.
 * Idempotent - safe to call multiple times.
 */
void exit_learning_mode(void);

/**
 * Check if currently in learning mode
 *
 * @return true if learning mode is active
 */
bool is_learning_mode_active(void);

/**
 * Get time remaining in learning mode
 *
 * @return Seconds remaining (0-30), or 0 if not in learning mode
 */
uint32_t learning_mode_time_remaining_sec(void);

/**
 * Get learning mode start timestamp
 *
 * @return Start time in milliseconds, or 0 if not in learning mode
 */
uint32_t learning_mode_get_start_ms(void);

/**
 * Register a captured sensor during learning mode
 *
 * Called by BLE callback when a sensor event is received in learning mode.
 * Saves sensor config to NVS and exits learning mode.
 *
 * @param mac         6-byte MAC address array
 * @param sensor_type Sensor type (BUTTON=1, MOTION=2, DOOR=3)
 * @param battery_pct Battery percentage (0-100, 255=unknown)
 * @param rssi        Signal strength in dBm
 */
void register_captured_sensor(const uint8_t *mac, uint8_t sensor_type,
                               uint8_t battery_pct, int8_t rssi);

/**
 * Process learning mode sensor capture from BLE event
 *
 * Called from main context (not BLE callback) to handle sensor registration.
 * Checks if learning mode is active and a sensor was captured.
 */
void learning_mode_process_capture(void);

#endif // LEARNING_MODE_H
