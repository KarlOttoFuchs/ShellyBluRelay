/*
 * Serial Protocol Component - Implementation
 *
 * Story 1.5: Serial Protocol Foundation & Hardware Test Suite
 *
 * Implements text-based serial command protocol over USB Serial JTAG for hardware validation.
 * Protocol format per architecture.md specification.
 *
 * Uses ESP-IDF's USB Serial JTAG driver directly for reliable input on ESP32-C3.
 */

#include "serial_protocol.h"
#include "relay_control.h"
#include "relay_timer.h"
#include "led_control.h"
#include "button_input.h"
#include "ble_scanner.h"
#include "bthome_parser.h"
#include "sensor_button.h"
#include "nvs_storage.h"
#include "nvs_flash.h"  // For ESP_ERR_NVS_NOT_FOUND
#include "state_machine.h"
#include "learning_mode.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/usb_serial_jtag.h"
#include "esp_vfs_usb_serial_jtag.h"
#include "esp_vfs_dev.h"

#include <string.h>
#include <strings.h>  // For strcasecmp (POSIX)
#include <ctype.h>
#include <stdio.h>

static const char *TAG = "SERIAL_PROTO";

#define SERIAL_CMD_MAX_LEN  64      // Maximum command line length
#define SERIAL_RX_BUF_SIZE  256     // USB Serial JTAG RX buffer

// Module state
static bool serial_initialized = false;
static TaskHandle_t serial_task_handle = NULL;

// Forward declarations for command handlers
static esp_err_t cmd_ping(const char *args);
static esp_err_t cmd_status(const char *args);
static esp_err_t cmd_test_relay(const char *args);
static esp_err_t cmd_test_led(const char *args);
static esp_err_t cmd_test_button(const char *args);
static esp_err_t cmd_ble_scan(const char *args);
static esp_err_t cmd_ble_events(const char *args);
static esp_err_t cmd_test_register(const char *args);
static esp_err_t cmd_test_unregister(const char *args);
static esp_err_t cmd_test_save_config(const char *args);
static esp_err_t cmd_test_load_config(const char *args);
static esp_err_t cmd_help(const char *args);
static esp_err_t cmd_register_sensor(const char *args);
static esp_err_t cmd_clear_sensor(const char *args);
static esp_err_t cmd_set_timer(const char *args);

// Command handler function type
typedef esp_err_t (*cmd_handler_fn)(const char *args);

// Command definition structure
typedef struct {
    const char *command;
    cmd_handler_fn handler;
    const char *help;
} serial_command_t;

// Command registry - sentinel terminated
static const serial_command_t commands[] = {
    {"PING",           cmd_ping,           "PING - Test connectivity (responds: pong)"},
    {"STATUS",         cmd_status,         "STATUS - Show system state, config, and learning mode info"},
    {"TEST_RELAY",     cmd_test_relay,     "TEST_RELAY [ON|OFF] - Control relay"},
    {"TEST_LED",       cmd_test_led,       "TEST_LED [STATUS|ERROR] [ON|OFF|BLINK] - Control LEDs"},
    {"TEST_BUTTON",    cmd_test_button,    "TEST_BUTTON - Read button state"},
    {"BLE_SCAN",       cmd_ble_scan,       "BLE_SCAN - Show recently seen BLE devices"},
    {"BLE_EVENTS",     cmd_ble_events,     "BLE_EVENTS - Show last 10 sensor events"},
    {"TEST_REGISTER",  cmd_test_register,  "TEST_REGISTER <MAC> - Register sensor MAC (e.g., AA:BB:CC:DD:EE:FF)"},
    {"TEST_UNREGISTER",cmd_test_unregister,"TEST_UNREGISTER - Clear registered sensor"},
    {"TEST_SAVE_CONFIG", cmd_test_save_config, "TEST_SAVE_CONFIG <MAC> [timer] - Save full config with CRC (Story 3.1)"},
    {"TEST_LOAD_CONFIG", cmd_test_load_config, "TEST_LOAD_CONFIG - Load and display config with CRC validation"},
    {"REGISTER_SENSOR", cmd_register_sensor, "REGISTER_SENSOR <MAC> <TYPE> - Register sensor (e.g., REGISTER_SENSOR AA:BB:CC:DD:EE:FF BUTTON)"},
    {"CLEAR_SENSOR",   cmd_clear_sensor,   "CLEAR_SENSOR - Clear registered sensor configuration"},
    {"SET_TIMER",      cmd_set_timer,      "SET_TIMER <1-600> - Set relay timer duration in seconds"},
    {"HELP",           cmd_help,           "HELP - Show available commands"},
    {NULL, NULL, NULL}  // Sentinel
};

// ============================================================================
// Response Helpers - Use USB Serial JTAG directly
// ============================================================================

static void serial_write(const char *str, size_t len)
{
    usb_serial_jtag_write_bytes((const uint8_t *)str, len, pdMS_TO_TICKS(100));
}

void serial_send_ok(const char *data)
{
    char response[512];  // Increased for STATUS JSON responses
    int len = snprintf(response, sizeof(response), "OK|%s\n", data);
    serial_write(response, len);
}

void serial_send_error(const char *code, const char *message)
{
    char response[256];
    int len = snprintf(response, sizeof(response), "ERROR|%s|%s\n", code, message);
    serial_write(response, len);
}

void serial_send_raw(const char *str)
{
    serial_write(str, strlen(str));
}

// ============================================================================
// Command Handlers
// ============================================================================

/**
 * PING command handler
 * Responds with OK|pong to verify serial connectivity
 */
static esp_err_t cmd_ping(const char *args)
{
    (void)args;  // Unused
    serial_send_ok("pong");
    ESP_LOGI(TAG, "PING received, responded with pong");
    return ESP_OK;
}

/**
 * STATUS command handler (Story 3.2, updated Story 3.4, Story 4A.1)
 * Returns system state, sensor config, and learning mode status
 * Response format: OK|{"state":"...","sensor_registered":bool,"sensor_mac":"...","sensor_type":"..."}
 *
 * When unconfigured (AC7):
 * {"state":"unconfigured","sensor_registered":false,"sensor_mac":"","sensor_type":""}
 *
 * When ACTIVE (Story 4A.1 AC8):
 * {"state":"ACTIVE","relay":"on","timer_remaining":0,"sensor_mac":"...","sensor_type":"...","last_trigger_ms":...}
 */
static esp_err_t cmd_status(const char *args)
{
    (void)args;  // Unused

    char response[384];  // Increased for ACTIVE state fields
    system_state_t state = state_get_current();
    const char *state_str = state_to_string(state);

    // Load config if available
    sensor_config_t config;
    esp_err_t config_ret = nvs_load_config(&config);
    bool has_config = (config_ret == ESP_OK);

    // Build JSON response
    if (state == STATE_LEARNING) {
        // Learning mode: include time remaining
        uint32_t time_remaining = learning_mode_time_remaining_sec();

        if (has_config) {
            snprintf(response, sizeof(response),
                     "{\"state\":\"%s\",\"time_remaining_sec\":%lu,\"sensor_registered\":true,\"sensor_mac\":\"%s\",\"sensor_type\":%d,\"timer_seconds\":%d}",
                     state_str,
                     (unsigned long)time_remaining,
                     config.sensor_mac,
                     config.sensor_type,
                     config.timer_seconds);
        } else {
            snprintf(response, sizeof(response),
                     "{\"state\":\"%s\",\"time_remaining_sec\":%lu,\"sensor_registered\":false,\"sensor_mac\":\"\",\"sensor_type\":\"\"}",
                     state_str,
                     (unsigned long)time_remaining);
        }
    } else if (state == STATE_ACTIVE) {
        // ACTIVE state (Story 4A.1 AC8, Story 4A.2 AC3): include relay state, timer remaining, last trigger info
        bool relay_on = relay_get_state();
        uint16_t timer_remaining = relay_timer_get_remaining_sec();
        uint32_t last_trigger = relay_get_last_trigger_ms();
        char trigger_mac[18] = {0};
        relay_get_last_trigger_mac(trigger_mac, sizeof(trigger_mac));
        uint8_t trigger_sensor_type = relay_get_last_trigger_sensor_type();

        // Convert sensor type to string
        const char *sensor_type_str = "UNKNOWN";
        switch (trigger_sensor_type) {
            case 1: sensor_type_str = "BUTTON"; break;
            case 2: sensor_type_str = "MOTION"; break;
            case 3: sensor_type_str = "DOOR"; break;
        }

        snprintf(response, sizeof(response),
                 "{\"state\":\"%s\",\"relay\":\"%s\",\"timer_remaining\":%u,\"sensor_mac\":\"%s\",\"sensor_type\":\"%s\",\"last_trigger_ms\":%lu}",
                 state_str,
                 relay_on ? "on" : "off",
                 timer_remaining,
                 trigger_mac,
                 sensor_type_str,
                 (unsigned long)last_trigger);
    } else if (state == STATE_UNCONFIGURED || !has_config) {
        // Unconfigured state (AC7): sensor_registered=false, empty strings
        snprintf(response, sizeof(response),
                 "{\"state\":\"%s\",\"sensor_registered\":false,\"sensor_mac\":\"\",\"sensor_type\":\"\"}",
                 state_str);
    } else {
        // Configured state (LISTENING) - Story 4A.2 AC7: include relay and timer_remaining
        bool relay_on = relay_get_state();
        snprintf(response, sizeof(response),
                 "{\"state\":\"%s\",\"relay\":\"%s\",\"timer_remaining\":0,\"sensor_registered\":true,\"sensor_mac\":\"%s\",\"sensor_type\":%d,\"timer_seconds\":%d,\"retrigger_mode\":%d}",
                 state_str,
                 relay_on ? "on" : "off",
                 config.sensor_mac,
                 config.sensor_type,
                 config.timer_seconds,
                 config.retrigger_mode);
    }

    serial_send_ok(response);
    ESP_LOGI(TAG, "STATUS: state=%s", state_str);
    return ESP_OK;
}

/**
 * TEST_RELAY command handler
 * Controls relay state: TEST_RELAY ON or TEST_RELAY OFF
 */
static esp_err_t cmd_test_relay(const char *args)
{
    char state_arg[16] = {0};

    // Parse argument
    if (args == NULL || args[0] == '\0') {
        serial_send_error("invalid_argument", "Usage: TEST_RELAY [ON|OFF]");
        return ESP_ERR_INVALID_ARG;
    }

    // Copy and convert to uppercase
    strncpy(state_arg, args, sizeof(state_arg) - 1);
    for (char *p = state_arg; *p; p++) {
        *p = toupper((unsigned char)*p);
    }

    // Trim trailing whitespace
    size_t len = strlen(state_arg);
    while (len > 0 && isspace((unsigned char)state_arg[len - 1])) {
        state_arg[--len] = '\0';
    }

    if (strcmp(state_arg, "ON") == 0) {
        esp_err_t ret = relay_set_state(true);
        if (ret == ESP_OK) {
            serial_send_ok("relay_on");
            ESP_LOGI(TAG, "TEST_RELAY ON executed");
        } else {
            serial_send_error("relay_error", "Failed to set relay ON");
            return ret;
        }
    } else if (strcmp(state_arg, "OFF") == 0) {
        esp_err_t ret = relay_set_state(false);
        if (ret == ESP_OK) {
            serial_send_ok("relay_off");
            ESP_LOGI(TAG, "TEST_RELAY OFF executed");
        } else {
            serial_send_error("relay_error", "Failed to set relay OFF");
            return ret;
        }
    } else {
        serial_send_error("invalid_argument", "Relay state must be ON or OFF");
        return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}

/**
 * TEST_LED command handler
 * Controls LED state: TEST_LED [STATUS|ERROR] [ON|OFF|BLINK]
 */
static esp_err_t cmd_test_led(const char *args)
{
    char led_arg[16] = {0};
    char state_arg[16] = {0};

    // Parse arguments
    if (args == NULL || sscanf(args, "%15s %15s", led_arg, state_arg) != 2) {
        serial_send_error("invalid_argument", "Usage: TEST_LED [STATUS|ERROR] [ON|OFF|BLINK]");
        return ESP_ERR_INVALID_ARG;
    }

    // Convert to uppercase
    for (char *p = led_arg; *p; p++) {
        *p = toupper((unsigned char)*p);
    }
    for (char *p = state_arg; *p; p++) {
        *p = toupper((unsigned char)*p);
    }

    // Determine LED
    led_id_t led;
    if (strcmp(led_arg, "STATUS") == 0) {
        led = LED_STATUS;
    } else if (strcmp(led_arg, "ERROR") == 0) {
        led = LED_ERROR;
    } else {
        serial_send_error("invalid_argument", "LED must be STATUS or ERROR");
        return ESP_ERR_INVALID_ARG;
    }

    // Determine state/pattern and execute
    esp_err_t ret;
    if (strcmp(state_arg, "ON") == 0) {
        ret = led_set_state(led, true);
        if (ret == ESP_OK) {
            serial_send_ok("led_on");
            ESP_LOGI(TAG, "TEST_LED %s ON executed", led_arg);
        }
    } else if (strcmp(state_arg, "OFF") == 0) {
        ret = led_set_state(led, false);
        if (ret == ESP_OK) {
            serial_send_ok("led_off");
            ESP_LOGI(TAG, "TEST_LED %s OFF executed", led_arg);
        }
    } else if (strcmp(state_arg, "BLINK") == 0) {
        ret = led_set_pattern(led, LED_PATTERN_BLINK_SLOW);
        if (ret == ESP_OK) {
            serial_send_ok("led_blinking");
            ESP_LOGI(TAG, "TEST_LED %s BLINK executed", led_arg);
        }
    } else {
        serial_send_error("invalid_argument", "State must be ON, OFF, or BLINK");
        return ESP_ERR_INVALID_ARG;
    }

    if (ret != ESP_OK) {
        serial_send_error("led_error", "Failed to set LED state");
        return ret;
    }

    return ESP_OK;
}

/**
 * TEST_BUTTON command handler
 * Reads and reports button state
 */
static esp_err_t cmd_test_button(const char *args)
{
    (void)args;  // Unused

    bool pressed = button_is_pressed();
    if (pressed) {
        serial_send_ok("button_state|pressed");
        ESP_LOGI(TAG, "TEST_BUTTON: pressed");
    } else {
        serial_send_ok("button_state|released");
        ESP_LOGI(TAG, "TEST_BUTTON: released");
    }

    return ESP_OK;
}

/**
 * BLE_SCAN command handler
 * Returns list of recently seen BLE devices (last 60 seconds)
 * Response format per AC5:
 *   OK|devices|N
 *   MAC|RSSI|last_seen_sec_ago
 *   ...
 */
static esp_err_t cmd_ble_scan(const char *args)
{
    (void)args;  // Unused

    ble_tracked_device_t devices[BLE_MAX_TRACKED_DEVICES];
    uint32_t count = 0;

    esp_err_t ret = ble_scanner_get_devices(devices, BLE_MAX_TRACKED_DEVICES, &count);
    if (ret != ESP_OK) {
        serial_send_error("ble_error", "Failed to get BLE devices");
        return ret;
    }

    // Send response header: OK|devices|N
    char header[32];
    int len = snprintf(header, sizeof(header), "OK|devices|%lu\r\n", (unsigned long)count);
    serial_write(header, len);

    // Get current time for calculating "seconds ago"
    uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);

    // Send each device: MAC|RSSI|last_seen_sec_ago
    for (uint32_t i = 0; i < count; i++) {
        uint32_t age_ms = now_ms - devices[i].last_seen_ms;
        uint32_t age_sec = age_ms / 1000;

        char line[64];
        len = snprintf(line, sizeof(line), "%s|%d|%lu\r\n",
                      devices[i].mac,
                      devices[i].rssi,
                      (unsigned long)age_sec);
        serial_write(line, len);
    }

    ESP_LOGI(TAG, "BLE_SCAN: returned %lu devices", (unsigned long)count);
    return ESP_OK;
}

/**
 * BLE_EVENTS command handler (Story 2.3, updated Story 2.4)
 * Returns last 10 decoded sensor events (button, motion, door)
 *
 * Response format:
 *   OK|events|N\n
 *   [timestamp_ms] MAC=[AA:BB:CC:DD:EE:FF] EVENT=[single_press] BATT=[85%] RSSI=[-65dBm]\n
 *   ...
 */
static esp_err_t cmd_ble_events(const char *args)
{
    (void)args;  // Unused

    sensor_event_record_t events[10];
    int count = sensor_event_get_history(events, 10);

    // Send response header: OK|events|N
    char header[32];
    int len = snprintf(header, sizeof(header), "OK|events|%d\r\n", count);
    serial_write(header, len);

    // Send each event
    for (int i = 0; i < count; i++) {
        const sensor_event_record_t *event = &events[i];

        // Convert event to string using unified function (Story 2.4)
        const char *event_name = sensor_event_to_string(event->sensor_type, event->event_value);

        // Format event line
        char line[128];
        if (event->battery_pct == BATTERY_UNKNOWN) {
            len = snprintf(line, sizeof(line),
                          "[%lu] MAC=[%s] EVENT=[%s] BATT=[N/A] RSSI=[%ddBm]\r\n",
                          (unsigned long)event->timestamp_ms,
                          event->mac,
                          event_name,
                          event->rssi);
        } else {
            len = snprintf(line, sizeof(line),
                          "[%lu] MAC=[%s] EVENT=[%s] BATT=[%d%%] RSSI=[%ddBm]\r\n",
                          (unsigned long)event->timestamp_ms,
                          event->mac,
                          event_name,
                          event->battery_pct,
                          event->rssi);
        }
        serial_write(line, len);
    }

    ESP_LOGI(TAG, "BLE_EVENTS: returned %d events", count);
    return ESP_OK;
}

/**
 * TEST_REGISTER command handler
 * Registers a sensor MAC for event filtering
 * Usage: TEST_REGISTER AA:BB:CC:DD:EE:FF
 */
static esp_err_t cmd_test_register(const char *args)
{
    if (args == NULL || args[0] == '\0') {
        serial_send_error("invalid_argument", "Usage: TEST_REGISTER <MAC> (e.g., AA:BB:CC:DD:EE:FF)");
        return ESP_ERR_INVALID_ARG;
    }

    // Copy and trim the MAC argument
    char mac[32] = {0};
    strncpy(mac, args, sizeof(mac) - 1);

    // Trim trailing whitespace
    size_t len = strlen(mac);
    while (len > 0 && isspace((unsigned char)mac[len - 1])) {
        mac[--len] = '\0';
    }

    // Convert to uppercase
    for (char *p = mac; *p; p++) {
        *p = toupper((unsigned char)*p);
    }

    // Attempt to register
    esp_err_t ret = nvs_set_registered_sensor_mac(mac);
    if (ret != ESP_OK) {
        if (ret == ESP_ERR_INVALID_ARG) {
            serial_send_error("invalid_mac", "MAC format must be AA:BB:CC:DD:EE:FF");
        } else {
            serial_send_error("nvs_error", "Failed to save MAC to NVS");
        }
        return ret;
    }

    // Invalidate MAC cache so BLE scanner picks up the new registration
    ble_scanner_invalidate_mac_cache();

    // Send success response
    char response[64];
    snprintf(response, sizeof(response), "registered|%s", mac);
    serial_send_ok(response);
    ESP_LOGI(TAG, "TEST_REGISTER: Sensor registered: %s", mac);

    return ESP_OK;
}

/**
 * TEST_UNREGISTER command handler
 * Clears the registered sensor MAC
 */
static esp_err_t cmd_test_unregister(const char *args)
{
    (void)args;  // Unused

    // Check if a sensor is registered
    if (!nvs_is_sensor_registered()) {
        serial_send_error("not_registered", "No sensor is currently registered");
        return ESP_ERR_NOT_FOUND;
    }

    // Clear registration
    esp_err_t ret = nvs_clear_registered_sensor();
    if (ret != ESP_OK) {
        serial_send_error("nvs_error", "Failed to clear sensor from NVS");
        return ret;
    }

    // Invalidate MAC cache
    ble_scanner_invalidate_mac_cache();

    serial_send_ok("unregistered");
    ESP_LOGI(TAG, "TEST_UNREGISTER: Sensor cleared");

    return ESP_OK;
}

/**
 * TEST_SAVE_CONFIG command handler (Story 3.1)
 * Saves a full sensor config with CRC validation
 * Usage: TEST_SAVE_CONFIG AA:BB:CC:DD:EE:FF [timer_seconds]
 * Default timer: 30 seconds
 */
static esp_err_t cmd_test_save_config(const char *args)
{
    if (args == NULL || args[0] == '\0') {
        serial_send_error("invalid_argument", "Usage: TEST_SAVE_CONFIG <MAC> [timer] (e.g., AA:BB:CC:DD:EE:FF 45)");
        return ESP_ERR_INVALID_ARG;
    }

    char mac[32] = {0};
    int timer = DEFAULT_TIMER_SECONDS;

    // Parse MAC and optional timer
    int parsed = sscanf(args, "%31s %d", mac, &timer);
    if (parsed < 1) {
        serial_send_error("invalid_argument", "Could not parse MAC address");
        return ESP_ERR_INVALID_ARG;
    }

    // Convert MAC to uppercase
    for (char *p = mac; *p; p++) {
        *p = toupper((unsigned char)*p);
    }

    // Validate timer range
    if (timer < TIMER_SECONDS_MIN || timer > TIMER_SECONDS_MAX) {
        char err_msg[64];
        snprintf(err_msg, sizeof(err_msg), "Timer must be %d-%d seconds", TIMER_SECONDS_MIN, TIMER_SECONDS_MAX);
        serial_send_error("invalid_argument", err_msg);
        return ESP_ERR_INVALID_ARG;
    }

    // Build config struct
    sensor_config_t config;
    nvs_init_config_defaults(&config);
    strncpy(config.sensor_mac, mac, sizeof(config.sensor_mac) - 1);
    config.sensor_type = NVS_SENSOR_TYPE_BUTTON;  // Default to button
    config.timer_seconds = (uint16_t)timer;
    config.retrigger_mode = RETRIGGER_EXTEND;
    config.config_version = CONFIG_VERSION;

    // Save config (CRC calculated internally)
    esp_err_t ret = nvs_save_config(&config);
    if (ret != ESP_OK) {
        if (ret == ESP_ERR_INVALID_ARG) {
            serial_send_error("invalid_config", "Config validation failed (check MAC format)");
        } else {
            serial_send_error("nvs_error", "Failed to save config to NVS");
        }
        return ret;
    }

    // Invalidate MAC cache so BLE scanner picks up the new config
    ble_scanner_invalidate_mac_cache();

    // Calculate CRC for response
    uint32_t crc = nvs_calculate_config_crc(&config);

    // Send success response with CRC
    char response[128];
    snprintf(response, sizeof(response), "config_saved|MAC=%s|timer=%d|CRC=0x%08lX",
             config.sensor_mac, config.timer_seconds, (unsigned long)crc);
    serial_send_ok(response);
    ESP_LOGI(TAG, "TEST_SAVE_CONFIG: Config saved with CRC 0x%08lX", (unsigned long)crc);

    return ESP_OK;
}

/**
 * TEST_LOAD_CONFIG command handler (Story 3.1)
 * Loads and displays the current config with CRC validation
 */
static esp_err_t cmd_test_load_config(const char *args)
{
    (void)args;  // Unused

    sensor_config_t config;
    esp_err_t ret = nvs_load_config(&config);

    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        serial_send_error("not_found", "No config stored in NVS");
        return ret;
    }

    if (ret == ESP_ERR_INVALID_CRC) {
        serial_send_error("crc_error", "Config CRC mismatch - corruption detected! Relay forced OFF.");
        return ret;
    }

    if (ret != ESP_OK) {
        serial_send_error("nvs_error", "Failed to load config from NVS");
        return ret;
    }

    // Format response with all config fields
    char response[256];
    snprintf(response, sizeof(response),
             "config_loaded|MAC=%s|type=%d|timer=%d|retrigger=%d|version=%d|CRC=0x%08lX",
             config.sensor_mac,
             config.sensor_type,
             config.timer_seconds,
             config.retrigger_mode,
             config.config_version,
             (unsigned long)config.config_crc);
    serial_send_ok(response);

    ESP_LOGI(TAG, "TEST_LOAD_CONFIG: Config loaded successfully (CRC valid)");
    return ESP_OK;
}

// ============================================================================
// REGISTER_SENSOR Helper Functions (Story 3.3)
// ============================================================================

/**
 * Validate MAC address format per FR16
 *
 * Requirements:
 * - Exactly 17 characters: AA:BB:CC:DD:EE:FF
 * - Colons at positions 2, 5, 8, 11, 14
 * - Uppercase hex digits only (0-9, A-F) at other positions
 *
 * @param mac MAC address string to validate
 * @return true if format is valid, false otherwise
 */
bool validate_mac_format(const char *mac)
{
    if (mac == NULL || strlen(mac) != 17) {
        return false;
    }

    // Check colons at correct positions
    if (mac[2] != ':' || mac[5] != ':' || mac[8] != ':' ||
        mac[11] != ':' || mac[14] != ':') {
        return false;
    }

    // Check uppercase hex digits at non-colon positions
    for (int i = 0; i < 17; i++) {
        if (i == 2 || i == 5 || i == 8 || i == 11 || i == 14) {
            continue;  // Skip colon positions
        }
        char c = mac[i];
        if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'))) {
            return false;  // Must be uppercase hex
        }
    }

    return true;
}

/**
 * Parse sensor type string (case-insensitive)
 *
 * @param type_str Sensor type string ("BUTTON", "MOTION", or "DOOR")
 * @return sensor_type_t value, or SENSOR_TYPE_NONE (0) if invalid
 */
sensor_type_t parse_sensor_type(const char *type_str)
{
    if (type_str == NULL || type_str[0] == '\0') {
        return SENSOR_TYPE_NONE;
    }

    if (strcasecmp(type_str, "BUTTON") == 0) {
        return SENSOR_TYPE_BUTTON;  // 1
    } else if (strcasecmp(type_str, "MOTION") == 0) {
        return SENSOR_TYPE_MOTION;  // 2
    } else if (strcasecmp(type_str, "DOOR") == 0) {
        return SENSOR_TYPE_DOOR;    // 3
    }

    return SENSOR_TYPE_NONE;  // 0 = invalid
}

/**
 * Convert sensor type to uppercase string for response
 *
 * @param type Sensor type enum value
 * @return String representation ("BUTTON", "MOTION", "DOOR", or "UNKNOWN")
 */
static const char* sensor_type_to_string(sensor_type_t type)
{
    switch (type) {
        case SENSOR_TYPE_BUTTON: return "BUTTON";
        case SENSOR_TYPE_MOTION: return "MOTION";
        case SENSOR_TYPE_DOOR:   return "DOOR";
        default: return "UNKNOWN";
    }
}

/**
 * REGISTER_SENSOR command handler (Story 3.3)
 *
 * Manually registers a sensor by MAC address and type.
 * Usage: REGISTER_SENSOR AA:BB:CC:DD:EE:FF BUTTON
 *
 * Validates MAC format (FR16) and sensor type, saves to NVS,
 * transitions state to LISTENING, and sets LED to slow blink.
 */
static esp_err_t cmd_register_sensor(const char *args)
{
    // Check for missing arguments
    if (args == NULL || args[0] == '\0') {
        serial_send_error("INVALID_ARGUMENT",
                          "Usage: REGISTER_SENSOR AA:BB:CC:DD:EE:FF BUTTON/MOTION/DOOR");
        return ESP_ERR_INVALID_ARG;
    }

    // Skip leading spaces
    while (*args == ' ') args++;

    if (*args == '\0') {
        serial_send_error("INVALID_ARGUMENT",
                          "Usage: REGISTER_SENSOR AA:BB:CC:DD:EE:FF BUTTON/MOTION/DOOR");
        return ESP_ERR_INVALID_ARG;
    }

    // Extract MAC address (first 17 characters)
    char mac[18] = {0};
    strncpy(mac, args, 17);
    mac[17] = '\0';

    // Move to sensor type argument
    const char *type_arg = args + 17;
    while (*type_arg == ' ') type_arg++;  // Skip spaces

    if (*type_arg == '\0') {
        serial_send_error("INVALID_ARGUMENT",
                          "Missing sensor type. Use BUTTON, MOTION, or DOOR");
        return ESP_ERR_INVALID_ARG;
    }

    // Extract sensor type (until space, newline, or end)
    char type_str[16] = {0};
    int i = 0;
    while (type_arg[i] != '\0' && type_arg[i] != ' ' &&
           type_arg[i] != '\n' && type_arg[i] != '\r' && i < 15) {
        type_str[i] = type_arg[i];
        i++;
    }
    type_str[i] = '\0';

    // Validate MAC format (AC2, AC3)
    if (!validate_mac_format(mac)) {
        ESP_LOGW(TAG, "REGISTER_SENSOR: Invalid MAC format: %s", mac);
        serial_send_error("INVALID_MAC",
                          "MAC must be uppercase with colons (AA:BB:CC:DD:EE:FF)");
        return ESP_ERR_INVALID_ARG;
    }

    // Parse and validate sensor type (AC4, AC5)
    sensor_type_t sensor_type = parse_sensor_type(type_str);
    if (sensor_type == SENSOR_TYPE_NONE) {
        ESP_LOGW(TAG, "REGISTER_SENSOR: Invalid sensor type: %s", type_str);
        serial_send_error("INVALID_TYPE",
                          "Sensor type must be BUTTON, MOTION, or DOOR");
        return ESP_ERR_INVALID_ARG;
    }

    // Check if sensor already registered (AC10 - overwrite case)
    sensor_config_t existing_config;
    esp_err_t existing_ret = nvs_load_config(&existing_config);
    if (existing_ret == ESP_OK) {
        ESP_LOGI(TAG, "Overwriting existing sensor: %s", existing_config.sensor_mac);
    }

    // Build config struct with defaults (AC6)
    sensor_config_t config;
    nvs_init_config_defaults(&config);
    strncpy(config.sensor_mac, mac, sizeof(config.sensor_mac) - 1);
    config.sensor_mac[sizeof(config.sensor_mac) - 1] = '\0';
    config.sensor_type = sensor_type;
    config.timer_seconds = LEARNING_DEFAULT_TIMER_SECONDS;  // 10 seconds per Story 3.2
    config.retrigger_mode = RETRIGGER_EXTEND;
    config.config_version = CONFIG_VERSION;

    // Save config to NVS (CRC calculated internally)
    esp_err_t ret = nvs_save_config(&config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "REGISTER_SENSOR: NVS save failed: %s", esp_err_to_name(ret));
        serial_send_error("NVS_FAILURE", "Failed to save configuration");
        return ret;
    }

    // Exit learning mode if active (idempotent)
    exit_learning_mode();

    // Transition state to LISTENING (AC9)
    state_set(STATE_LISTENING);

    // Set LED to slow blink pattern (AC8)
    led_set_pattern(LED_STATUS, LED_PATTERN_BLINK_SLOW);

    // Invalidate MAC cache so BLE scanner picks up the new registration
    ble_scanner_invalidate_mac_cache();

    // Send success response (AC7)
    char response[64];
    snprintf(response, sizeof(response), "registered|%s|%s",
             mac, sensor_type_to_string(sensor_type));
    serial_send_ok(response);

    ESP_LOGI(TAG, "REGISTER_SENSOR: Sensor registered: %s (%s)",
             mac, sensor_type_to_string(sensor_type));

    return ESP_OK;
}

/**
 * CLEAR_SENSOR command handler (Story 3.4)
 *
 * Clears all sensor configuration from NVS.
 * Usage: CLEAR_SENSOR
 *
 * Response: OK|cleared
 * Error: ERROR|NVS_FAILURE|Failed to clear configuration
 *
 * Side effects:
 * - Erases all NVS config keys (sensor_mac, sensor_type, timer, etc.)
 * - Exits learning mode if active
 * - Sets state to STATE_UNCONFIGURED
 * - Sets status LED to off pattern
 */
static esp_err_t cmd_clear_sensor(const char *args)
{
    (void)args;  // No arguments for this command

    // Clear all configuration from NVS
    esp_err_t ret = nvs_clear_config();

    // nvs_clear_config returns ESP_ERR_NVS_NOT_FOUND if nothing to clear
    // Treat this as success (idempotent behavior)
    if (ret != ESP_OK && ret != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Failed to clear config: %s", esp_err_to_name(ret));
        serial_send_error("NVS_FAILURE", "Failed to clear configuration");
        return ret;
    }

    // Exit learning mode if active (idempotent)
    exit_learning_mode();

    // Update state to unconfigured
    state_set(STATE_UNCONFIGURED);

    // Set LED to off pattern (unconfigured state)
    led_set_pattern(LED_STATUS, LED_PATTERN_OFF);

    // Invalidate BLE MAC cache
    ble_scanner_invalidate_mac_cache();

    ESP_LOGI(TAG, "Sensor configuration cleared");
    serial_send_ok("cleared");

    return ESP_OK;
}

/**
 * SET_TIMER command handler (Story 4A.3)
 *
 * Sets the relay timer duration via serial command.
 * Usage: SET_TIMER <1-600>
 *
 * Validates the timer value (1-600 seconds per FR19), updates NVS config,
 * and responds with success or appropriate error.
 *
 * Response formats:
 *   OK|timer_set|60
 *   ERROR|INVALID_FORMAT|Timer must be an integer
 *   ERROR|INVALID_RANGE|Timer must be 1-600 seconds
 *   ERROR|NVS_FAILURE|Failed to load/save config
 */
static esp_err_t cmd_set_timer(const char *args)
{
    // Check for missing argument (AC3: non-integer includes empty)
    if (args == NULL || args[0] == '\0') {
        serial_send_error("INVALID_FORMAT", "Timer must be an integer");
        return ESP_ERR_INVALID_ARG;
    }

    // Skip leading whitespace
    while (*args == ' ') args++;

    if (*args == '\0') {
        serial_send_error("INVALID_FORMAT", "Timer must be an integer");
        return ESP_ERR_INVALID_ARG;
    }

    // Parse integer value using strtol for proper validation
    char *endptr;
    long value = strtol(args, &endptr, 10);

    // Skip trailing whitespace before checking if string fully consumed
    while (*endptr == ' ' || *endptr == '\r' || *endptr == '\n') {
        endptr++;
    }

    // Check if entire string was consumed (valid integer) - AC3
    if (*endptr != '\0') {
        ESP_LOGW(TAG, "SET_TIMER: Invalid format: '%s'", args);
        serial_send_error("INVALID_FORMAT", "Timer must be an integer");
        return ESP_ERR_INVALID_ARG;
    }

    // Range check 1-600 seconds per FR19 - AC2
    if (value < TIMER_SECONDS_MIN || value > TIMER_SECONDS_MAX) {
        ESP_LOGW(TAG, "SET_TIMER: Out of range: %ld", value);
        serial_send_error("INVALID_RANGE", "Timer must be 1-600 seconds");
        return ESP_ERR_INVALID_ARG;
    }

    // Load existing config from NVS - AC4
    sensor_config_t config;
    esp_err_t err = nvs_load_config(&config);

    if (err == ESP_ERR_NVS_NOT_FOUND) {
        // No config exists yet - initialize with defaults
        nvs_init_config_defaults(&config);
    } else if (err != ESP_OK) {
        // NVS error (could be CRC failure or other)
        ESP_LOGE(TAG, "SET_TIMER: Failed to load config: %s", esp_err_to_name(err));
        serial_send_error("NVS_FAILURE", "Failed to load config");
        return err;
    }

    // Update timer field - AC4
    config.timer_seconds = (uint16_t)value;

    // Save config (CRC recalculated internally by nvs_save_config) - AC4
    err = nvs_save_config(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SET_TIMER: Failed to save config: %s", esp_err_to_name(err));
        serial_send_error("NVS_FAILURE", "Failed to save config");
        return err;
    }

    // Log success - AC5
    ESP_LOGI(TAG, "Timer set to %d seconds", (int)value);

    // Send success response - AC5
    char response[32];
    snprintf(response, sizeof(response), "timer_set|%d", (int)value);
    serial_send_ok(response);

    return ESP_OK;
}

/**
 * HELP command handler
 * Lists all available commands with descriptions
 */
static esp_err_t cmd_help(const char *args)
{
    (void)args;  // Unused

    serial_send_ok("help");
    serial_send_raw("Available commands:\r\n");

    for (int i = 0; commands[i].command != NULL; i++) {
        char line[128];
        int len = snprintf(line, sizeof(line), "  %s\r\n", commands[i].help);
        serial_write(line, len);
    }

    ESP_LOGI(TAG, "HELP command executed");
    return ESP_OK;
}

// ============================================================================
// Command Processing
// ============================================================================

/**
 * Process a received command line
 * Parses command and arguments, finds handler, executes it
 */
static void process_command(const char *line)
{
    char cmd[32] = {0};
    char args[SERIAL_CMD_MAX_LEN] = {0};

    ESP_LOGI(TAG, "Received command: %s", line);

    // Skip empty lines
    if (line[0] == '\0') {
        return;
    }

    // Split at first space
    const char *space = strchr(line, ' ');
    if (space) {
        size_t cmd_len = space - line;
        if (cmd_len >= sizeof(cmd)) {
            cmd_len = sizeof(cmd) - 1;
        }
        strncpy(cmd, line, cmd_len);
        cmd[cmd_len] = '\0';

        // Copy arguments (skip leading space)
        strncpy(args, space + 1, sizeof(args) - 1);
    } else {
        strncpy(cmd, line, sizeof(cmd) - 1);
        args[0] = '\0';
    }

    // Convert command to uppercase for case-insensitive matching
    for (char *p = cmd; *p; p++) {
        *p = toupper((unsigned char)*p);
    }

    // Find and execute handler
    for (int i = 0; commands[i].command != NULL; i++) {
        if (strcmp(cmd, commands[i].command) == 0) {
            commands[i].handler(args);
            return;
        }
    }

    // Unknown command
    serial_send_error("invalid_command", "Unknown command. Type HELP for available commands.");
    ESP_LOGW(TAG, "Unknown command: %s", cmd);
}

// ============================================================================
// Serial Protocol Task
// ============================================================================

/**
 * Serial protocol task
 * Reads bytes from USB Serial JTAG, assembles into lines, processes commands
 */
static void serial_protocol_task(void *pvParameters)
{
    (void)pvParameters;

    char line_buffer[SERIAL_CMD_MAX_LEN + 1];
    int line_pos = 0;
    uint8_t byte;

    ESP_LOGI(TAG, "Serial protocol task started - waiting for commands");

    while (1) {
        // Read one byte at a time from USB Serial JTAG with timeout
        int len = usb_serial_jtag_read_bytes(&byte, 1, pdMS_TO_TICKS(100));

        if (len > 0) {
            // Echo the character back for user feedback
            usb_serial_jtag_write_bytes(&byte, 1, pdMS_TO_TICKS(10));

            // Handle newline - end of command
            if (byte == '\n' || byte == '\r') {
                // Send newline for clean output
                serial_write("\r\n", 2);

                if (line_pos > 0) {
                    line_buffer[line_pos] = '\0';
                    process_command(line_buffer);
                    line_pos = 0;
                }
            }
            // Handle backspace
            else if (byte == 0x08 || byte == 0x7F) {
                if (line_pos > 0) {
                    line_pos--;
                    // Erase character on terminal
                    serial_write("\b \b", 3);
                }
            }
            // Accumulate printable characters
            else if (line_pos < SERIAL_CMD_MAX_LEN && byte >= 0x20 && byte < 0x7F) {
                line_buffer[line_pos++] = (char)byte;
            }
            // Ignore overflow and non-printable
        }
    }
}

// ============================================================================
// Public API
// ============================================================================

esp_err_t serial_protocol_init(void)
{
    if (serial_initialized) {
        ESP_LOGW(TAG, "Serial protocol already initialized");
        return ESP_OK;
    }

    // Configure USB Serial JTAG driver
    usb_serial_jtag_driver_config_t usb_serial_config = {
        .rx_buffer_size = SERIAL_RX_BUF_SIZE,
        .tx_buffer_size = SERIAL_RX_BUF_SIZE,
    };

    esp_err_t ret = usb_serial_jtag_driver_install(&usb_serial_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install USB Serial JTAG driver: %s", esp_err_to_name(ret));
        relay_force_off();  // Safety first
        return ret;
    }

    // Create serial protocol task
    BaseType_t task_ret = xTaskCreate(
        serial_protocol_task,
        "serial_proto",
        4096,   // Stack size (larger for string operations)
        NULL,
        2,      // Priority
        &serial_task_handle
    );

    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create serial protocol task");
        relay_force_off();  // Safety first
        usb_serial_jtag_driver_uninstall();
        return ESP_FAIL;
    }

    serial_initialized = true;
    ESP_LOGI(TAG, "Serial protocol initialized (USB Serial JTAG)");

    return ESP_OK;
}
