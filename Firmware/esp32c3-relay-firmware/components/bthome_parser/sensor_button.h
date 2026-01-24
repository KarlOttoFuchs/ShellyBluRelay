/*
 * Button Event Handler - BTHome Object ID 0x3A
 *
 * Story 2.3: Decode Shelly BLU Button Events
 *
 * Handles button press events from Shelly BLU Button sensor.
 * Decodes BTHome v2 Object ID 0x3A (Button event) values.
 */

#ifndef SENSOR_BUTTON_H
#define SENSOR_BUTTON_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// BTHome v2 Object ID for Button events
#define BTHOME_OBJ_BUTTON 0x3A

// Button event values (per BTHome v2 spec)
// See: https://bthome.io/format/#button
typedef enum {
    BUTTON_EVENT_NONE = 0x00,           // No event (typically not sent)
    BUTTON_EVENT_SINGLE = 0x01,         // Single press
    BUTTON_EVENT_DOUBLE = 0x02,         // Double press
    BUTTON_EVENT_TRIPLE = 0x03,         // Triple press
    BUTTON_EVENT_LONG = 0x04,           // Long press
} button_event_t;

// Sensor types for config and event buffer (Story 2.4, 3.1)
// Values per architecture.md Decision 3 - must match NVS config schema
typedef enum {
    SENSOR_TYPE_NONE   = 0,     // No sensor registered (NVS config only)
    SENSOR_TYPE_BUTTON = 1,     // Shelly BLU Button
    SENSOR_TYPE_MOTION = 2,     // Shelly BLU Motion
    SENSOR_TYPE_DOOR   = 3,     // Shelly BLU Door/Window
} sensor_type_t;

// Unified event record for circular buffer (RAM only, not persisted)
// Supports button, motion, and door sensors (Story 2.4)
typedef struct {
    uint32_t timestamp_ms;              // Uptime in milliseconds
    char mac[18];                       // MAC address string (AA:BB:CC:DD:EE:FF)
    sensor_type_t sensor_type;          // BUTTON, MOTION, or DOOR
    uint8_t event_value;                // Raw event value (button: 0x01-0x04, motion/door: 0x00-0x01)
    uint8_t battery_pct;                // 0-100%, 255=unknown
    int8_t rssi;                        // Signal strength in dBm
} sensor_event_record_t;

// Legacy type alias for backwards compatibility (Story 2.3)
typedef sensor_event_record_t button_event_record_t;

/**
 * Button event handler callback
 *
 * Matches bthome_handler_t signature from bthome_parser.h.
 * Called when Object ID 0x3A (Button) is encountered in BTHome packet.
 *
 * @param mac        MAC address string (format: "AA:BB:CC:DD:EE:FF")
 * @param object_id  BTHome v2 Object ID (0x3A for Button)
 * @param value      Pointer to value bytes (button event code)
 * @param value_len  Length of value in bytes (must be 1)
 * @param rssi       Signal strength in dBm
 */
void button_event_handler(const char *mac, uint8_t object_id,
                          const uint8_t *value, size_t value_len,
                          int8_t rssi);

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
                      uint8_t event_value, uint8_t battery_pct, int8_t rssi);

/**
 * Get number of events in buffer
 *
 * @return Number of events (0-10)
 */
int sensor_event_get_count(void);

/**
 * Get last event from buffer
 *
 * @param record  Pointer to record structure to fill
 * @return true if event retrieved, false if buffer empty
 */
bool sensor_event_get_last(sensor_event_record_t *record);

/**
 * Get event history from buffer
 *
 * Returns events in reverse chronological order (newest first).
 *
 * @param records     Array to fill with event records
 * @param max_records Maximum number of records to retrieve
 * @return Number of records actually retrieved
 */
int sensor_event_get_history(sensor_event_record_t *records, int max_records);

/**
 * Clear event buffer
 *
 * Used for testing and initialization.
 */
void sensor_event_clear(void);

/**
 * Convert sensor event to human-readable string
 *
 * @param sensor_type  Sensor type
 * @param event_value  Raw event value
 * @return String representation of event
 */
const char* sensor_event_to_string(sensor_type_t sensor_type, uint8_t event_value);

// Legacy function aliases for backwards compatibility (Story 2.3)
#define button_event_get_count() sensor_event_get_count()
#define button_event_get_last(record) sensor_event_get_last(record)
#define button_event_get_history(records, max) sensor_event_get_history(records, max)
#define button_event_clear() sensor_event_clear()

#endif // SENSOR_BUTTON_H
