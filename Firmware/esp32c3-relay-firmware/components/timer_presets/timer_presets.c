/*
 * Timer Presets Component - Implementation
 *
 * Story 1.9: Timer Preset Cycling Logic
 *
 * State Machine:
 *   IDLE -> [short press in LISTENING] -> CYCLING -> [3s timeout] -> SAVING -> IDLE
 *
 * Uses:
 *   - button_check_short_press() from Story 1.7
 *   - led_blink_count() from Story 1.8
 */

#include "timer_presets.h"
#include "button_input.h"
#include "led_control.h"
#include "nvs_storage.h"
#include "state_machine.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "TIMER_PRESET";

// Preset values array
static const uint16_t TIMER_PRESETS[TIMER_PRESET_COUNT] = {
    TIMER_PRESET_1,  // 30 seconds  (1 blink)
    TIMER_PRESET_2,  // 60 seconds  (2 blinks)
    TIMER_PRESET_3,  // 120 seconds (3 blinks)
    TIMER_PRESET_4   // 300 seconds (4 blinks)
};

// State machine states
typedef enum {
    PRESET_STATE_IDLE,      // Not actively cycling presets
    PRESET_STATE_CYCLING,   // User is cycling through presets
} preset_state_t;

// Module state
static bool preset_initialized = false;
static uint8_t current_index = TIMER_PRESET_COUNT - 1;  // Default to 300s (index 3)
static preset_state_t preset_state = PRESET_STATE_IDLE;
static uint32_t timeout_start_ms = 0;
static bool blink_in_progress = false;

/**
 * Callback when LED blinks complete
 */
static void on_blink_complete(void)
{
    blink_in_progress = false;
    ESP_LOGD(TAG, "Blink complete, waiting for next press or timeout");
}

/**
 * Save current preset to NVS
 */
static void save_preset_to_nvs(void)
{
    sensor_config_t config;

    // Load existing config
    esp_err_t ret = nvs_load_config(&config);
    if (ret != ESP_OK) {
        // If no config exists, initialize defaults
        nvs_init_config_defaults(&config);
    }

    // Update timer value
    config.timer_seconds = TIMER_PRESETS[current_index];

    // Save back to NVS
    ret = nvs_save_config(&config);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Timer preset saved: %d seconds (%d blinks)",
                 TIMER_PRESETS[current_index], current_index + 1);
    } else {
        ESP_LOGE(TAG, "Failed to save timer preset: %s", esp_err_to_name(ret));
    }
}

esp_err_t timer_presets_init(void)
{
    if (preset_initialized) {
        ESP_LOGW(TAG, "Timer presets already initialized");
        return ESP_OK;
    }

    // Load current timer value from NVS and find matching preset index
    sensor_config_t config;
    esp_err_t ret = nvs_load_config(&config);

    if (ret == ESP_OK) {
        // Find matching preset index
        bool found = false;
        for (int i = 0; i < TIMER_PRESET_COUNT; i++) {
            if (TIMER_PRESETS[i] == config.timer_seconds) {
                current_index = i;
                found = true;
                break;
            }
        }

        if (!found) {
            // Value doesn't match any preset, default to 300s
            current_index = TIMER_PRESET_COUNT - 1;
            ESP_LOGW(TAG, "Timer value %d doesn't match presets, defaulting to index %d",
                     config.timer_seconds, current_index);
        }
    } else {
        // No config or error, use default
        current_index = TIMER_PRESET_COUNT - 1;  // 300 seconds
    }

    preset_state = PRESET_STATE_IDLE;
    preset_initialized = true;

    ESP_LOGI(TAG, "Timer presets initialized, current index: %d (%d seconds)",
             current_index, TIMER_PRESETS[current_index]);

    return ESP_OK;
}

void timer_presets_process(void)
{
    if (!preset_initialized) {
        return;
    }

    uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);

    // Check for timeout in CYCLING state
    if (preset_state == PRESET_STATE_CYCLING) {
        uint32_t elapsed = now_ms - timeout_start_ms;

        if (elapsed >= TIMER_PRESET_TIMEOUT_MS) {
            // Timeout - save and return to idle
            save_preset_to_nvs();
            preset_state = PRESET_STATE_IDLE;
            // Restore slow blink pattern for LISTENING state
            led_set_pattern(LED_STATUS, LED_PATTERN_BLINK_SLOW);
            ESP_LOGI(TAG, "Preset confirmed after timeout");
            return;
        }
    }

    // Check for short press (only in LISTENING state)
    if (button_check_short_press()) {
        if (state_get_current() != STATE_LISTENING) {
            ESP_LOGD(TAG, "Short press ignored - not in LISTENING state");
            return;
        }

        // Don't process if blink is still in progress
        if (blink_in_progress) {
            ESP_LOGD(TAG, "Short press ignored - blink in progress");
            return;
        }

        if (preset_state == PRESET_STATE_IDLE) {
            // First press - turn off slow blink and show current preset
            led_set_pattern(LED_STATUS, LED_PATTERN_OFF);
            ESP_LOGI(TAG, "Showing current preset: %d (%d seconds)",
                     current_index + 1, TIMER_PRESETS[current_index]);
        } else {
            // Subsequent press - advance to next preset
            current_index = (current_index + 1) % TIMER_PRESET_COUNT;
            ESP_LOGI(TAG, "Advanced to preset: %d (%d seconds)",
                     current_index + 1, TIMER_PRESETS[current_index]);
        }

        // Show preset with LED blinks (index + 1 = blink count)
        blink_in_progress = true;
        led_blink_count(LED_STATUS, current_index + 1, on_blink_complete);

        // Start/reset timeout
        timeout_start_ms = now_ms;
        preset_state = PRESET_STATE_CYCLING;
    }
}

uint8_t timer_presets_get_current_index(void)
{
    return current_index;
}

uint16_t timer_presets_get_value(uint8_t index)
{
    if (index >= TIMER_PRESET_COUNT) {
        return TIMER_PRESET_4;  // Default to 300 seconds
    }
    return TIMER_PRESETS[index];
}
