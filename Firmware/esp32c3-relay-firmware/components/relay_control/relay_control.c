/*
 * Relay Control Component - Implementation
 *
 * Story 1.2: Implement Relay Control (GPIO7)
 *
 * Hardware Configuration:
 * - GPIO7 controls relay via MOSFET Q1 (AO3400A)
 * - Relay: SRD-03VDC-SL-C (3VDC coil, 30-40mA)
 * - Active HIGH: GPIO7 HIGH = relay ON, GPIO7 LOW = relay OFF
 * - Flyback diode D2 (1N4148W) protects MOSFET from back-EMF
 *
 * Safety Requirements (NFR1):
 * - Relay MUST default to OFF on boot
 * - Relay MUST be forced OFF before any error handling
 * - relay_force_off() is the safety-critical function for this
 */

#include "relay_control.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "RELAY_CTRL";

// GPIO pin for relay control (from gpio_config.h, but defined here to avoid dependency)
#define RELAY_GPIO_PIN  7

// Track current relay state (GPIO read not reliable for output pins)
static bool relay_current_state = false;

// Track initialization state
static bool relay_initialized = false;

/**
 * Force relay OFF immediately (SAFETY-CRITICAL)
 *
 * This function MUST be called BEFORE any error handling per architecture.md.
 * It does not check return values - it always attempts to set GPIO LOW.
 */
void relay_force_off(void)
{
    // CRITICAL: Always attempt to set GPIO LOW regardless of initialization state
    // Do not check return value - this must always attempt to turn relay OFF
    gpio_set_level(RELAY_GPIO_PIN, 0);
    relay_current_state = false;

    ESP_LOGI(TAG, "Relay forced OFF (safety-critical)");
}

/**
 * Initialize the relay control component
 *
 * MUST be called first in app_main() to ensure fail-safe relay state.
 */
esp_err_t relay_init(void)
{
    esp_err_t ret;

    // Reset GPIO pad to clear any boot-time configuration
    // This is critical for GPIO7 on ESP32-C3 which may have special boot state
    ret = gpio_reset_pin(RELAY_GPIO_PIN);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "GPIO reset warning: %s (continuing)", esp_err_to_name(ret));
    }

    // Configure GPIO with full pad configuration
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << RELAY_GPIO_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure relay GPIO: %s", esp_err_to_name(ret));
        // Still attempt to force relay OFF even if config failed
        relay_force_off();
        return ret;
    }

    // CRITICAL: Force relay OFF immediately after GPIO configuration (NFR1)
    relay_force_off();

    relay_initialized = true;
    ESP_LOGI(TAG, "Relay initialized (GPIO%d, default OFF)", RELAY_GPIO_PIN);

    return ESP_OK;
}

/**
 * Set relay state
 *
 * @param on true to energize relay, false to de-energize
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t relay_set_state(bool on)
{
    if (!relay_initialized) {
        ESP_LOGE(TAG, "Relay not initialized - call relay_init() first");
        relay_force_off();  // Safety first
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = gpio_set_level(RELAY_GPIO_PIN, on ? 1 : 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GPIO set failed: %s", esp_err_to_name(ret));
        relay_force_off();  // Safety first on any error
        return ret;
    }

    relay_current_state = on;
    ESP_LOGI(TAG, "Relay state: %s", on ? "ON" : "OFF");

    return ESP_OK;
}

/**
 * Get current relay state
 *
 * @return true if relay is ON, false if OFF
 */
bool relay_get_state(void)
{
    return relay_current_state;
}
