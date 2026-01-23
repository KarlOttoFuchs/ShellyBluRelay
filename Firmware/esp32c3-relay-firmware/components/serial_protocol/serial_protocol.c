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
#include "led_control.h"
#include "button_input.h"
#include "ble_scanner.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/usb_serial_jtag.h"
#include "esp_vfs_usb_serial_jtag.h"
#include "esp_vfs_dev.h"

#include <string.h>
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
static esp_err_t cmd_test_relay(const char *args);
static esp_err_t cmd_test_led(const char *args);
static esp_err_t cmd_test_button(const char *args);
static esp_err_t cmd_ble_scan(const char *args);
static esp_err_t cmd_help(const char *args);

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
    {"PING",        cmd_ping,        "PING - Test connectivity (responds: pong)"},
    {"TEST_RELAY",  cmd_test_relay,  "TEST_RELAY [ON|OFF] - Control relay"},
    {"TEST_LED",    cmd_test_led,    "TEST_LED [STATUS|ERROR] [ON|OFF|BLINK] - Control LEDs"},
    {"TEST_BUTTON", cmd_test_button, "TEST_BUTTON - Read button state"},
    {"BLE_SCAN",    cmd_ble_scan,    "BLE_SCAN - Show recently seen BLE devices"},
    {"HELP",        cmd_help,        "HELP - Show available commands"},
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
    char response[128];
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
