/*
 * NVS Storage Component - Public API
 *
 * Story 3.1: Implement NVS Storage Component with CRC Validation
 *
 * Abstracts NVS (Non-Volatile Storage) operations for configuration persistence.
 * Stores sensor configuration data with CRC32 validation for corruption detection.
 */

#ifndef NVS_STORAGE_H
#define NVS_STORAGE_H

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// NVS namespace for relay configuration (per architecture.md)
#define NVS_NAMESPACE "relay_config"

// NVS keys for configuration items
#define NVS_KEY_SENSOR_MAC      "sensor_mac"      // Registered sensor MAC address (string, 18 bytes)
#define NVS_KEY_SENSOR_TYPE     "sensor_type"     // Sensor type (uint8_t)
#define NVS_KEY_TIMER_SECONDS   "timer_seconds"   // Timer duration (uint16_t)
#define NVS_KEY_RETRIGGER_MODE  "retrigger_mode"  // Retrigger mode (uint8_t)
#define NVS_KEY_CONFIG_VERSION  "config_version"  // Config schema version (uint8_t)
#define NVS_KEY_CONFIG_CRC      "config_crc"      // CRC32 checksum (uint32_t)

// Config constants
#define CONFIG_VERSION          1       // Current config schema version
#define DEFAULT_TIMER_SECONDS   30      // Default timer duration in seconds
#define DEFAULT_RETRIGGER_MODE  0       // Default: RETRIGGER_EXTEND

// Timer limits (per FR19: 1-600 seconds)
#define TIMER_SECONDS_MIN       1
#define TIMER_SECONDS_MAX       600

// Forward declare sensor_type_t from sensor_button.h to avoid circular includes
// Values: SENSOR_TYPE_NONE=0, SENSOR_TYPE_BUTTON=1, SENSOR_TYPE_MOTION=2, SENSOR_TYPE_DOOR=3

/**
 * Retrigger mode enumeration
 */
typedef enum {
    RETRIGGER_EXTEND = 0,       // New trigger resets timer
    RETRIGGER_IGNORE = 1,       // New triggers ignored while active
} retrigger_mode_t;

/**
 * Sensor configuration structure (per architecture.md Decision 3)
 *
 * All fields are stored in NVS and validated with CRC32 checksum.
 * Uses sensor_type values: SENSOR_TYPE_NONE=0, BUTTON=1, MOTION=2, DOOR=3
 */
typedef struct {
    char sensor_mac[18];        // "AA:BB:CC:DD:EE:FF" format + null terminator
    uint8_t sensor_type;        // sensor_type_t enum value (0-3)
    uint16_t timer_seconds;     // Timer duration 1-600 seconds (FR19)
    uint8_t retrigger_mode;     // retrigger_mode_t enum value
    uint8_t config_version;     // Schema version for migration
    uint32_t config_crc;        // CRC32 checksum for integrity
} sensor_config_t;

// Sensor type constants (matching sensor_button.h sensor_type_t enum)
// These allow use without including sensor_button.h
#define NVS_SENSOR_TYPE_NONE    0
#define NVS_SENSOR_TYPE_BUTTON  1
#define NVS_SENSOR_TYPE_MOTION  2
#define NVS_SENSOR_TYPE_DOOR    3

/**
 * Initialize NVS storage
 *
 * Initializes NVS flash partition. Must be called before any NVS operations.
 * Safe to call multiple times (idempotent).
 *
 * @return ESP_OK on success
 *         ESP_ERR_NVS_NO_FREE_PAGES if NVS partition corrupted (erase and retry)
 *         ESP_ERR_NOT_FOUND if NVS partition not found
 */
esp_err_t nvs_storage_init(void);

/**
 * Get registered sensor MAC address
 *
 * Retrieves the MAC address of the registered sensor from NVS.
 * MAC format: "AA:BB:CC:DD:EE:FF" (17 chars + null terminator = 18 bytes)
 *
 * @param mac_out  Buffer to store MAC address string (must be at least 18 bytes)
 * @param mac_len  Size of mac_out buffer
 *
 * @return ESP_OK on success (MAC copied to mac_out)
 *         ESP_ERR_NVS_NOT_FOUND if no sensor registered
 *         ESP_ERR_INVALID_ARG if mac_out is NULL or mac_len too small
 *         ESP_FAIL on NVS read error
 */
esp_err_t nvs_get_registered_sensor_mac(char *mac_out, size_t mac_len);

/**
 * Set registered sensor MAC address
 *
 * Stores the MAC address of a sensor in NVS for persistent registration.
 * MAC format must be: "AA:BB:CC:DD:EE:FF" (uppercase, with colons)
 *
 * @param mac  MAC address string (format: "AA:BB:CC:DD:EE:FF")
 *
 * @return ESP_OK on success
 *         ESP_ERR_INVALID_ARG if mac is NULL or invalid format
 *         ESP_FAIL on NVS write error
 */
esp_err_t nvs_set_registered_sensor_mac(const char *mac);

/**
 * Check if a sensor is registered
 *
 * Quick check to determine if any sensor MAC is stored in NVS.
 *
 * @return true if sensor is registered, false otherwise
 */
bool nvs_is_sensor_registered(void);

/**
 * Clear registered sensor MAC
 *
 * Removes the registered sensor MAC from NVS.
 * Used for factory reset or sensor unregistration.
 *
 * @return ESP_OK on success
 *         ESP_ERR_NVS_NOT_FOUND if no sensor was registered
 *         ESP_FAIL on NVS erase error
 */
esp_err_t nvs_clear_registered_sensor(void);

/*
 * ============================================================================
 * Full Configuration API (Story 3.1)
 * ============================================================================
 */

/**
 * Calculate CRC32 checksum over config fields
 *
 * Calculates CRC32 over all config fields EXCLUDING config_crc itself.
 * Uses ESP-IDF ROM CRC32-LE function with IEEE 802.3 polynomial.
 *
 * @param config Pointer to sensor configuration
 * @return CRC32 checksum value
 */
uint32_t nvs_calculate_config_crc(const sensor_config_t *config);

/**
 * Save sensor configuration to NVS
 *
 * Validates config, calculates CRC32, and writes all fields atomically.
 * Only commits if all writes succeed.
 *
 * @param config Pointer to configuration to save (config_crc will be calculated)
 *
 * @return ESP_OK on success
 *         ESP_ERR_INVALID_ARG if config is NULL or validation fails
 *         ESP_FAIL on NVS write error
 */
esp_err_t nvs_save_config(const sensor_config_t *config);

/**
 * Load sensor configuration from NVS
 *
 * Reads all config fields and validates CRC32.
 * On CRC mismatch, calls relay_force_off() and sets error LED.
 *
 * @param config Pointer to buffer for loaded configuration
 *
 * @return ESP_OK on success (CRC valid)
 *         ESP_ERR_NVS_NOT_FOUND if no config stored
 *         ESP_ERR_INVALID_CRC if CRC mismatch (corruption detected)
 *         ESP_FAIL on NVS read error
 */
esp_err_t nvs_load_config(sensor_config_t *config);

/**
 * Validate sensor configuration fields
 *
 * Checks all field values are within valid ranges.
 * Does NOT validate CRC (use nvs_load_config for CRC validation).
 *
 * @param config Pointer to configuration to validate
 *
 * @return ESP_OK if all fields valid
 *         ESP_ERR_INVALID_ARG if any field value out of range
 */
esp_err_t nvs_validate_config(const sensor_config_t *config);

/**
 * Clear all configuration from NVS
 *
 * Erases all config keys (sensor_mac, sensor_type, timer, etc).
 *
 * @return ESP_OK on success
 *         ESP_ERR_NVS_NOT_FOUND if no config exists
 *         ESP_FAIL on NVS erase error
 */
esp_err_t nvs_clear_config(void);

/**
 * Initialize sensor config with default values
 *
 * Convenience function to initialize a config struct with sensible defaults:
 * - sensor_mac: empty string
 * - sensor_type: SENSOR_TYPE_NONE
 * - timer_seconds: DEFAULT_TIMER_SECONDS (30)
 * - retrigger_mode: RETRIGGER_EXTEND
 * - config_version: CONFIG_VERSION
 * - config_crc: 0 (calculated on save)
 *
 * @param config Pointer to config struct to initialize
 */
void nvs_init_config_defaults(sensor_config_t *config);

#endif // NVS_STORAGE_H
