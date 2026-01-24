/*
 * Door Event Handler - BTHome Object ID 0x2D
 *
 * Story 2.4: Decode Motion & Door Events + Implement MAC Filtering
 *
 * Handles door/window opening events from Shelly BLU Door/Window sensor.
 * Decodes BTHome v2 Object ID 0x2D (Opening) values.
 */

#ifndef SENSOR_DOOR_H
#define SENSOR_DOOR_H

#include <stdint.h>
#include <stddef.h>

// BTHome v2 Object ID for Door/Window Opening events
#define BTHOME_OBJ_DOOR 0x2D

// Door event values (per BTHome v2 spec - boolean)
// See: https://bthome.io/format/#opening
typedef enum {
    DOOR_CLOSED = 0x00,                 // Door/window closed
    DOOR_OPEN = 0x01,                   // Door/window open
} door_event_t;

/**
 * Door event handler callback
 *
 * Matches bthome_handler_t signature from bthome_parser.h.
 * Called when Object ID 0x2D (Opening) is encountered in BTHome packet.
 *
 * @param mac        MAC address string (format: "AA:BB:CC:DD:EE:FF")
 * @param object_id  BTHome v2 Object ID (0x2D for Opening)
 * @param value      Pointer to value bytes (door event: 0x00 or 0x01)
 * @param value_len  Length of value in bytes (must be 1)
 * @param rssi       Signal strength in dBm
 */
void door_event_handler(const char *mac, uint8_t object_id,
                        const uint8_t *value, size_t value_len,
                        int8_t rssi);

#endif // SENSOR_DOOR_H
