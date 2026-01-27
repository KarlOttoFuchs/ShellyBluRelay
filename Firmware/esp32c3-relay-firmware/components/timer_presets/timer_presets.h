/*
 * Timer Presets Component - Public API
 *
 * Story 1.9: Timer Preset Cycling Logic
 *
 * Enables button-based timer preset cycling without serial connection.
 * Short press cycles through presets: 30s -> 60s -> 120s -> 300s -> wrap
 * LED feedback shows current preset (N blinks = preset position 1-4)
 * 5-second timeout confirms selection and saves to NVS
 */

#ifndef TIMER_PRESETS_H
#define TIMER_PRESETS_H

#include "esp_err.h"
#include <stdint.h>

// Preset configuration
#define TIMER_PRESET_COUNT      4
#define TIMER_PRESET_TIMEOUT_MS 5000  // 5-second confirmation timeout

// Preset values in seconds
#define TIMER_PRESET_1          30    // 30 seconds
#define TIMER_PRESET_2          60    // 1 minute
#define TIMER_PRESET_3          120   // 2 minutes
#define TIMER_PRESET_4          300   // 5 minutes (default)

/**
 * Initialize timer presets component
 *
 * Loads current preset index from NVS by matching saved timer_seconds.
 * If saved value doesn't match any preset, defaults to preset 4 (300s).
 *
 * @return ESP_OK on success
 */
esp_err_t timer_presets_init(void);

/**
 * Process timer preset button interactions
 *
 * Call from main loop. Handles:
 * - Short press detection (only in LISTENING state)
 * - Preset cycling with LED feedback
 * - 5-second timeout confirmation
 * - NVS save on timeout
 *
 * Non-blocking - returns immediately if no action needed.
 */
void timer_presets_process(void);

/**
 * Get current preset index (0-3)
 *
 * @return Current preset index where:
 *         0 = 30 seconds (1 blink)
 *         1 = 60 seconds (2 blinks)
 *         2 = 120 seconds (3 blinks)
 *         3 = 300 seconds (4 blinks)
 */
uint8_t timer_presets_get_current_index(void);

/**
 * Get timer value in seconds for given preset index
 *
 * @param index Preset index (0-3)
 * @return Timer duration in seconds, or 300 if index out of range
 */
uint16_t timer_presets_get_value(uint8_t index);

#endif // TIMER_PRESETS_H
