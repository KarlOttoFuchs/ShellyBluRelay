/*
 * ESP32-C3 Relay Module - Main Application
 *
 * Story 1.1: GPIO Initialization and Project Foundation
 * Story 1.2: Relay Control Component Integration
 * Story 1.3: Button Input Component Integration
 * Story 1.4: LED Control Component Integration
 * Story 1.5: Serial Protocol Foundation & Hardware Test Suite
 * Story 1.7: Button Short-Press Detection
 * Story 1.8: LED Counted Blink Function
 * Story 1.9: Timer Preset Cycling Logic
 * Story 2.1: Initialize BLE Stack & Scan for Advertising Packets
 * Story 2.2: Implement BTHome v2 Parser with Handler Registry
 * Story 3.1: NVS Storage Component with CRC Validation
 * Story 3.2: Implement 30-Second Learning Mode
 * Story 4B.4: Boot Reason Tracking
 * Story 4B.5: Watchdog Timer Implementation
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_task_wdt.h"  // Story 4B.5: Watchdog timer
#include "gpio_config.h"
#include "relay_control.h"
#include "button_input.h"
#include "led_control.h"
#include "serial_protocol.h"
#include "ble_scanner.h"
#include "bthome_parser.h"
#include "nvs_storage.h"
#include "nvs_flash.h"  // For ESP_ERR_NVS_NOT_FOUND
#include "state_machine.h"
#include "learning_mode.h"
#include "boot_reason.h"  // Story 4B.4: Boot reason tracking
#include "error_log.h"  // Story 5.2: Error logging
#include "firmware_version.h"  // Story 5.4: Firmware version
#include "timer_presets.h"  // Story 1.9: Timer preset cycling

static const char *TAG = "MAIN";

// Forward declaration for main loop task
static void main_loop_task(void *pvParameters);

void app_main(void) {
    // ============================================================================
    // Story 4B.4: Initialize boot reason tracking IMMEDIATELY at startup (AC1, AC2)
    // This must happen before any other logging to capture the true reset reason
    // ============================================================================
    boot_reason_init();

    // ============================================================================
    // Story 5.2: Initialize error log component
    // ============================================================================
    error_log_init();

    ESP_LOGI(TAG, "ESP32-C3 Relay Module - Starting initialization");

    // Story 5.4 AC3: Log firmware version at startup
    ESP_LOGI(TAG, "Firmware version: %s", get_firmware_version());

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
    // Initialize NVS storage component (Story 2.4, 3.1)
    // ============================================================================

    ret = nvs_storage_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NVS storage initialization failed: %s", esp_err_to_name(ret));
        ESP_LOGE(TAG, "CRITICAL ERROR - System halted");
        relay_force_off();  // Safety first
        led_set_pattern(LED_ERROR, LED_PATTERN_ERROR_DOUBLE);  // Double blink error pattern (FR4)
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    ESP_LOGI(TAG, "NVS storage initialized");

    // ============================================================================
    // Load sensor configuration from NVS and set initial state (Story 3.1, 3.2)
    // ============================================================================

    sensor_config_t config;
    ret = nvs_load_config(&config);

    if (ret == ESP_OK) {
        // Config loaded successfully with valid CRC
        ESP_LOGI(TAG, "Config loaded: MAC=%s, Type=%d, Timer=%ds, Retrigger=%d",
                 config.sensor_mac, config.sensor_type, config.timer_seconds, config.retrigger_mode);
        // Enter LISTENING state
        state_set(STATE_LISTENING);
        led_set_pattern(LED_STATUS, LED_PATTERN_BLINK_SLOW);
    } else if (ret == ESP_ERR_NVS_NOT_FOUND) {
        // No config stored - normal for fresh device
        ESP_LOGI(TAG, "No sensor config found - entering unconfigured mode");
        state_set(STATE_UNCONFIGURED);
        led_set_pattern(LED_STATUS, LED_PATTERN_OFF);
    } else if (ret == ESP_ERR_INVALID_CRC) {
        // CRC mismatch - nvs_load_config() already called relay_force_off() and set error LED
        // AC11: Log exact message per specification
        ESP_LOGE(TAG, "Config corrupted, relay disabled");
        // AC12: Error LED shows double blink pattern (already set by nvs_load_config)
        // AC13: Relay remains OFF (already forced off by nvs_load_config - fail-safe per NFR1)
        // AC14: State set to UNCONFIGURED allows recovery via CLEAR_SENSOR command
        state_set(STATE_UNCONFIGURED);
    } else {
        // Other NVS error
        ESP_LOGE(TAG, "Config load failed: %s", esp_err_to_name(ret));
        relay_force_off();  // Safety first
        led_set_pattern(LED_ERROR, LED_PATTERN_ERROR_DOUBLE);
        state_set(STATE_UNCONFIGURED);
        // Continue in degraded mode - don't halt
    }

    // ============================================================================
    // Story 1.9: Initialize timer presets component
    // ============================================================================

    ret = timer_presets_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Timer presets initialization failed: %s", esp_err_to_name(ret));
        // Non-critical - continue anyway
    }

    ESP_LOGI(TAG, "Timer presets initialized");

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
        error_log_add(ERROR_CODE_BLE_INIT_FAIL, "BLE stack initialization failed");  // Story 5.2
        ESP_LOGE(TAG, "CRITICAL ERROR - System halted");
        relay_force_off();  // Safety first (already called in ble_scanner_init, but be explicit)
        led_set_pattern(LED_ERROR, LED_PATTERN_ERROR_TRIPLE);  // Triple blink error pattern (FR4)
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    ESP_LOGI(TAG, "BLE scanner initialized");

    // ============================================================================
    // Initialize learning mode subsystem (Story 3.2)
    // ============================================================================

    ret = learning_mode_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Learning mode initialization failed: %s", esp_err_to_name(ret));
        // Non-critical - continue anyway
    }

    // Register learning mode callback with BTHome parser
    bthome_set_learning_callback(register_captured_sensor);

    ESP_LOGI(TAG, "Learning mode initialized");

    // ============================================================================
    // Story 4B.5: Configure Task Watchdog Timer (TWDT) - 10 second timeout (AC1)
    // ============================================================================

    esp_task_wdt_config_t twdt_config = {
        .timeout_ms = 10000,        // 10 seconds per NFR2
        .idle_core_mask = 0,        // Don't monitor idle tasks
        .trigger_panic = true,      // Reset on timeout (AC4)
    };

    ret = esp_task_wdt_reconfigure(&twdt_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "TWDT reconfigure failed: %s", esp_err_to_name(ret));
        ESP_LOGW(TAG, "Watchdog timer not configured - system may not recover from hangs");
    } else {
        ESP_LOGI(TAG, "Watchdog timer configured (10s timeout)");
    }

    // ============================================================================
    // Start main loop task for button monitoring and learning mode processing
    // ============================================================================

    BaseType_t task_ret = xTaskCreate(
        main_loop_task,
        "main_loop",
        4096,   // Stack size
        NULL,
        5,      // Priority (same as button in Story 1.3)
        NULL
    );

    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create main loop task");
        // Non-critical for basic operation, but learning mode won't work
    }

    ESP_LOGI(TAG, "============================================");
    ESP_LOGI(TAG, "Firmware initialized successfully");
    ESP_LOGI(TAG, "Hardware test commands available via serial");
    ESP_LOGI(TAG, "Type HELP for available commands");
    ESP_LOGI(TAG, "Short-press button (<500ms) to cycle timer presets");
    ESP_LOGI(TAG, "Long-press button (2s) to enter learning mode");
    ESP_LOGI(TAG, "============================================");
}

/**
 * Main loop task - handles button monitoring and learning mode processing
 *
 * Story 1.9: Processes timer preset cycling (short-press button interaction)
 * Story 3.2: Checks for button long-press to enter learning mode,
 * and processes captured sensors when in learning mode.
 * Story 4B.5: Feeds watchdog timer to prevent system reset (AC2, AC3)
 */
static void main_loop_task(void *pvParameters)
{
    (void)pvParameters;

    ESP_LOGI(TAG, "Main loop task started");

    // Story 4B.5 AC2: Subscribe this task to TWDT monitoring
    esp_err_t wdt_ret = esp_task_wdt_add(NULL);
    if (wdt_ret == ESP_OK) {
        ESP_LOGI(TAG, "Main loop subscribed to watchdog");
    } else {
        ESP_LOGW(TAG, "Failed to subscribe to watchdog: %s", esp_err_to_name(wdt_ret));
    }

    while (1) {
        // Story 1.9: Process timer preset button interactions
        // Handles short-press detection and LED feedback via Stories 1.7, 1.8
        timer_presets_process();

        // Check for button long-press to enter learning mode
        if (button_check_long_press()) {
            // Only enter learning mode if not already in it
            if (state_get_current() != STATE_LEARNING) {
                enter_learning_mode();
            }
        }

        // Process any captured sensor during learning mode
        learning_mode_process_capture();

        // Story 4B.5 AC3: Reset watchdog timer to signal normal operation
        esp_task_wdt_reset();

        // Short delay to prevent busy-loop
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
