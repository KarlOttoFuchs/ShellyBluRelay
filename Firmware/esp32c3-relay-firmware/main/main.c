/*
 * ESP32-C3 Relay Module - Main Application
 *
 * Story 1.1: GPIO Initialization and Project Foundation
 * Story 1.2: Relay Control Component Integration
 * Story 1.3: Button Input Component Integration
 * Story 1.4: LED Control Component Integration
 * Story 1.5: Serial Protocol Foundation & Hardware Test Suite
 * Story 2.1: Initialize BLE Stack & Scan for Advertising Packets
 * Story 2.2: Implement BTHome v2 Parser with Handler Registry
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "gpio_config.h"
#include "relay_control.h"
#include "button_input.h"
#include "led_control.h"
#include "serial_protocol.h"
#include "ble_scanner.h"
#include "bthome_parser.h"

static const char *TAG = "MAIN";

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
    // Initialize button input component
    // ============================================================================

    ret = button_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Button initialization failed: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "CRITICAL ERROR - System halted");
        relay_force_off();  // Safety first
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    ESP_LOGI(TAG, "Button input initialized");

    // ============================================================================
    // Initialize LED control component
    // ============================================================================

    ret = led_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LED control initialization failed: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "CRITICAL ERROR - System halted");
        relay_force_off();  // Safety first
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    ESP_LOGI(TAG, "LED control initialized");

    // Set status LED to slow blink pattern (listening mode indicator)
    led_set_pattern(LED_STATUS, LED_PATTERN_BLINK_SLOW);

    // ============================================================================
    // Initialize serial protocol component
    // ============================================================================

    ret = serial_protocol_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Serial protocol initialization failed: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "CRITICAL ERROR - System halted");
        relay_force_off();  // Safety first
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    ESP_LOGI(TAG, "Serial protocol initialized (115200 baud)");

    // ============================================================================
    // Initialize BTHome parser component (Story 2.2)
    // ============================================================================

    ret = bthome_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BTHome parser initialization failed: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "CRITICAL ERROR - System halted");
        relay_force_off();  // Safety first
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    ESP_LOGI(TAG, "BTHome parser initialized");

    // ============================================================================
    // Initialize BLE scanner component
    // ============================================================================

    ret = ble_scanner_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BLE scanner initialization failed: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "CRITICAL ERROR - System halted");
        relay_force_off();  // Safety first (already called in ble_scanner_init, but be explicit)
        led_set_pattern(LED_ERROR, LED_PATTERN_ERROR_TRIPLE);  // Triple blink error pattern (FR4)
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    ESP_LOGI(TAG, "BLE scanner initialized");

    ESP_LOGI(TAG, "============================================");
    ESP_LOGI(TAG, "Firmware initialized successfully");
    ESP_LOGI(TAG, "Hardware test commands available via serial");
    ESP_LOGI(TAG, "Type HELP for available commands");
    ESP_LOGI(TAG, "============================================");
}
