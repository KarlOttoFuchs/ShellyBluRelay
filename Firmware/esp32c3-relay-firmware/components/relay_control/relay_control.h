/*
 * Relay Control Component - Public API
 *
 * Story 1.2: Implement Relay Control (GPIO7)
 *
 * This component provides safe relay control with fail-safe guarantees.
 * The relay MUST default to OFF in 100% of error conditions (NFR1).
 */

#ifndef RELAY_CONTROL_H
#define RELAY_CONTROL_H

#include "esp_err.h"
#include <stdbool.h>

/**
 * Initialize the relay control component
 *
 * Configures GPIO7 as output and sets relay to OFF state (fail-safe).
 * This function MUST be called first in app_main() before any other
 * initialization to ensure relay is in safe state.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t relay_init(void);

/**
 * Set relay state
 *
 * @param on true to energize relay (ON), false to de-energize (OFF)
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t relay_set_state(bool on);

/**
 * Get current relay state
 *
 * @return true if relay is ON (energized), false if OFF (de-energized)
 */
bool relay_get_state(void);

/**
 * Force relay OFF immediately (SAFETY-CRITICAL)
 *
 * This function unconditionally turns the relay OFF regardless of current state.
 * It does not check return values and always attempts to set GPIO LOW.
 *
 * CRITICAL: This function MUST be called BEFORE any error handling
 * to ensure relay is in safe state before logging, LED patterns, or restart.
 *
 * Usage pattern:
 *   if (some_operation() != ESP_OK) {
 *       relay_force_off();  // SAFETY FIRST - call before any error handling
 *       ESP_LOGE(TAG, "Operation failed");
 *       // Then handle error (LED pattern, restart, etc.)
 *   }
 */
void relay_force_off(void);

/**
 * Activate relay on sensor trigger (Story 4A.1)
 *
 * Called by sensor handlers when a registered sensor event is detected.
 * Performs state check, activates relay, transitions state, and sets LED.
 *
 * Actions performed:
 * 1. Check if in LISTENING state (skip if not)
 * 2. Capture timestamp for latency measurement
 * 3. Call relay_set_state(true) to energize relay
 * 4. Transition to STATE_ACTIVE
 * 5. Set LED to solid ON pattern
 * 6. Store last trigger timestamp for STATUS command
 * 7. Log relay activation
 *
 * @param mac MAC address string of triggering sensor
 * @param sensor_type Sensor type (BUTTON, MOTION, DOOR)
 * @param event_name Human-readable event name for logging
 * @return ESP_OK if relay activated
 *         ESP_ERR_INVALID_STATE if not in LISTENING state
 *         Other error codes on relay failure
 */
esp_err_t relay_activate_on_trigger(const char *mac, uint8_t sensor_type, const char *event_name);

/**
 * Get last trigger timestamp (Story 4A.1)
 *
 * Returns the uptime in milliseconds when the relay was last triggered
 * by a sensor event. Used by STATUS command.
 *
 * @return Timestamp in milliseconds, or 0 if never triggered
 */
uint32_t relay_get_last_trigger_ms(void);

/**
 * Get last triggering sensor MAC address (Story 4A.1)
 *
 * Returns the MAC address of the sensor that last triggered the relay.
 * Used by STATUS command.
 *
 * @param mac_out Buffer to store MAC string (at least 18 bytes)
 * @param max_len Size of mac_out buffer
 * @return ESP_OK if MAC copied, ESP_ERR_NOT_FOUND if never triggered
 */
esp_err_t relay_get_last_trigger_mac(char *mac_out, size_t max_len);

/**
 * Get last triggering sensor type (Story 4A.1)
 *
 * Returns the sensor type that last triggered the relay.
 * Used by STATUS command.
 *
 * @return Sensor type (1=BUTTON, 2=MOTION, 3=DOOR), or 0 if never triggered
 */
uint8_t relay_get_last_trigger_sensor_type(void);

#endif // RELAY_CONTROL_H
