/*
 * BTHome v2 Parser Component - Public API
 *
 * Story 2.2: Implement BTHome v2 Parser with Handler Registry
 *
 * Parses BTHome v2 BLE advertising packets from Shelly BLU sensors.
 * Uses extensible handler registry pattern for adding new sensor types.
 */

#ifndef BTHOME_PARSER_H
#define BTHOME_PARSER_H

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// BTHome v2 service UUID (16-bit, little-endian in advertising data)
#define BTHOME_SERVICE_UUID 0xFCD2

// Battery level sentinel value (unknown/not reported)
#define BATTERY_UNKNOWN 255

/**
 * Handler function signature for BTHome object handlers
 *
 * Called when a registered Object ID is encountered in a BTHome packet.
 *
 * @param mac        MAC address string (format: "AA:BB:CC:DD:EE:FF")
 * @param object_id  BTHome v2 Object ID (e.g., 0x3A for Button)
 * @param value      Pointer to value bytes
 * @param value_len  Length of value in bytes
 * @param rssi       Signal strength in dBm
 */
typedef void (*bthome_handler_t)(const char *mac, uint8_t object_id,
                                  const uint8_t *value, size_t value_len,
                                  int8_t rssi);

/**
 * Initialize BTHome parser
 *
 * Sets up handler registry and registers internal handlers (battery).
 *
 * @return ESP_OK on success, ESP_FAIL on initialization failure
 */
esp_err_t bthome_init(void);

/**
 * Parse BTHome v2 advertising packet
 *
 * Detects BTHome v2 service UUID (0xFCD2), extracts device info byte,
 * iterates through OLV triplets, and calls registered handlers.
 *
 * @param mac       MAC address string (format: "AA:BB:CC:DD:EE:FF")
 * @param adv_data  Advertising data buffer
 * @param adv_len   Length of advertising data
 * @param rssi      Signal strength in dBm
 *
 * @return ESP_OK on successful parse
 *         ESP_ERR_INVALID_ARG if mac or adv_data is NULL
 *         ESP_ERR_NOT_FOUND if packet is not BTHome v2
 *         ESP_ERR_NOT_SUPPORTED if packet is encrypted
 */
esp_err_t bthome_parse_packet(const char *mac, const uint8_t *adv_data,
                               size_t adv_len, int8_t rssi);

/**
 * Register handler for BTHome Object ID
 *
 * Allows custom handlers to be registered for specific Object IDs.
 * Used for extensibility (adding new sensor types).
 *
 * @param object_id  BTHome Object ID to handle (e.g., 0x3A for Button)
 * @param handler    Function pointer to handler callback
 *
 * @return ESP_OK on success
 *         ESP_ERR_INVALID_ARG if handler is NULL
 *         ESP_ERR_NO_MEM if handler registry is full
 */
esp_err_t bthome_register_handler(uint8_t object_id, bthome_handler_t handler);

/**
 * Get current battery level from last parsed packet
 *
 * Returns battery percentage (0-100%) from most recent packet with
 * Object ID 0x01 (Battery %).
 *
 * @return Battery percentage 0-100%, or BATTERY_UNKNOWN (255) if not available
 */
uint8_t bthome_get_battery_level(void);

/**
 * Learning mode callback signature (Story 3.2)
 *
 * Called by sensor handlers when an event is detected.
 * Used to capture sensor info for registration in learning mode.
 *
 * @param mac         6-byte MAC address array (raw bytes, not string)
 * @param sensor_type Sensor type (1=BUTTON, 2=MOTION, 3=DOOR)
 * @param battery_pct Battery percentage (0-100, 255=unknown)
 * @param rssi        Signal strength in dBm
 */
typedef void (*bthome_learning_callback_t)(const uint8_t *mac, uint8_t sensor_type,
                                            uint8_t battery_pct, int8_t rssi);

/**
 * Register learning mode callback (Story 3.2)
 *
 * Registers a callback to be invoked whenever a sensor event is detected.
 * Used by learning_mode.c to capture sensor info for registration.
 *
 * @param callback Function to call on sensor events, or NULL to clear
 */
void bthome_set_learning_callback(bthome_learning_callback_t callback);

/**
 * Get current learning mode callback
 *
 * @return Current callback or NULL if not set
 */
bthome_learning_callback_t bthome_get_learning_callback(void);

/**
 * Convert MAC string to 6-byte array
 *
 * @param mac_str MAC address string (format: "AA:BB:CC:DD:EE:FF")
 * @param mac_out 6-byte output array
 * @return true on success, false on parse error
 */
bool bthome_mac_str_to_bytes(const char *mac_str, uint8_t *mac_out);

#endif // BTHOME_PARSER_H
