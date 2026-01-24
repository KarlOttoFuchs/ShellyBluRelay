/*
 * BTHome v2 Parser Component - Implementation
 *
 * Story 2.2: Implement BTHome v2 Parser with Handler Registry
 *
 * Parses BTHome v2 BLE advertising packets and dispatches to registered handlers.
 */

#include "bthome_parser.h"
#include "sensor_button.h"
#include "sensor_motion.h"
#include "sensor_door.h"
#include "led_control.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "bthome_parser";

// Handler registry (max 16 handlers for MVP)
#define MAX_HANDLERS 16

typedef struct {
    uint8_t object_id;
    bthome_handler_t handler;
    bool registered;
} bthome_handler_entry_t;

static bthome_handler_entry_t handler_registry[MAX_HANDLERS];

// Battery level tracking (RAM storage, not persisted)
static uint8_t current_battery_pct = BATTERY_UNKNOWN;
static uint32_t last_battery_update_ms = 0;

// Track if error LED was set by battery handler (to avoid clearing other errors)
static bool battery_error_led_active = false;

// Learning mode callback (Story 3.2)
static bthome_learning_callback_t learning_callback = NULL;

// Forward declarations
static uint8_t bthome_get_value_length(uint8_t object_id);
static bthome_handler_t bthome_find_handler(uint8_t object_id);
static void battery_handler(const char *mac, uint8_t object_id,
                             const uint8_t *value, size_t value_len,
                             int8_t rssi);

/**
 * Initialize BTHome parser and register internal handlers
 */
esp_err_t bthome_init(void)
{
    // Clear handler registry
    memset(handler_registry, 0, sizeof(handler_registry));

    // Register internal battery handler for Object ID 0x01
    esp_err_t ret = bthome_register_handler(0x01, battery_handler);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register battery handler");
        return ret;
    }

    // Register button handler for Object ID 0x3A (Story 2.3)
    ret = bthome_register_handler(0x3A, button_event_handler);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register button handler");
        return ret;
    }
    ESP_LOGD(TAG, "Button handler registered for Object ID 0x3A");

    // Register motion handler for Object ID 0x21 (Story 2.4)
    ret = bthome_register_handler(0x21, motion_event_handler);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register motion handler");
        return ret;
    }
    ESP_LOGD(TAG, "Motion handler registered for Object ID 0x21");

    // Register door handler for Object ID 0x2D (Story 2.4)
    ret = bthome_register_handler(0x2D, door_event_handler);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register door handler");
        return ret;
    }
    ESP_LOGD(TAG, "Door handler registered for Object ID 0x2D");

    ESP_LOGI(TAG, "BTHome parser initialized");
    return ESP_OK;
}

/**
 * Register handler for BTHome Object ID
 */
esp_err_t bthome_register_handler(uint8_t object_id, bthome_handler_t handler)
{
    if (handler == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // Find empty slot
    for (int i = 0; i < MAX_HANDLERS; i++) {
        if (!handler_registry[i].registered) {
            handler_registry[i].object_id = object_id;
            handler_registry[i].handler = handler;
            handler_registry[i].registered = true;
            ESP_LOGD(TAG, "Registered handler for Object ID 0x%02X", object_id);
            return ESP_OK;
        }
    }

    ESP_LOGE(TAG, "Handler registry full (max %d)", MAX_HANDLERS);
    return ESP_ERR_NO_MEM;
}

/**
 * Find registered handler by Object ID
 *
 * @return Handler function pointer or NULL if not registered
 */
static bthome_handler_t bthome_find_handler(uint8_t object_id)
{
    for (int i = 0; i < MAX_HANDLERS; i++) {
        if (handler_registry[i].registered &&
            handler_registry[i].object_id == object_id) {
            return handler_registry[i].handler;
        }
    }
    return NULL;
}

/**
 * Get BTHome Object ID value length (per BTHome v2 spec)
 *
 * Returns the number of value bytes for each Object ID type.
 * See: https://bthome.io/format/
 *
 * @param object_id  BTHome Object ID
 * @return Value length in bytes, or 0 if unknown
 */
static uint8_t bthome_get_value_length(uint8_t object_id)
{
    switch (object_id) {
        // 1-byte values (uint8 or boolean)
        case 0x00:  // Packet ID (counter)
        case 0x01:  // Battery %
        case 0x10:  // Power (on/off)
        case 0x11:  // Opening (bool) - alternate
        case 0x12:  // CO2 (bool)
        case 0x13:  // Cold (bool)
        case 0x14:  // Connectivity (bool)
        case 0x15:  // Door (bool)
        case 0x16:  // Garage door (bool)
        case 0x17:  // Gas (bool)
        case 0x18:  // Generic boolean
        case 0x19:  // Heat (bool)
        case 0x1A:  // Light (bool)
        case 0x1B:  // Lock (bool)
        case 0x1C:  // Moisture (bool)
        case 0x1D:  // Motion (bool) - alternate
        case 0x1E:  // Moving (bool)
        case 0x1F:  // Occupancy (bool)
        case 0x20:  // Plug (bool)
        case 0x21:  // Motion (bool) - presence
        case 0x22:  // Presence (bool)
        case 0x23:  // Problem (bool)
        case 0x24:  // Running (bool)
        case 0x25:  // Safety (bool)
        case 0x26:  // Smoke (bool)
        case 0x27:  // Sound (bool)
        case 0x28:  // Tamper (bool)
        case 0x29:  // Vibration (bool)
        case 0x2A:  // Window (bool)
        case 0x2D:  // Opening (bool) - door/window sensor
        case 0x2E:  // Battery (bool) - low battery indicator
        case 0x2F:  // Battery charging (bool)
        case 0x3A:  // Button event
        case 0x3C:  // Dimmer event
        case 0x3E:  // Timestamp (uint48) - actually 4 bytes in some implementations
            return 1;

        // Rotation/angle sensors (sint16 = 2 bytes)
        case 0x3F:  // Rotation (sint16, 0.1°) - Shelly Door/Window tilt angle
            return 2;

        // 2-byte values (uint16 or sint16)
        case 0x02:  // Temperature (sint16, 0.01°C)
        case 0x03:  // Humidity (uint16, 0.01%)
        case 0x04:  // Pressure (uint16, 0.01 hPa) - NOTE: spec says 3 bytes but some use 2
        case 0x06:  // Mass kg (uint16, 0.01 kg)
        case 0x07:  // Mass lb (uint16, 0.01 lb)
        case 0x08:  // Dewpoint (sint16, 0.01°C)
        case 0x09:  // Count (uint16)
        case 0x0A:  // Energy (uint16, 0.001 kWh) - NOTE: spec says 3 bytes
        case 0x0B:  // Power (uint16, 0.01 W) - NOTE: spec says 3 bytes
        case 0x0C:  // Voltage (uint16, 0.001 V)
        case 0x0D:  // PM2.5 (uint16)
        case 0x0E:  // PM10 (uint16)
        case 0x40:  // Distance mm (uint16)
        case 0x41:  // Distance m (uint16, 0.1 m)
        case 0x43:  // Current (uint16, 0.001 A)
        case 0x4A:  // Voltage (uint16, 0.1 V)
        case 0x4D:  // Count (uint16)
            return 2;

        // 3-byte values (uint24)
        case 0x05:  // Illuminance (uint24, 0.01 lux)
        case 0x0F:  // CO2 (uint24)
            return 3;

        // 4-byte values (uint32 or sint32)
        case 0x42:  // Duration (uint24) - 3 bytes per spec
        case 0x4B:  // Gas (uint24, 0.001 m³)
        case 0x4C:  // Gas (uint32, 0.001 m³)
            return 4;

        // Unknown Object IDs - log and return 0 to stop parsing
        default:
            ESP_LOGW(TAG, "Unknown Object ID 0x%02X - cannot determine length, stopping parse", object_id);
            return 0;
    }
}

/**
 * Internal handler for Object ID 0x01 (Battery %)
 *
 * Extracts battery level and stores in global state.
 * Monitors battery thresholds and sets LED error patterns (Story 2.4, AC9).
 */
static void battery_handler(const char *mac, uint8_t object_id,
                             const uint8_t *value, size_t value_len,
                             int8_t rssi)
{
    if (value_len != 1) {
        ESP_LOGW(TAG, "Invalid battery length: %d", value_len);
        return;
    }

    uint8_t battery_pct = value[0];  // 0-100%

    // Store battery level in global state (RAM only, not persisted)
    current_battery_pct = battery_pct;
    last_battery_update_ms = esp_timer_get_time() / 1000;  // Convert µs to ms

    // Battery threshold checks (FR11 / AC9)
    if (battery_pct < 10) {
        ESP_LOGE(TAG, "Battery CRITICAL: %d%% (MAC: %s)", battery_pct, mac);
        led_set_pattern(LED_ERROR, LED_PATTERN_ERROR_SINGLE);  // Single blink (FR4)
        battery_error_led_active = true;
    } else if (battery_pct < 20) {
        ESP_LOGW(TAG, "Battery LOW: %d%% (MAC: %s)", battery_pct, mac);
        led_set_pattern(LED_ERROR, LED_PATTERN_ERROR_SINGLE);  // Single blink (FR4)
        battery_error_led_active = true;
    } else {
        // Battery OK - log at DEBUG level only
        ESP_LOGD(TAG, "Battery: %d%% (MAC: %s, RSSI: %d dBm)",
                 battery_pct, mac, rssi);
        // Only clear error LED if it was set by battery handler (don't override other errors)
        if (battery_error_led_active) {
            led_set_pattern(LED_ERROR, LED_PATTERN_OFF);
            battery_error_led_active = false;
        }
    }
}

/**
 * Get current battery level
 */
uint8_t bthome_get_battery_level(void)
{
    return current_battery_pct;
}

/**
 * Parse BTHome v2 advertising packet
 *
 * Detects BTHome service UUID 0xFCD2, extracts device info byte,
 * iterates through OLV triplets, and calls registered handlers.
 */
esp_err_t bthome_parse_packet(const char *mac, const uint8_t *adv_data,
                               size_t adv_len, int8_t rssi)
{
    // Validate inputs
    if (!mac || !adv_data || adv_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    // Find BTHome v2 service data (UUID 0xFCD2)
    // Advertising data format: [AD Length] [AD Type] [Data...]
    // Service Data AD Type = 0x16 (16-bit UUID)
    // BTHome UUID = 0xFCD2 (little-endian: 0xD2, 0xFC)

    const uint8_t *service_data = NULL;
    size_t service_data_len = 0;

    size_t pos = 0;
    while (pos < adv_len) {
        uint8_t ad_len = adv_data[pos];
        if (ad_len == 0 || pos + ad_len + 1 > adv_len) {
            break;  // Invalid or end of data
        }

        uint8_t ad_type = adv_data[pos + 1];
        const uint8_t *ad_payload = &adv_data[pos + 2];
        uint8_t ad_payload_len = ad_len - 1;

        // Check for Service Data (0x16) with BTHome UUID (0xFCD2)
        if (ad_type == 0x16 && ad_payload_len >= 2) {
            uint16_t uuid = ad_payload[0] | (ad_payload[1] << 8);
            if (uuid == BTHOME_SERVICE_UUID) {
                service_data = &ad_payload[2];  // Skip UUID bytes
                service_data_len = ad_payload_len - 2;
                break;
            }
        }

        pos += ad_len + 1;
    }

    if (service_data == NULL) {
        // Not a BTHome packet (debug level - not an error)
        ESP_LOGD(TAG, "No BTHome service data in packet (MAC: %s)", mac);
        return ESP_ERR_NOT_FOUND;
    }

    if (service_data_len == 0) {
        ESP_LOGW(TAG, "Empty BTHome service data (MAC: %s)", mac);
        return ESP_ERR_INVALID_SIZE;
    }

    // Parse device info byte (first byte after UUID)
    // BTHome v2 Device Info byte format:
    //   Bit 0:   Encryption flag (0=unencrypted, 1=encrypted)
    //   Bit 1:   Reserved
    //   Bit 2:   Trigger-based device flag (0=interval advertising, 1=event-triggered)
    //   Bits 3-4: Reserved
    //   Bits 5-7: BTHome Version (010 = v2)
    uint8_t device_info = service_data[0];

    // Extract and validate BTHome version (bits 5-7)
    // Version 2 = 0b010 in bits 5-7, so device_info & 0xE0 should equal 0x40
    uint8_t version = (device_info >> 5) & 0x07;
    if (version != 2) {
        ESP_LOGD(TAG, "Unsupported BTHome version %d from %s (expected v2)", version, mac);
        return ESP_ERR_NOT_SUPPORTED;
    }

    // Check encryption bit (bit 0)
    // BTHome v2 supports encryption, but ESP32-C3 lacks crypto hardware
    // Reject encrypted packets early to save processing
    bool encrypted = (device_info & 0x01);
    if (encrypted) {
        ESP_LOGW(TAG, "Encrypted BTHome packet from %s - not supported", mac);
        return ESP_ERR_NOT_SUPPORTED;
    }

    // Extract trigger flag (bit 2)
    // Indicates event-triggered advertising (button press, motion detected)
    // vs interval-based periodic advertising
    bool trigger = (device_info & 0x04) != 0;
    (void)trigger;  // Will be used in future stories for relay logic

    ESP_LOGD(TAG, "BTHome v2 packet from %s (trigger=%d, data_len=%d)", mac, trigger, service_data_len);

    // Iterate through OLV triplets starting at byte 1
    // Format: [Object ID] [Value bytes...] (length implicit from Object ID)
    pos = 1;  // Skip device_info byte
    while (pos < service_data_len) {
        uint8_t object_id = service_data[pos++];
        uint8_t value_len = bthome_get_value_length(object_id);

        if (value_len == 0) {
            // Unknown Object ID - can't determine length, stop parsing
            ESP_LOGD(TAG, "Unknown Object ID 0x%02X - stopping parse", object_id);
            break;
        }

        if (pos + value_len > service_data_len) {
            ESP_LOGW(TAG, "Malformed packet - value exceeds data length");
            return ESP_ERR_INVALID_SIZE;
        }

        const uint8_t *value = &service_data[pos];
        pos += value_len;

        // Call registered handler if exists
        bthome_handler_t handler = bthome_find_handler(object_id);
        if (handler) {
            handler(mac, object_id, value, value_len, rssi);
        } else {
            // Log unhandled Object IDs at DEBUG level
            ESP_LOGD(TAG, "No handler for Object ID 0x%02X (value=0x%02X)", object_id, value[0]);
        }
    }

    return ESP_OK;
}

/**
 * Set learning mode callback (Story 3.2)
 */
void bthome_set_learning_callback(bthome_learning_callback_t callback)
{
    learning_callback = callback;
    ESP_LOGI(TAG, "Learning callback %s", callback ? "registered" : "cleared");
}

/**
 * Get current learning mode callback
 */
bthome_learning_callback_t bthome_get_learning_callback(void)
{
    return learning_callback;
}

/**
 * Convert MAC string to 6-byte array
 *
 * @param mac_str MAC address string (format: "AA:BB:CC:DD:EE:FF")
 * @param mac_out 6-byte output array
 * @return true on success, false on parse error
 */
bool bthome_mac_str_to_bytes(const char *mac_str, uint8_t *mac_out)
{
    if (mac_str == NULL || mac_out == NULL) {
        return false;
    }

    unsigned int bytes[6];
    int parsed = sscanf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X",
                        &bytes[0], &bytes[1], &bytes[2],
                        &bytes[3], &bytes[4], &bytes[5]);

    if (parsed != 6) {
        return false;
    }

    for (int i = 0; i < 6; i++) {
        mac_out[i] = (uint8_t)bytes[i];
    }

    return true;
}
