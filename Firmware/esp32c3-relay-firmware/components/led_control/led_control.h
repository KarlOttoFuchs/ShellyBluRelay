/*
 * LED Control Component - Public API Header
 *
 * Story 1.4: Implement LED Control (Status & Error LEDs)
 *
 * Hardware Configuration:
 * - GPIO10 (Status LED): White LED D3, active HIGH, R8=1kΩ (~1.3mA)
 * - GPIO0 (Error LED): Red LED D1, active HIGH, R7=1kΩ (~1.3mA)
 *
 * Note: GPIO0 is a strapping pin (LOW during reset = download mode).
 * LED circuit doesn't pull GPIO low, so this is safe.
 *
 * Usage:
 *   led_init();                                    // Initialize LED control
 *   led_set_pattern(LED_STATUS, LED_PATTERN_BLINK_SLOW);  // Slow blink
 *   led_set_state(LED_ERROR, true);               // Turn on error LED
 */

#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

// Story 1.8: Counted blink constants
#define LED_BLINK_COUNT_ON_MS   250   // On duration for counted blinks
#define LED_BLINK_COUNT_OFF_MS  250   // Off duration between counted blinks
#define LED_BLINK_COUNT_PAUSE_MS 500  // Pause after blinks before restoring pattern
#define LED_BLINK_COUNT_MAX     10    // Maximum blink count (clamped)

/**
 * LED identifiers
 */
typedef enum {
    LED_STATUS = 0,  // White LED (GPIO10) - system status indication
    LED_ERROR  = 1,  // Red LED (GPIO0) - error indication
    LED_COUNT  = 2   // Number of LEDs
} led_id_t;

/**
 * LED patterns
 */
typedef enum {
    LED_PATTERN_OFF = 0,           // Solid OFF
    LED_PATTERN_ON,                // Solid ON
    LED_PATTERN_BLINK_FAST,        // Fast blink (250ms on/250ms off) - configuration mode
    LED_PATTERN_BLINK_SLOW,        // Slow blink (1s on/1s off) - listening mode
    LED_PATTERN_ERROR_SINGLE,      // Single blink then pause - error indication
    LED_PATTERN_ERROR_DOUBLE,      // Double blink then pause - error indication
    LED_PATTERN_ERROR_TRIPLE,      // Triple blink then pause - error indication
    LED_PATTERN_COUNT              // Number of patterns
} led_pattern_t;

/**
 * Initialize the LED control component
 *
 * Configures GPIO0 and GPIO10 as outputs using gpio_reset_pin() + gpio_config()
 * pattern. Creates LED pattern task for non-blocking pattern execution.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t led_init(void);

/**
 * Set LED pattern
 *
 * Sets the pattern for the specified LED. Pattern runs in background task.
 *
 * @param led LED identifier (LED_STATUS or LED_ERROR)
 * @param pattern Pattern to set
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if led or pattern is invalid
 * @return ESP_ERR_INVALID_STATE if led_init() not called
 */
esp_err_t led_set_pattern(led_id_t led, led_pattern_t pattern);

/**
 * Set LED state (convenience function)
 *
 * Wrapper that sets LED_PATTERN_ON or LED_PATTERN_OFF.
 *
 * @param led LED identifier (LED_STATUS or LED_ERROR)
 * @param on true = LED on, false = LED off
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if led is invalid
 * @return ESP_ERR_INVALID_STATE if led_init() not called
 */
esp_err_t led_set_state(led_id_t led, bool on);

/**
 * Get current LED pattern
 *
 * @param led LED identifier (LED_STATUS or LED_ERROR)
 * @param pattern Output parameter for current pattern
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if led is invalid or pattern is NULL
 * @return ESP_ERR_INVALID_STATE if led_init() not called
 */
esp_err_t led_get_pattern(led_id_t led, led_pattern_t *pattern);

/**
 * Blink LED a specific number of times (Story 1.8)
 *
 * Non-blocking function that queues counted blinks for execution by the
 * LED pattern task. After all blinks complete, the LED returns to its
 * previous pattern and the optional callback is invoked.
 *
 * Timing: 150ms on + 150ms off per blink (300ms per cycle)
 *
 * @param led LED identifier (LED_STATUS or LED_ERROR)
 * @param count Number of blinks (1-10, 0 = immediate callback, >10 clamped to 10)
 * @param callback Optional function called when blinks complete (may be NULL)
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if led is invalid
 * @return ESP_ERR_INVALID_STATE if led_init() not called
 */
esp_err_t led_blink_count(led_id_t led, uint8_t count, void (*callback)(void));

#endif // LED_CONTROL_H
