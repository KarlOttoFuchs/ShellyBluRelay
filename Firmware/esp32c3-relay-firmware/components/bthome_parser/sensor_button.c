/*
 * Button Event Handler - BTHome Object ID 0x3A
 *
 * Story 2.3: Decode Shelly BLU Button Events
 *
 * Implements button press event decoding from Shelly BLU Button sensor.
 */

#include "sensor_button.h"
#include "bthome_parser.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>

static const char *TAG = "sensor_button";

// Unified circular buffer for last 10 events (volatile, lost on reboot)
// Supports button, motion, and door sensors (Story 2.4)
#define EVENT_BUFFER_SIZE 10
static sensor_event_record_t event_buffer[EVENT_BUFFER_SIZE];
static uint8_t event_write_index = 0;
static uint8_t event_count = 0;  // 0-10, how many valid events

/**
 * Convert sensor event to human-readable string (Story 2.4 - Unified)
 *
 * @param sensor_type  Sensor type (BUTTON, MOTION, DOOR)
 * @param event_value  Event value
 * @return String representation of event
 */
const char* sensor_event_to_string(sensor_type_t sensor_type, uint8_t event_value) {
    switch (sensor_type) {
        case SENSOR_TYPE_BUTTON:
            switch (event_value) {
                case BUTTON_EVENT_SINGLE:
                    return "single_press";
                case BUTTON_EVENT_DOUBLE:
                    return "double_press";
                case BUTTON_EVENT_TRIPLE:
                    return "triple_press";
                case BUTTON_EVENT_LONG:
                    return "long_press";
                default:
                    return "unknown_button";
            }
        case SENSOR_TYPE_MOTION:
            return (event_value == 0x01) ? "motion_detected" : "motion_timeout";
        case SENSOR_TYPE_DOOR:
            return (event_value == 0x01) ? "door_open" : "door_closed";
        default:
            return "unknown_type";
    }
}

/**
 * Convert button event value to human-readable string (legacy wrapper)
 *
 * @param event_value  Button event value (0x01-0x04, or unknown)
 * @return String representation of event
 */
static const char *button_value_to_string(uint8_t event_value) {
    return sensor_event_to_string(SENSOR_TYPE_BUTTON, event_value);
}

/**
 * Log sensor event to unified circular buffer (Story 2.4)
 *
 * Unified function for logging events from any sensor type.
 *
 * @param mac          MAC address string
 * @param sensor_type  Sensor type (BUTTON, MOTION, or DOOR)
 * @param event_value  Raw event value
 * @param battery_pct  Battery percentage (0-100, 255=unknown)
 * @param rssi         Signal strength in dBm
 */
void sensor_event_log(const char *mac, sensor_type_t sensor_type,
                      uint8_t event_value, uint8_t battery_pct, int8_t rssi) {
    sensor_event_record_t *record = &event_buffer[event_write_index];
    record->timestamp_ms = esp_timer_get_time() / 1000;  // Convert µs to ms
    strncpy(record->mac, mac, sizeof(record->mac) - 1);
    record->mac[sizeof(record->mac) - 1] = '\0';
    record->sensor_type = sensor_type;
    record->event_value = event_value;
    record->battery_pct = battery_pct;
    record->rssi = rssi;

    event_write_index = (event_write_index + 1) % EVENT_BUFFER_SIZE;
    if (event_count < EVENT_BUFFER_SIZE) {
        event_count++;
    }
}

/**
 * Log button event to circular buffer (legacy wrapper for Story 2.3 compatibility)
 *
 * @param mac          MAC address string
 * @param event_type   Button event type value
 * @param battery_pct  Battery percentage (0-100, 255=unknown)
 * @param rssi         Signal strength in dBm
 */
static void button_event_log(const char *mac, uint8_t event_type,
                              uint8_t battery_pct, int8_t rssi) {
    sensor_event_log(mac, SENSOR_TYPE_BUTTON, event_type, battery_pct, rssi);
}

// Button event handler (matches bthome_handler_t signature)
void button_event_handler(const char *mac, uint8_t object_id,
                          const uint8_t *value, size_t value_len,
                          int8_t rssi) {
    // Validate value length (button events are 1 byte per BTHome v2 spec)
    if (value_len != 1) {
        ESP_LOGW(TAG, "Button event invalid length: %d (expected 1)", value_len);
        return;  // Skip this event, continue scanning
    }

    uint8_t event_value = value[0];
    const char *event_name = button_value_to_string(event_value);

    // Log unknown button events at WARN level
    if (event_value > BUTTON_EVENT_LONG && event_value != BUTTON_EVENT_NONE) {
        ESP_LOGW(TAG, "Unknown button event: 0x%02X (MAC: %s)", event_value, mac);
    }

    // Get battery level from BTHome parser (if available in same packet)
    uint8_t battery_pct = bthome_get_battery_level();

    // Log button event with context
    if (battery_pct == BATTERY_UNKNOWN) {
        ESP_LOGI(TAG, "Button: %s (MAC: %s, RSSI: %d dBm)", event_name, mac, rssi);
    } else {
        ESP_LOGI(TAG, "Button: %s (MAC: %s, RSSI: %d dBm, Battery: %d%%)",
                 event_name, mac, rssi, battery_pct);
    }

    // Log to event buffer for history tracking
    button_event_log(mac, event_value, battery_pct, rssi);

    // Invoke learning mode callback if registered (Story 3.2)
    bthome_learning_callback_t callback = bthome_get_learning_callback();
    if (callback != NULL) {
        uint8_t mac_bytes[6];
        if (bthome_mac_str_to_bytes(mac, mac_bytes)) {
            callback(mac_bytes, SENSOR_TYPE_BUTTON, battery_pct, rssi);
        }
    }
}

// Get number of events in buffer (unified for all sensor types)
int sensor_event_get_count(void) {
    return event_count;
}

// Get last event from buffer (unified for all sensor types)
bool sensor_event_get_last(sensor_event_record_t *record) {
    if (event_count == 0 || record == NULL) {
        return false;
    }

    // Last event is at (write_index - 1 + SIZE) % SIZE
    int last_index = (event_write_index - 1 + EVENT_BUFFER_SIZE) % EVENT_BUFFER_SIZE;
    *record = event_buffer[last_index];
    return true;
}

// Get event history from buffer (newest first, unified for all sensor types)
int sensor_event_get_history(sensor_event_record_t *records, int max_records) {
    if (records == NULL || max_records <= 0) {
        return 0;
    }

    int count = (event_count < max_records) ? event_count : max_records;

    // Copy events in reverse chronological order (newest first)
    for (int i = 0; i < count; i++) {
        int buffer_index = (event_write_index - 1 - i + EVENT_BUFFER_SIZE) % EVENT_BUFFER_SIZE;
        records[i] = event_buffer[buffer_index];
    }

    return count;
}

// Clear event buffer (for testing and initialization, unified for all sensor types)
void sensor_event_clear(void) {
    event_count = 0;
    event_write_index = 0;
    memset(event_buffer, 0, sizeof(event_buffer));
}
