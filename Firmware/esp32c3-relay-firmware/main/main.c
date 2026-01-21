/*
 * ESP32-C3 Relay Module - Main Application
 *
 * Story 1.1: GPIO Initialization and Project Foundation
 * Story 1.2: Relay Control Component Integration
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "gpio_config.h"
#include "relay_control.h"

static const char *TAG = "MAIN";

/**
 * Initialize non-relay GPIO pins (LEDs, button)
 *
 * NOTE: Relay initialization is handled separately by relay_control component
 * and MUST be called first in app_main() for safety (NFR1).
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t gpio_init_peripherals(void) {
    esp_err_t ret;

    // ============================================================================
    // Configure LED outputs (GPIO0, GPIO10)
    // ============================================================================

    // Configure GPIO0 (Error LED) as output
    ret = gpio_set_direction(PIN_LED_ERROR, GPIO_MODE_OUTPUT);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure error LED GPIO: %s", esp_err_to_name(ret));
        relay_force_off();  // Safety first
        return ret;
    }
    gpio_set_level(PIN_LED_ERROR, 0);  // LED off initially
    ESP_LOGI(TAG, "Error LED GPIO0 configured");

    // Configure GPIO10 (Status LED) as output
    ret = gpio_set_direction(PIN_LED_STATUS, GPIO_MODE_OUTPUT);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure status LED GPIO: %s", esp_err_to_name(ret));
        relay_force_off();  // Safety first
        return ret;
    }
    gpio_set_level(PIN_LED_STATUS, 0);  // LED off initially
    ESP_LOGI(TAG, "Status LED GPIO10 configured");

    // ============================================================================
    // Configure button input (GPIO9) with internal pull-up
    // ============================================================================

    // Configure GPIO9 (Button) as input with pull-up
    ret = gpio_set_direction(PIN_BUTTON, GPIO_MODE_INPUT);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure button GPIO direction: %s", esp_err_to_name(ret));
        relay_force_off();  // Safety first
        return ret;
    }

    ret = gpio_set_pull_mode(PIN_BUTTON, GPIO_PULLUP_ONLY);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure button GPIO pull-up: %s", esp_err_to_name(ret));
        relay_force_off();  // Safety first
        return ret;
    }
    ESP_LOGI(TAG, "Button GPIO9 configured (active-low with pull-up)");

    return ESP_OK;
}

/**
 * GPIO test task: Blink status LED, log button state, and test relay
 */
void gpio_test_task(void *pvParameters) {
    bool led_state = false;
    int cycle_count = 0;

    ESP_LOGI(TAG, "Starting GPIO test - Status LED will blink, relay will cycle every 10s");

    while (1) {
        // Toggle status LED
        led_state = !led_state;
        gpio_set_level(PIN_LED_STATUS, led_state);
        ESP_LOGI(TAG, "Status LED: %s", led_state ? "ON" : "OFF");

        // Read and log button state
        int button_state = gpio_get_level(PIN_BUTTON);
        ESP_LOGI(TAG, "Button state: %s (raw level: %d)",
                 button_state == 0 ? "PRESSED" : "RELEASED", button_state);

        // Log current relay state
        ESP_LOGI(TAG, "Relay state: %s", relay_get_state() ? "ON" : "OFF");

        // Test relay cycling every 10 seconds (5 cycles * 2 seconds per LED toggle)
        cycle_count++;
        if (cycle_count >= 5) {
            cycle_count = 0;
            bool current_relay = relay_get_state();
            esp_err_t ret = relay_set_state(!current_relay);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to toggle relay");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(2000));  // 2 second delay
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "ESP32-C3 Relay Module - Starting initialization");

    // ============================================================================
    // CRITICAL: Initialize relay FIRST to ensure fail-safe OFF state (NFR1)
    // This is the MANDATORY first action in app_main() - non-negotiable
    // ============================================================================

    esp_err_t ret = relay_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Relay initialization failed: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "CRITICAL ERROR - System halted");
        // Relay is already forced OFF by relay_init() on failure
        // In production, trigger watchdog reset here
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    ESP_LOGI(TAG, "Relay control initialized (fail-safe default OFF)");

    // ============================================================================
    // Initialize other peripherals (LEDs, button)
    // ============================================================================

    ret = gpio_init_peripherals();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Peripheral GPIO initialization failed: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "CRITICAL ERROR - System halted");
        // relay_force_off() already called in gpio_init_peripherals() on failure
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    ESP_LOGI(TAG, "Peripheral GPIO initialization complete");

    // Create GPIO test task
    xTaskCreate(gpio_test_task, "gpio_test", 2048, NULL, 5, NULL);

    ESP_LOGI(TAG, "Firmware initialized successfully");
}
