/*
 * Relay Timer Component - Public API
 *
 * Story 4A.2: Automatic Timer De-energize with Configurable Duration
 *
 * Provides automatic relay de-energization after a configurable timer.
 * Uses esp_timer for accurate countdown with 1-second resolution.
 */

#ifndef RELAY_TIMER_H
#define RELAY_TIMER_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

// Default timer duration if not configured in NVS (AC2)
#define RELAY_TIMER_DEFAULT_SECONDS  10

/**
 * Start relay timer countdown
 *
 * Starts a one-shot timer that will de-energize the relay when it expires.
 * If a timer is already running, it is stopped and restarted.
 *
 * On expiry, the callback will:
 * - Call relay_set_state(false) to de-energize relay
 * - Transition state to STATE_LISTENING
 * - Set LED to slow blink pattern
 * - Log "Timer expired" and "Relay deactivated"
 *
 * @param seconds Timer duration in seconds (1-600)
 * @return ESP_OK on success
 *         ESP_ERR_INVALID_ARG if seconds is 0 or > 600
 *         ESP_FAIL on timer creation/start failure
 */
esp_err_t relay_timer_start(uint16_t seconds);

/**
 * Stop relay timer
 *
 * Cancels any running relay timer. If no timer is running, does nothing.
 * Does NOT de-energize the relay.
 *
 * @return ESP_OK on success
 */
esp_err_t relay_timer_stop(void);

/**
 * Get remaining timer seconds
 *
 * Returns the remaining time on the relay timer in seconds.
 * Returns 0 if no timer is running or timer has expired.
 *
 * @return Remaining seconds (0 if no timer active)
 */
uint16_t relay_timer_get_remaining_sec(void);

/**
 * Check if timer is active
 *
 * @return true if timer is running, false otherwise
 */
bool relay_timer_is_active(void);

#endif // RELAY_TIMER_H
