/*
 * Motion Event Handler - BTHome Object ID 0x21
 *
 * Story 2.4: Decode Motion & Door Events + Implement MAC Filtering
 *
 * Handles motion detection events from Shelly BLU Motion sensor.
 * Decodes BTHome v2 Object ID 0x21 (Motion) values.
 */

#ifndef SENSOR_MOTION_H
#define SENSOR_MOTION_H

#include <stdint.h>
#include <stddef.h>

// BTHome v2 Object ID for Motion events
#define BTHOME_OBJ_MOTION 0x21

// Motion event values (per BTHome v2 spec - boolean)
// See: https://bthome.io/format/#motion
typedef enum {
    MOTION_TIMEOUT = 0x00,              // No motion detected (timeout)
    MOTION_DETECTED = 0x01,             // Motion detected
} motion_event_t;

/**
 * Motion event handler callback
 *
 * Matches bthome_handler_t signature from bthome_parser.h.
 * Called when Object ID 0x21 (Motion) is encountered in BTHome packet.
 *
 * @param mac        MAC address string (format: "AA:BB:CC:DD:EE:FF")
 * @param object_id  BTHome v2 Object ID (0x21 for Motion)
 * @param value      Pointer to value bytes (motion event: 0x00 or 0x01)
 * @param value_len  Length of value in bytes (must be 1)
 * @param rssi       Signal strength in dBm
 */
void motion_event_handler(const char *mac, uint8_t object_id,
                          const uint8_t *value, size_t value_len,
                          int8_t rssi);

#endif // SENSOR_MOTION_H
