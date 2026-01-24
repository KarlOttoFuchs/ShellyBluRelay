/*
 * Learning Mode - Implementation
 *
 * Story 3.2: Implement 30-Second Learning Mode
 */

#include "learning_mode.h"
#include "state_machine.h"
#include "led_control.h"
#include "nvs_storage.h"
#include "relay_control.h"
#include "ble_scanner.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>

static const char *TAG = "LEARNING";

// Learning mode timer
static esp_timer_handle_t learning_timer = NULL;

// Learning mode start timestamp (milliseconds)
static uint32_t learning_mode_start_ms = 0;

// Captured sensor data (set by BLE callback, processed in main context)
static bool sensor_captured = false;
static uint8_t captured_mac[6];
static uint8_t captured_sensor_type;
static uint8_t captured_battery_pct;
static int8_t captured_rssi;

// LED pattern before entering learning mode (for restoration on timeout)
static led_pattern_t previous_led_pattern = LED_PATTERN_OFF;

// Forward declaration
static void learning_mode_timeout_callback(void *arg);

/**
 * Sensor type names for logging
 */
static const char* sensor_type_names[] = {
    "NONE",     // 0
    "BUTTON",   // 1
    "MOTION",   // 2
    "DOOR",     // 3
};

esp_err_t learning_mode_init(void)
{
    // Timer will be created when entering learning mode
    learning_timer = NULL;
    learning_mode_start_ms = 0;
    sensor_captured = false;

    ESP_LOGI(TAG, "Learning mode subsystem initialized");
    return ESP_OK;
}

void enter_learning_mode(void)
{
    // Check if already in learning mode
    if (state_get_current() == STATE_LEARNING) {
        ESP_LOGW(TAG, "Already in learning mode");
        return;
    }

    // Save current LED pattern for potential restoration on timeout
    led_get_pattern(LED_STATUS, &previous_led_pattern);

    // Set state to learning
    state_set(STATE_LEARNING);

    // Record start time
    learning_mode_start_ms = (uint32_t)(esp_timer_get_time() / 1000);

    // Clear any previous captured sensor data
    sensor_captured = false;

    // Create and start 30-second timer
    if (learning_timer != NULL) {
        // Clean up any existing timer
        esp_timer_stop(learning_timer);
        esp_timer_delete(learning_timer);
        learning_timer = NULL;
    }

    esp_timer_create_args_t timer_args = {
        .callback = learning_mode_timeout_callback,
        .name = "learning_timer"
    };

    esp_err_t ret = esp_timer_create(&timer_args, &learning_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create learning timer: %s", esp_err_to_name(ret));
        state_set(STATE_UNCONFIGURED);
        return;
    }

    ret = esp_timer_start_once(learning_timer, LEARNING_MODE_DURATION_MS * 1000ULL);  // Convert ms to us
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start learning timer: %s", esp_err_to_name(ret));
        esp_timer_delete(learning_timer);
        learning_timer = NULL;
        state_set(STATE_UNCONFIGURED);
        return;
    }

    // Set LED to fast blink pattern (500ms on/off per FR3)
    led_set_pattern(LED_STATUS, LED_PATTERN_BLINK_FAST);

    ESP_LOGI(TAG, "Entering learning mode for 30 seconds");
}

void exit_learning_mode(void)
{
    // Stop and delete timer if running
    if (learning_timer != NULL) {
        esp_timer_stop(learning_timer);
        esp_timer_delete(learning_timer);
        learning_timer = NULL;
    }

    // Clear start time
    learning_mode_start_ms = 0;

    // Clear captured sensor data
    sensor_captured = false;
}

bool is_learning_mode_active(void)
{
    return (state_get_current() == STATE_LEARNING);
}

uint32_t learning_mode_time_remaining_sec(void)
{
    if (state_get_current() != STATE_LEARNING || learning_mode_start_ms == 0) {
        return 0;
    }

    uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);
    uint32_t elapsed_ms = now_ms - learning_mode_start_ms;

    if (elapsed_ms >= LEARNING_MODE_DURATION_MS) {
        return 0;
    }

    return (LEARNING_MODE_DURATION_MS - elapsed_ms) / 1000;
}

uint32_t learning_mode_get_start_ms(void)
{
    return learning_mode_start_ms;
}

/**
 * Learning mode timeout callback
 *
 * Called by esp_timer when 30 seconds expires without sensor capture.
 * NOTE: This runs in timer task context, not main task.
 */
static void learning_mode_timeout_callback(void *arg)
{
    (void)arg;

    // Check if still in learning mode (may have already registered)
    if (state_get_current() != STATE_LEARNING) {
        ESP_LOGD(TAG, "Timeout callback: not in learning mode (already exited)");
        return;
    }

    ESP_LOGI(TAG, "Learning mode timeout, no sensor detected");

    // Exit learning mode (clean up timer - but it's already fired)
    learning_mode_start_ms = 0;
    sensor_captured = false;

    // Delete timer handle (already fired, just cleanup)
    if (learning_timer != NULL) {
        esp_timer_delete(learning_timer);
        learning_timer = NULL;
    }

    // Determine appropriate state and LED pattern based on config
    sensor_config_t config;
    esp_err_t ret = nvs_load_config(&config);

    if (ret == ESP_OK) {
        // Config exists - return to listening state
        state_set(STATE_LISTENING);
        led_set_pattern(LED_STATUS, LED_PATTERN_BLINK_SLOW);
    } else {
        // No config - return to unconfigured state
        state_set(STATE_UNCONFIGURED);
        led_set_pattern(LED_STATUS, LED_PATTERN_OFF);
    }
}

void register_captured_sensor(const uint8_t *mac, uint8_t sensor_type,
                               uint8_t battery_pct, int8_t rssi)
{
    // Only capture when in learning mode
    if (state_get_current() != STATE_LEARNING) {
        return;
    }

    if (mac == NULL || sensor_type == 0 || sensor_type > 3) {
        ESP_LOGE(TAG, "Invalid sensor data for registration");
        return;
    }

    // Store captured sensor data for processing in main context
    // (This may be called from BLE callback context)
    memcpy(captured_mac, mac, 6);
    captured_sensor_type = sensor_type;
    captured_battery_pct = battery_pct;
    captured_rssi = rssi;
    sensor_captured = true;

    // Log capture immediately
    ESP_LOGI(TAG, "Sensor captured: %02X:%02X:%02X:%02X:%02X:%02X (%s) RSSI=%d",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
             sensor_type <= 3 ? sensor_type_names[sensor_type] : "UNKNOWN",
             rssi);
}

void learning_mode_process_capture(void)
{
    // Check if a sensor was captured and we're in learning mode
    if (!sensor_captured || state_get_current() != STATE_LEARNING) {
        return;
    }

    // Reset capture flag first to avoid re-processing
    sensor_captured = false;

    // Build config struct
    sensor_config_t config;
    nvs_init_config_defaults(&config);

    // Format MAC address (uppercase with colons)
    snprintf(config.sensor_mac, sizeof(config.sensor_mac),
             "%02X:%02X:%02X:%02X:%02X:%02X",
             captured_mac[0], captured_mac[1], captured_mac[2],
             captured_mac[3], captured_mac[4], captured_mac[5]);

    // Set sensor type
    config.sensor_type = captured_sensor_type;

    // Set defaults per Story 3.2 spec
    config.timer_seconds = LEARNING_DEFAULT_TIMER_SECONDS;  // 10 seconds
    config.retrigger_mode = RETRIGGER_EXTEND;
    config.config_version = CONFIG_VERSION;
    // config_crc will be calculated by nvs_save_config()

    // Save to NVS
    esp_err_t ret = nvs_save_config(&config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save sensor config: %s", esp_err_to_name(ret));
        // Stay in learning mode to allow retry
        return;
    }

    // Exit learning mode (stops timer)
    exit_learning_mode();

    // Invalidate BLE MAC cache so scanner picks up new registration
    ble_scanner_invalidate_mac_cache();

    // Update state to listening
    state_set(STATE_LISTENING);

    // Set LED to slow blink (listening mode per FR3)
    led_set_pattern(LED_STATUS, LED_PATTERN_BLINK_SLOW);

    // Log successful registration
    const char* type_name = captured_sensor_type <= 3 ?
                            sensor_type_names[captured_sensor_type] : "UNKNOWN";
    ESP_LOGI(TAG, "Sensor registered: %s (%s)", config.sensor_mac, type_name);
}
