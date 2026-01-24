/*
 * BLE Scanner Component - Public API
 *
 * Story 2.1: Initialize BLE Stack & Scan for Advertising Packets
 *
 * This component provides BLE scanning functionality using NimBLE stack.
 * Receives advertising packets from nearby BLE devices without establishing connections.
 */

#ifndef BLE_SCANNER_H
#define BLE_SCANNER_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#define BLE_MAX_TRACKED_DEVICES 32
#define BLE_DEVICE_TIMEOUT_MS (60 * 1000)  // 60 seconds

/**
 * Tracked BLE device structure
 */
typedef struct {
    char mac[18];           // "AA:BB:CC:DD:EE:FF" + null terminator
    int8_t rssi;            // Last seen RSSI value in dBm
    uint32_t last_seen_ms;  // Timestamp when last seen (milliseconds since boot)
    bool active;            // Entry is in use
} ble_tracked_device_t;

/**
 * Initialize the BLE scanner component
 *
 * Initializes NimBLE stack and starts passive BLE scanning.
 * On failure, calls relay_force_off() and sets triple blink error LED pattern.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t ble_scanner_init(void);

/**
 * Get list of recently seen BLE devices
 *
 * Returns devices seen within the last 60 seconds.
 * Thread-safe function that can be called from any task.
 *
 * @param devices Output buffer for device list (caller allocates)
 * @param max_devices Maximum number of devices to return (size of devices array)
 * @param count Output parameter for actual number of devices returned
 * @return ESP_OK on success
 * @return ESP_ERR_INVALID_ARG if devices is NULL or count is NULL
 * @return ESP_ERR_INVALID_STATE if ble_scanner_init() not called
 */
esp_err_t ble_scanner_get_devices(ble_tracked_device_t *devices, uint32_t max_devices, uint32_t *count);

/**
 * Invalidate MAC filter cache
 *
 * Call this function after changing sensor registration (via nvs_set_registered_sensor_mac
 * or nvs_clear_registered_sensor) to force the scanner to reload the registered MAC
 * from NVS on the next packet.
 *
 * Story 2.4: Added for MAC filtering cache management.
 */
void ble_scanner_invalidate_mac_cache(void);

#endif // BLE_SCANNER_H
