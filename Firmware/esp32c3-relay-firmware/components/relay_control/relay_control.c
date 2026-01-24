/*
 * Relay Control Component - Implementation
 *
 * Story 1.2: Implement Relay Control (GPIO7)
 * Story 4A.1: Relay Activation on Registered Sensor Event
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
#include "relay_timer.h"
#include "state_machine.h"
#include "led_control.h"
#include "nvs_storage.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>

static const char *TAG = "RELAY_CTRL";

// GPIO pin for relay control (from gpio_config.h, but defined here to avoid dependency)
#define RELAY_GPIO_PIN  7

// Track current relay state (GPIO read not reliable for output pins)
static bool relay_current_state = false;

// Track initialization state
static bool relay_initialized = false;

// Story 4A.1: Last trigger information for STATUS command
static uint32_t last_trigger_ms = 0;
static char last_trigger_mac[18] = {0};
static uint8_t last_trigger_sensor_type = 0;

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

/**
 * Activate relay on sensor trigger (Story 4A.1)
 *
 * Called by sensor handlers when a registered sensor event is detected.
 * Performs state check, activates relay, transitions state, and sets LED.
 */
esp_err_t relay_activate_on_trigger(const char *mac, uint8_t sensor_type, const char *event_name)
{
    // Capture timestamp at entry for latency measurement (NFP1)
    int64_t entry_time_us = esp_timer_get_time();

    // Check if we're in LISTENING state - only activate from LISTENING
    system_state_t current_state = state_get_current();
    if (current_state != STATE_LISTENING) {
        ESP_LOGD(TAG, "Ignoring trigger - not in LISTENING state (current: %s)",
                 state_to_string(current_state));
        return ESP_ERR_INVALID_STATE;
    }

    // Log the sensor trigger event (AC5)
    ESP_LOGI(TAG, "Sensor triggered: %s (%s)", mac, event_name);

    // Activate relay (AC2)
    esp_err_t ret = relay_set_state(true);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to activate relay: %s", esp_err_to_name(ret));
        return ret;
    }

    // Capture relay activation timestamp
    int64_t relay_time_us = esp_timer_get_time();

    // Transition to STATE_ACTIVE (AC1)
    state_set(STATE_ACTIVE);

    // Set LED to solid ON (AC4)
    led_set_pattern(LED_STATUS, LED_PATTERN_ON);

    // Store trigger info for STATUS command (AC8)
    last_trigger_ms = (uint32_t)(relay_time_us / 1000);
    if (mac != NULL) {
        strncpy(last_trigger_mac, mac, sizeof(last_trigger_mac) - 1);
        last_trigger_mac[sizeof(last_trigger_mac) - 1] = '\0';
    }
    last_trigger_sensor_type = sensor_type;

    // Log relay activation (AC5)
    ESP_LOGI(TAG, "Relay activated");

    // Latency measurement (NFP1 - must be ≤500ms)
    int64_t latency_us = relay_time_us - entry_time_us;
    int64_t latency_ms = latency_us / 1000;
    if (latency_ms > 500) {
        ESP_LOGW(TAG, "LATENCY VIOLATION: %lld ms (max 500ms)", latency_ms);
    } else {
        ESP_LOGD(TAG, "Activation latency: %lld ms", latency_ms);
    }

    // Story 4A.2: Start auto-deactivate timer (AC1, AC2)
    // Load timer duration from NVS config, default to 10 seconds
    uint16_t timer_seconds = RELAY_TIMER_DEFAULT_SECONDS;
    sensor_config_t config;
    if (nvs_load_config(&config) == ESP_OK) {
        timer_seconds = config.timer_seconds;
        ESP_LOGD(TAG, "Using configured timer: %u seconds", timer_seconds);
    } else {
        ESP_LOGD(TAG, "Using default timer: %u seconds", timer_seconds);
    }

    esp_err_t timer_ret = relay_timer_start(timer_seconds);
    if (timer_ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to start timer: %s", esp_err_to_name(timer_ret));
        // Continue anyway - relay is activated, timer failure is non-critical
    }

    return ESP_OK;
}

/**
 * Get last trigger timestamp (Story 4A.1)
 */
uint32_t relay_get_last_trigger_ms(void)
{
    return last_trigger_ms;
}

/**
 * Get last triggering sensor MAC address (Story 4A.1)
 */
esp_err_t relay_get_last_trigger_mac(char *mac_out, size_t max_len)
{
    if (mac_out == NULL || max_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    if (last_trigger_mac[0] == '\0') {
        return ESP_ERR_NOT_FOUND;
    }

    strncpy(mac_out, last_trigger_mac, max_len - 1);
    mac_out[max_len - 1] = '\0';
    return ESP_OK;
}

/**
 * Get last triggering sensor type (Story 4A.1)
 */
uint8_t relay_get_last_trigger_sensor_type(void)
{
    return last_trigger_sensor_type;
}
