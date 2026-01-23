/*
 * Button Input Component - Implementation
 *
 * Story 1.3: Implement Button Input Detection (GPIO9)
 *
 * Hardware Configuration:
 * - GPIO9 connected to tactile button S1 (160gf actuation)
 * - External pull-up: R6 = 10kΩ to 3.3V
 * - Active LOW: GPIO9 LOW = pressed, GPIO9 HIGH = released
 * - Strapping pin: LOW during reset enters download boot mode
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
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "BUTTON_INPUT";

// GPIO pin for button input (from gpio_config.h, but defined here to avoid dependency)
#define BUTTON_GPIO_PIN  9

// Debounce time in milliseconds
#define DEBOUNCE_TIME_MS  20

// Polling interval for wait functions
#define POLL_INTERVAL_MS  10

// Track initialization state
static bool button_initialized = false;

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
