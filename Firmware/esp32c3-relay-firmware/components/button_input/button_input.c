/*
 * Button Input Component - Implementation
 *
 * Story 1.3: Implement Button Input Detection (GPIO9)
 * Story 1.7: Button Short-Press Detection
 *
 * Hardware Configuration:
 * - GPIO9 connected to tactile button S1 (160gf actuation)
 * - External pull-up: R6 = 10kΩ to 3.3V
 * - Active LOW: GPIO9 LOW = pressed, GPIO9 HIGH = released
 * - Strapping pin: LOW during reset enters download boot mode
 *
 * Press Duration Classification (Story 1.7):
 *   < 500ms         SHORT PRESS  -> timer preset cycling
 *   500ms - 2000ms  IGNORED      -> debounce zone
 *   >= 2000ms       LONG PRESS   -> learning mode
 *
 * Debouncing Strategy:
 * - Read GPIO twice with 20ms delay between reads
 * - Only report "pressed" if both reads agree
 * - Mechanical switch bounce typically 5-20ms
 */

#include "button_input.h"
#include "relay_control.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "BUTTON_INPUT";

// GPIO pin for button input (from gpio_config.h, but defined here to avoid dependency)
#define BUTTON_GPIO_PIN  9

// Debounce time in milliseconds
#define DEBOUNCE_TIME_MS  20

// Polling interval for wait functions
#define POLL_INTERVAL_MS  10

// Long press detection threshold (2 seconds per AC1)
#define BUTTON_LONG_PRESS_MS  2000

// Track initialization state
static bool button_initialized = false;

// Press type classification (Story 1.7)
typedef enum {
    PRESS_NONE,    // No press detected yet
    PRESS_SHORT,   // < 500ms
    PRESS_MEDIUM,  // 500-2000ms (ignored debounce zone)
    PRESS_LONG     // >= 2000ms
} press_type_t;

// Button press detection state (shared between short and long press)
static uint32_t button_press_start_ms = 0;
static bool button_was_pressed = false;
static press_type_t last_detected_press = PRESS_NONE;
static bool press_consumed = false;  // Prevents double-detection

/**
 * Read raw button state (no debouncing)
 *
 * @return true if button is pressed (GPIO LOW), false if released (GPIO HIGH)
 */
static bool button_read_raw(void)
{
    // Active LOW: GPIO LOW = pressed, GPIO HIGH = released
    return (gpio_get_level(BUTTON_GPIO_PIN) == 0);
}

/**
 * Initialize the button input component
 */
esp_err_t button_init(void)
{
    esp_err_t ret;

    // Reset GPIO pad to clear any boot-time configuration
    // CRITICAL: ESP32-C3 requires this for reliable GPIO operation (Story 1.2 learning)
    ret = gpio_reset_pin(BUTTON_GPIO_PIN);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "GPIO reset warning: %s (continuing)", esp_err_to_name(ret));
    }

    // Configure GPIO with full pad configuration
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BUTTON_GPIO_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,    // Internal pull-up (backup to external R6)
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,      // Polling mode, no interrupts
    };

    ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure button GPIO: %s", esp_err_to_name(ret));
        relay_force_off();  // Safety first on any error
        return ret;
    }

    button_initialized = true;
    ESP_LOGI(TAG, "Button input initialized (GPIO%d, active-low with pull-up)", BUTTON_GPIO_PIN);

    return ESP_OK;
}

/**
 * Read button state with software debouncing
 */
esp_err_t button_read_state(bool *pressed)
{
    if (!button_initialized) {
        ESP_LOGE(TAG, "Button not initialized - call button_init() first");
        return ESP_ERR_INVALID_STATE;
    }

    if (pressed == NULL) {
        ESP_LOGE(TAG, "Invalid argument: pressed is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    // First read
    bool first_read = button_read_raw();

    // Wait for debounce period
    vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_TIME_MS));

    // Second read
    bool second_read = button_read_raw();

    // Both reads must agree for valid pressed state
    // Only report pressed if both reads show pressed (conservative debouncing)
    *pressed = (first_read && second_read);

    return ESP_OK;
}

/**
 * Check if button is currently pressed (convenience function)
 */
bool button_is_pressed(void)
{
    bool pressed = false;

    if (button_read_state(&pressed) != ESP_OK) {
        return false;  // Return false on any error
    }

    return pressed;
}

/**
 * Wait for button press with timeout
 */
esp_err_t button_wait_for_press(uint32_t timeout_ms)
{
    if (!button_initialized) {
        ESP_LOGE(TAG, "Button not initialized - call button_init() first");
        return ESP_ERR_INVALID_STATE;
    }

    uint32_t elapsed_ms = 0;
    bool pressed = false;

    ESP_LOGI(TAG, "Waiting for button press (timeout: %lums)", (unsigned long)timeout_ms);

    while (timeout_ms == 0 || elapsed_ms < timeout_ms) {
        // Read with debouncing
        esp_err_t ret = button_read_state(&pressed);
        if (ret != ESP_OK) {
            return ret;
        }

        if (pressed) {
            ESP_LOGI(TAG, "Button press detected");
            return ESP_OK;
        }

        // Poll interval (note: button_read_state already delays DEBOUNCE_TIME_MS)
        // So effective poll rate is DEBOUNCE_TIME_MS + POLL_INTERVAL_MS
        vTaskDelay(pdMS_TO_TICKS(POLL_INTERVAL_MS));
        elapsed_ms += DEBOUNCE_TIME_MS + POLL_INTERVAL_MS;
    }

    ESP_LOGW(TAG, "Button wait timeout (%lums)", (unsigned long)timeout_ms);
    return ESP_ERR_TIMEOUT;
}

/**
 * Wait for button release with timeout
 */
esp_err_t button_wait_for_release(uint32_t timeout_ms)
{
    if (!button_initialized) {
        ESP_LOGE(TAG, "Button not initialized - call button_init() first");
        return ESP_ERR_INVALID_STATE;
    }

    uint32_t elapsed_ms = 0;
    bool pressed = false;

    ESP_LOGI(TAG, "Waiting for button release (timeout: %lums)", (unsigned long)timeout_ms);

    while (timeout_ms == 0 || elapsed_ms < timeout_ms) {
        // Read with debouncing
        esp_err_t ret = button_read_state(&pressed);
        if (ret != ESP_OK) {
            return ret;
        }

        if (!pressed) {
            ESP_LOGI(TAG, "Button release detected");
            return ESP_OK;
        }

        // Poll interval
        vTaskDelay(pdMS_TO_TICKS(POLL_INTERVAL_MS));
        elapsed_ms += DEBOUNCE_TIME_MS + POLL_INTERVAL_MS;
    }

    ESP_LOGW(TAG, "Button release wait timeout (%lums)", (unsigned long)timeout_ms);
    return ESP_ERR_TIMEOUT;
}

/**
 * Update button state and classify press on release (Story 1.7)
 *
 * Internal function that tracks button state and classifies the press
 * duration when the button is released. Called by both short and long
 * press detection functions.
 *
 * @return The detected press type (PRESS_NONE if no release detected)
 */
static press_type_t button_update_state(void)
{
    if (!button_initialized) {
        return PRESS_NONE;
    }

    // Read raw button state (active LOW)
    bool currently_pressed = (gpio_get_level(BUTTON_GPIO_PIN) == 0);
    uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);

    if (currently_pressed && !button_was_pressed) {
        // Button just pressed - start timer, reset detection state
        button_press_start_ms = now_ms;
        button_was_pressed = true;
        last_detected_press = PRESS_NONE;
        press_consumed = false;
        return PRESS_NONE;
    }

    // If we already detected a press type this cycle, return it
    // This allows multiple callers (short/long press checks) to see the same result
    if (last_detected_press != PRESS_NONE) {
        return last_detected_press;
    }

    if (!currently_pressed && button_was_pressed) {
        // Button just released - classify press duration
        button_was_pressed = false;
        uint32_t duration = now_ms - button_press_start_ms;

        if (duration < SHORT_PRESS_THRESHOLD_MS) {
            last_detected_press = PRESS_SHORT;
            ESP_LOGI(TAG, "Short press detected (%lu ms)", (unsigned long)duration);
        } else if (duration < BUTTON_LONG_PRESS_MS) {
            last_detected_press = PRESS_MEDIUM;
            ESP_LOGD(TAG, "Medium press ignored (%lu ms)", (unsigned long)duration);
        } else {
            last_detected_press = PRESS_LONG;
            ESP_LOGI(TAG, "Long press detected (%lu ms)", (unsigned long)duration);
        }
        return last_detected_press;
    }

    return PRESS_NONE;
}

/**
 * Check for button short press (< 500ms) - Story 1.7
 *
 * Non-blocking function that tracks button state across calls.
 * Returns true ONCE when button is released after being held for < 500ms.
 * Must be called repeatedly in main loop for accurate detection.
 */
bool button_check_short_press(void)
{
    press_type_t press = button_update_state();

    // Return true only once for a short press
    if (press == PRESS_SHORT && !press_consumed) {
        press_consumed = true;
        return true;
    }

    return false;
}

/**
 * Check for button long press (>= 2000ms)
 *
 * Non-blocking function that tracks button state across calls.
 * Returns true ONCE when button is released after being held for 2+ seconds.
 * Must be called repeatedly in main loop for accurate detection.
 */
bool button_check_long_press(void)
{
    press_type_t press = button_update_state();

    // Return true only once for a long press
    if (press == PRESS_LONG && !press_consumed) {
        press_consumed = true;
        return true;
    }

    return false;
}

/**
 * Reset button press detection state
 *
 * Resets internal tracking state for both short and long press detection.
 * Call when entering a mode that should ignore any ongoing button press.
 */
void button_reset_long_press(void)
{
    button_press_start_ms = 0;
    button_was_pressed = false;
    last_detected_press = PRESS_NONE;
    press_consumed = false;
}
