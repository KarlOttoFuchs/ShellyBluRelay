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

#endif // RELAY_CONTROL_H
