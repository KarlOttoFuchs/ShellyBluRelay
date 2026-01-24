/*
 * Button Input Component - Public API Header
 *
 * Story 1.3: Implement Button Input Detection (GPIO9)
 *
 * Hardware Configuration:
 * - GPIO9 connected to tactile button S1 (160gf)
 * - External pull-up: R6 = 10kΩ to 3.3V
 * - Active LOW: GPIO9 LOW = button pressed, GPIO9 HIGH = button released
 * - Also strapping pin for boot mode (LOW during reset = download mode)
 *
 * Usage:
 *   button_init();                          // Initialize button GPIO
 *   bool pressed = button_is_pressed();     // Quick check if pressed
 *   button_wait_for_press(5000);            // Wait up to 5 seconds for press
 */

#ifndef BUTTON_INPUT_H
#define BUTTON_INPUT_H

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * Initialize the button input component
 *
 * Configures GPIO9 as input with internal pull-up enabled (backup to external R6).
 * Uses gpio_reset_pin() + gpio_config() pattern for reliable ESP32-C3 operation.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t button_init(void);

/**
 * Read button state with software debouncing
 *
 * Performs two reads separated by debounce delay (20ms).
 * Only reports "pressed" if both reads show pressed state.
 *
 * @param pressed Output parameter: true if button pressed, false if released
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_STATE if button_init() not called
 * @return ESP_ERR_INVALID_ARG if pressed is NULL
 */
esp_err_t button_read_state(bool *pressed);

/**
 * Check if button is currently pressed (convenience function)
 *
 * Uses debounced read internally.
 *
 * @return true if button is pressed, false if released or error
 */
bool button_is_pressed(void);

/**
 * Wait for button press with timeout
 *
 * Blocks until button is pressed or timeout expires.
 * Polls button state every 10ms using debounced read.
 *
 * @param timeout_ms Maximum time to wait in milliseconds (0 = wait forever)
 * @return ESP_OK if button press detected
 * @return ESP_ERR_TIMEOUT if timeout expired without press
 * @return ESP_ERR_INVALID_STATE if button_init() not called
 */
esp_err_t button_wait_for_press(uint32_t timeout_ms);

/**
 * Wait for button release with timeout
 *
 * Blocks until button is released or timeout expires.
 * Useful after detecting a press to wait for release.
 *
 * @param timeout_ms Maximum time to wait in milliseconds (0 = wait forever)
 * @return ESP_OK if button release detected
 * @return ESP_ERR_TIMEOUT if timeout expired without release
 * @return ESP_ERR_INVALID_STATE if button_init() not called
 */
esp_err_t button_wait_for_release(uint32_t timeout_ms);

/**
 * Check for button long press (2 seconds)
 *
 * Non-blocking function that tracks button state across calls.
 * Returns true ONCE when button is released after being held for 2+ seconds.
 * Must be called repeatedly in main loop for accurate detection.
 *
 * @return true if long press detected (on release after 2s hold)
 * @return false otherwise
 */
bool button_check_long_press(void);

/**
 * Reset long press detection state
 *
 * Resets internal tracking state. Call when entering a mode that
 * should ignore any ongoing button press detection.
 */
void button_reset_long_press(void);

#endif // BUTTON_INPUT_H
