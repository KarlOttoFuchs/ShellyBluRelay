/*
 * ESP32-C3 Relay Module - Main Application
 *
 * Story 1.1: GPIO Initialization and Project Foundation
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "gpio_config.h"

static const char *TAG = "MAIN";

/**
 * Initialize all GPIO pins according to hardware configuration
 *
 * CRITICAL: Relay GPIO (GPIO7) MUST be configured FIRST and set to LOW
 * to ensure fail-safe relay-OFF state (NFR1 requirement)
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t gpio_init_all(void) {
    esp_err_t ret;

    // ============================================================================
    // CRITICAL SAFETY REQUIREMENT (NFR1): Configure relay GPIO FIRST
    // Relay MUST default to OFF in 100% of error conditions
    // ============================================================================

    // Configure GPIO7 (Relay) as output
    ret = gpio_set_direction(PIN_RELAY, GPIO_MODE_OUTPUT);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure relay GPIO: %s", esp_err_to_name(ret));
        return ret;
    }

    // Set relay to OFF state (LOW) immediately
    gpio_set_level(PIN_RELAY, 0);
    ESP_LOGI(TAG, "Relay GPIO7 configured (default OFF - fail-safe)");

    // ============================================================================
    // Configure LED outputs (GPIO0, GPIO10)
    // ============================================================================

    // Configure GPIO0 (Error LED) as output
    ret = gpio_set_direction(PIN_LED_ERROR, GPIO_MODE_OUTPUT);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure error LED GPIO: %s", esp_err_to_name(ret));
        return ret;
    }
    gpio_set_level(PIN_LED_ERROR, 0);  // LED off initially
    ESP_LOGI(TAG, "Error LED GPIO0 configured");

    // Configure GPIO10 (Status LED) as output
    ret = gpio_set_direction(PIN_LED_STATUS, GPIO_MODE_OUTPUT);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure status LED GPIO: %s", esp_err_to_name(ret));
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
        return ret;
    }

    ret = gpio_set_pull_mode(PIN_BUTTON, GPIO_PULLUP_ONLY);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure button GPIO pull-up: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "Button GPIO9 configured (active-low with pull-up)");

    return ESP_OK;
}

/**
 * Simple GPIO test: Blink status LED and log button state
 */
void gpio_test_task(void *pvParameters) {
    bool led_state = false;

    ESP_LOGI(TAG, "Starting GPIO test - Status LED will blink");

    while (1) {
        // Toggle status LED
        led_state = !led_state;
        gpio_set_level(PIN_LED_STATUS, led_state);
        ESP_LOGI(TAG, "Status LED: %s", led_state ? "ON" : "OFF");

        // Read and log button state
        int button_state = gpio_get_level(PIN_BUTTON);
        ESP_LOGI(TAG, "Button state: %s (raw level: %d)",
                 button_state == 0 ? "PRESSED" : "RELEASED", button_state);

        // Verify relay is still OFF (safety check)
        int relay_state = gpio_get_level(PIN_RELAY);
        if (relay_state != 0) {
            ESP_LOGE(TAG, "WARNING: Relay is not in OFF state! Level: %d", relay_state);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));  // 1 second delay
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "ESP32-C3 Relay Module - Starting initialization");

    // ============================================================================
    // CRITICAL: Initialize relay GPIO FIRST to ensure fail-safe OFF state
    // This is the mandatory first action in app_main() (NFR1 requirement)
    // ============================================================================

    esp_err_t ret = gpio_init_all();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GPIO initialization failed: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "CRITICAL ERROR - System halted");
        // In production, trigger watchdog reset here
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    ESP_LOGI(TAG, "GPIO initialization complete");
    ESP_LOGI(TAG, "Relay forced OFF (fail-safe default)");

    // Create GPIO test task
    xTaskCreate(gpio_test_task, "gpio_test", 2048, NULL, 5, NULL);

    ESP_LOGI(TAG, "Firmware initialized successfully");
}
