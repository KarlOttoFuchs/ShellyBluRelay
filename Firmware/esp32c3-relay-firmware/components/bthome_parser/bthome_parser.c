/*
 * BTHome v2 Parser Component - Implementation
 *
 * Story 2.2: Implement BTHome v2 Parser with Handler Registry
 *
 * Parses BTHome v2 BLE advertising packets and dispatches to registered handlers.
 */

#include "bthome_parser.h"
#include "esp_log.h"
#include <string.h>

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
 *
 * @param object_id  BTHome Object ID
 * @return Value length in bytes, or 0 if unknown
 */
static uint8_t bthome_get_value_length(uint8_t object_id)
{
    switch (object_id) {
        // Sensor measurements (1 byte values)
        case 0x00:  // Packet ID (counter) → 1 byte (uint8)
        case 0x01:  // Battery %          → 1 byte (uint8)
        case 0x21:  // Motion              → 1 byte (uint8, 0x00/0x01)
        case 0x2D:  // Opening             → 1 byte (uint8, 0x00/0x01)
        case 0x3A:  // Button              → 1 byte (uint8, event type: 0x01=press, 0x02=double, 0x03=triple, 0x04=long)
            return 1;

        // Temperature, humidity (2 byte values - future extension)
        case 0x02:  // Temperature         → 2 bytes (sint16, 0.01°C)
        case 0x03:  // Humidity            → 2 bytes (uint16, 0.01%)
            return 2;

        // Unknown Object IDs
        default:
            ESP_LOGD(TAG, "Unknown Object ID 0x%02X", object_id);
            return 0;  // Signal to skip this object
    }
}

/**
 * Internal handler for Object ID 0x01 (Battery %)
 *
 * Extracts battery level and stores in global state.
 * Logs warnings for low battery thresholds (<20%, <10%).
 */
static void battery_handler(const char *mac, uint8_t object_id,
                             const uint8_t *value, size_t value_len,
                             int8_t rssi)
{
    if (value_len != 1) {
        ESP_LOGW(TAG, "Invalid battery length: %d", value_len);
        return;
    }

    current_battery_pct = value[0];  // 0-100%
    ESP_LOGI(TAG, "Battery: %d%% (MAC: %s, RSSI: %d dBm)",
             current_battery_pct, mac, rssi);

    // Battery warning thresholds (per FR11)
    if (current_battery_pct < 20) {
        ESP_LOGW(TAG, "Battery low: %d%%", current_battery_pct);
        // TODO: Set warning LED pattern (single blink per FR4) when LED component integrated
    }
    if (current_battery_pct < 10) {
        ESP_LOGE(TAG, "Battery critical: %d%%", current_battery_pct);
        // TODO: Set error LED pattern when LED component integrated
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
        ESP_LOGD(TAG, "No BTHome service data found (MAC: %s)", mac);
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

    ESP_LOGD(TAG, "BTHome v2 packet from %s (trigger=%d)", mac, trigger);

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
            // Log value even without handler (for debugging)
            if (value_len == 1) {
                ESP_LOGD(TAG, "No handler for Object ID 0x%02X, value=0x%02X", object_id, value[0]);
            } else if (value_len == 2) {
                ESP_LOGD(TAG, "No handler for Object ID 0x%02X, value=0x%02X%02X", object_id, value[0], value[1]);
            } else {
                ESP_LOGD(TAG, "No handler for Object ID 0x%02X, len=%d", object_id, value_len);
            }
        }
    }

    return ESP_OK;
}
