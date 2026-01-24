/*
 * Door Event Handler - BTHome Object ID 0x2D
 *
 * Story 2.4: Decode Motion & Door Events + Implement MAC Filtering
 *
 * Implements door/window opening event decoding from Shelly BLU Door/Window sensor.
 */

#include "sensor_door.h"
#include "sensor_button.h"  // For unified sensor_event_log()
#include "bthome_parser.h"
#include "esp_log.h"

static const char *TAG = "sensor_door";

/**
 * Door event handler (matches bthome_handler_t signature)
 */
void door_event_handler(const char *mac, uint8_t object_id,
                        const uint8_t *value, size_t value_len,
                        int8_t rssi) {
    // Validate value length (door events are 1 byte per BTHome v2 spec)
    if (value_len != 1) {
        ESP_LOGW(TAG, "Door event invalid length: %d (expected 1)", value_len);
        return;  // Skip this event, continue scanning
    }

    uint8_t event_value = value[0];
    const char *event_name = (event_value == DOOR_OPEN) ? "door_open" : "door_closed";

    // Get battery level from BTHome parser (if available in same packet)
    uint8_t battery_pct = bthome_get_battery_level();

    // Log door event with context
    if (battery_pct == BATTERY_UNKNOWN) {
        ESP_LOGI(TAG, "Door: %s (MAC: %s, RSSI: %d dBm)", event_name, mac, rssi);
    } else {
        ESP_LOGI(TAG, "Door: %s (MAC: %s, RSSI: %d dBm, Battery: %d%%)",
                 event_name, mac, rssi, battery_pct);
    }

    // Log to unified event buffer for history tracking (Story 2.4)
    sensor_event_log(mac, SENSOR_TYPE_DOOR, event_value, battery_pct, rssi);

    // Invoke learning mode callback if registered (Story 3.2)
    bthome_learning_callback_t callback = bthome_get_learning_callback();
    if (callback != NULL) {
        uint8_t mac_bytes[6];
        if (bthome_mac_str_to_bytes(mac, mac_bytes)) {
            callback(mac_bytes, SENSOR_TYPE_DOOR, battery_pct, rssi);
        }
    }
}
