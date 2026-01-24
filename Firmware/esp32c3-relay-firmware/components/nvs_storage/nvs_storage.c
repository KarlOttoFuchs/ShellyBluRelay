/*
 * NVS Storage Component - Implementation
 *
 * Story 3.1: Implement NVS Storage Component with CRC Validation
 *
 * Implements NVS operations for configuration persistence with CRC32 validation.
 */

#include "nvs_storage.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "esp_rom_crc.h"
#include "relay_control.h"
#include "led_control.h"
#include <string.h>

static const char *TAG = "nvs_storage";

// Track NVS initialization state
static bool nvs_initialized = false;

/**
 * Initialize NVS storage
 */
esp_err_t nvs_storage_init(void)
{
    if (nvs_initialized) {
        ESP_LOGD(TAG, "NVS already initialized");
        return ESP_OK;
    }

    // Initialize NVS flash
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated or needs erase
        ESP_LOGW(TAG, "NVS partition needs erase, erasing...");
        ret = nvs_flash_erase();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "NVS erase failed: %s", esp_err_to_name(ret));
            return ret;
        }

        // Retry init after erase
        ret = nvs_flash_init();
    }

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NVS init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    nvs_initialized = true;
    ESP_LOGI(TAG, "NVS storage initialized");
    return ESP_OK;
}

/**
 * Get registered sensor MAC address
 */
esp_err_t nvs_get_registered_sensor_mac(char *mac_out, size_t mac_len)
{
    if (!mac_out || mac_len < 18) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!nvs_initialized) {
        ESP_LOGE(TAG, "NVS not initialized");
        return ESP_FAIL;
    }

    // Open NVS handle
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (ret != ESP_OK) {
        if (ret == ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGD(TAG, "NVS namespace not found (no sensor registered)");
        } else {
            ESP_LOGE(TAG, "NVS open failed: %s", esp_err_to_name(ret));
        }
        return ret;
    }

    // Get required size
    size_t required_size = 0;
    ret = nvs_get_str(handle, NVS_KEY_SENSOR_MAC, NULL, &required_size);
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGD(TAG, "Sensor MAC not found in NVS");
        nvs_close(handle);
        return ESP_ERR_NVS_NOT_FOUND;
    }

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NVS get_str (size) failed: %s", esp_err_to_name(ret));
        nvs_close(handle);
        return ESP_FAIL;
    }

    // Read string
    ret = nvs_get_str(handle, NVS_KEY_SENSOR_MAC, mac_out, &mac_len);
    nvs_close(handle);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NVS get_str (read) failed: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "Retrieved sensor MAC: %s", mac_out);
    return ESP_OK;
}

/**
 * Set registered sensor MAC address
 */
esp_err_t nvs_set_registered_sensor_mac(const char *mac)
{
    if (!mac) {
        return ESP_ERR_INVALID_ARG;
    }

    // Validate MAC format (basic check: length and colons)
    size_t len = strlen(mac);
    if (len != 17) {
        ESP_LOGE(TAG, "Invalid MAC length: %d (expected 17)", len);
        return ESP_ERR_INVALID_ARG;
    }

    // Check for colons at positions 2, 5, 8, 11, 14
    if (mac[2] != ':' || mac[5] != ':' || mac[8] != ':' ||
        mac[11] != ':' || mac[14] != ':') {
        ESP_LOGE(TAG, "Invalid MAC format (missing colons)");
        return ESP_ERR_INVALID_ARG;
    }

    if (!nvs_initialized) {
        ESP_LOGE(TAG, "NVS not initialized");
        return ESP_FAIL;
    }

    // Open NVS handle for read/write
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NVS open failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Write MAC string
    ret = nvs_set_str(handle, NVS_KEY_SENSOR_MAC, mac);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NVS set_str failed: %s", esp_err_to_name(ret));
        nvs_close(handle);
        return ESP_FAIL;
    }

    // Commit changes
    ret = nvs_commit(handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NVS commit failed: %s", esp_err_to_name(ret));
        nvs_close(handle);
        return ESP_FAIL;
    }

    nvs_close(handle);
    ESP_LOGI(TAG, "Sensor MAC registered: %s", mac);
    return ESP_OK;
}

/**
 * Check if a sensor is registered
 */
bool nvs_is_sensor_registered(void)
{
    char mac[18];
    esp_err_t ret = nvs_get_registered_sensor_mac(mac, sizeof(mac));
    return (ret == ESP_OK);
}

/**
 * Clear registered sensor MAC
 */
esp_err_t nvs_clear_registered_sensor(void)
{
    if (!nvs_initialized) {
        ESP_LOGE(TAG, "NVS not initialized");
        return ESP_FAIL;
    }

    // Open NVS handle for read/write
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (ret != ESP_OK) {
        if (ret == ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGD(TAG, "NVS namespace not found (nothing to clear)");
            return ESP_ERR_NVS_NOT_FOUND;
        }
        ESP_LOGE(TAG, "NVS open failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Erase key
    ret = nvs_erase_key(handle, NVS_KEY_SENSOR_MAC);
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGD(TAG, "Sensor MAC key not found (already cleared)");
        nvs_close(handle);
        return ESP_ERR_NVS_NOT_FOUND;
    }

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NVS erase_key failed: %s", esp_err_to_name(ret));
        nvs_close(handle);
        return ESP_FAIL;
    }

    // Commit changes
    ret = nvs_commit(handle);
    nvs_close(handle);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NVS commit failed: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Sensor MAC cleared");
    return ESP_OK;
}

/*
 * ============================================================================
 * Full Configuration API (Story 3.1)
 * ============================================================================
 */

/**
 * Initialize config struct with default values
 */
void nvs_init_config_defaults(sensor_config_t *config)
{
    if (!config) {
        return;
    }

    memset(config, 0, sizeof(sensor_config_t));
    config->sensor_type = NVS_SENSOR_TYPE_NONE;
    config->timer_seconds = DEFAULT_TIMER_SECONDS;
    config->retrigger_mode = DEFAULT_RETRIGGER_MODE;
    config->config_version = CONFIG_VERSION;
    config->config_crc = 0;
}

/**
 * Calculate CRC32 checksum over config fields (excluding config_crc)
 *
 * CRC32-LE calculation order:
 * 1. sensor_mac (18 bytes)
 * 2. sensor_type (1 byte)
 * 3. timer_seconds (2 bytes)
 * 4. retrigger_mode (1 byte)
 * 5. config_version (1 byte)
 */
uint32_t nvs_calculate_config_crc(const sensor_config_t *config)
{
    if (!config) {
        return 0;
    }

    uint32_t crc = 0xFFFFFFFF;  // Initial CRC value

    // Calculate CRC over fields in defined order
    crc = esp_rom_crc32_le(crc, (const uint8_t *)config->sensor_mac, sizeof(config->sensor_mac));
    crc = esp_rom_crc32_le(crc, &config->sensor_type, sizeof(config->sensor_type));
    crc = esp_rom_crc32_le(crc, (const uint8_t *)&config->timer_seconds, sizeof(config->timer_seconds));
    crc = esp_rom_crc32_le(crc, &config->retrigger_mode, sizeof(config->retrigger_mode));
    crc = esp_rom_crc32_le(crc, &config->config_version, sizeof(config->config_version));

    return crc;
}

/**
 * Validate sensor configuration fields
 */
esp_err_t nvs_validate_config(const sensor_config_t *config)
{
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    // Validate sensor_mac format (skip if SENSOR_TYPE_NONE)
    if (config->sensor_type != NVS_SENSOR_TYPE_NONE) {
        size_t len = strlen(config->sensor_mac);
        if (len != 17) {
            ESP_LOGE(TAG, "Invalid MAC length: %d (expected 17)", (int)len);
            return ESP_ERR_INVALID_ARG;
        }

        // Check for colons at positions 2, 5, 8, 11, 14
        if (config->sensor_mac[2] != ':' || config->sensor_mac[5] != ':' ||
            config->sensor_mac[8] != ':' || config->sensor_mac[11] != ':' ||
            config->sensor_mac[14] != ':') {
            ESP_LOGE(TAG, "Invalid MAC format (missing colons)");
            return ESP_ERR_INVALID_ARG;
        }

        // Validate uppercase hex digits
        for (int i = 0; i < 17; i++) {
            if (i == 2 || i == 5 || i == 8 || i == 11 || i == 14) continue;
            char c = config->sensor_mac[i];
            if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'))) {
                ESP_LOGE(TAG, "MAC must be uppercase hex: invalid char '%c' at pos %d", c, i);
                return ESP_ERR_INVALID_ARG;
            }
        }
    }

    // Validate sensor_type (0-3)
    if (config->sensor_type > NVS_SENSOR_TYPE_DOOR) {
        ESP_LOGE(TAG, "Invalid sensor_type: %d (must be 0-%d)", config->sensor_type, NVS_SENSOR_TYPE_DOOR);
        return ESP_ERR_INVALID_ARG;
    }

    // Validate timer_seconds (FR19: 1-600 seconds)
    if (config->timer_seconds < TIMER_SECONDS_MIN || config->timer_seconds > TIMER_SECONDS_MAX) {
        ESP_LOGE(TAG, "Invalid timer_seconds: %d (must be %d-%d)",
                 config->timer_seconds, TIMER_SECONDS_MIN, TIMER_SECONDS_MAX);
        return ESP_ERR_INVALID_ARG;
    }

    // Validate retrigger_mode (0-1)
    if (config->retrigger_mode > RETRIGGER_IGNORE) {
        ESP_LOGE(TAG, "Invalid retrigger_mode: %d (must be 0-%d)", config->retrigger_mode, RETRIGGER_IGNORE);
        return ESP_ERR_INVALID_ARG;
    }

    // Validate config_version (warn on mismatch but don't fail - for future migration)
    if (config->config_version != CONFIG_VERSION) {
        ESP_LOGW(TAG, "Config version mismatch: %d (expected %d)", config->config_version, CONFIG_VERSION);
    }

    return ESP_OK;
}

/**
 * Save sensor configuration to NVS with CRC validation
 */
esp_err_t nvs_save_config(const sensor_config_t *config)
{
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    // Validate config fields before saving
    esp_err_t ret = nvs_validate_config(config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Config validation failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Make local copy and calculate CRC
    sensor_config_t config_to_save = *config;
    config_to_save.config_crc = nvs_calculate_config_crc(&config_to_save);

    if (!nvs_initialized) {
        ESP_LOGE(TAG, "NVS not initialized");
        return ESP_FAIL;
    }

    // Open NVS namespace
    nvs_handle_t handle;
    ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NVS open failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Write all fields (don't commit until all succeed)
    bool write_success = true;

    if (nvs_set_str(handle, NVS_KEY_SENSOR_MAC, config_to_save.sensor_mac) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write sensor_mac");
        write_success = false;
    }
    if (write_success && nvs_set_u8(handle, NVS_KEY_SENSOR_TYPE, config_to_save.sensor_type) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write sensor_type");
        write_success = false;
    }
    if (write_success && nvs_set_u16(handle, NVS_KEY_TIMER_SECONDS, config_to_save.timer_seconds) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write timer_seconds");
        write_success = false;
    }
    if (write_success && nvs_set_u8(handle, NVS_KEY_RETRIGGER_MODE, config_to_save.retrigger_mode) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write retrigger_mode");
        write_success = false;
    }
    if (write_success && nvs_set_u8(handle, NVS_KEY_CONFIG_VERSION, config_to_save.config_version) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write config_version");
        write_success = false;
    }
    if (write_success && nvs_set_u32(handle, NVS_KEY_CONFIG_CRC, config_to_save.config_crc) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write config_crc");
        write_success = false;
    }

    if (!write_success) {
        ESP_LOGE(TAG, "NVS write failed, not committing");
        nvs_close(handle);
        return ESP_FAIL;
    }

    // Commit atomically
    ret = nvs_commit(handle);
    nvs_close(handle);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NVS commit failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Config saved (CRC: 0x%08lx)", (unsigned long)config_to_save.config_crc);
    return ESP_OK;
}

/**
 * Load sensor configuration from NVS with CRC validation
 */
esp_err_t nvs_load_config(sensor_config_t *config)
{
    if (!config) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!nvs_initialized) {
        ESP_LOGE(TAG, "NVS not initialized");
        return ESP_FAIL;
    }

    // Open NVS namespace
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (ret != ESP_OK) {
        if (ret == ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGD(TAG, "NVS namespace not found (no config stored)");
        } else {
            ESP_LOGE(TAG, "NVS open failed: %s", esp_err_to_name(ret));
        }
        return ret;
    }

    // Initialize config with defaults first
    nvs_init_config_defaults(config);

    // Read sensor_mac
    size_t mac_len = sizeof(config->sensor_mac);
    ret = nvs_get_str(handle, NVS_KEY_SENSOR_MAC, config->sensor_mac, &mac_len);
    if (ret == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGD(TAG, "No config stored in NVS");
        nvs_close(handle);
        return ESP_ERR_NVS_NOT_FOUND;
    }
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read sensor_mac: %s", esp_err_to_name(ret));
        nvs_close(handle);
        return ESP_FAIL;
    }

    // Read remaining fields
    ret = nvs_get_u8(handle, NVS_KEY_SENSOR_TYPE, &config->sensor_type);
    if (ret != ESP_OK && ret != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Failed to read sensor_type: %s", esp_err_to_name(ret));
        nvs_close(handle);
        return ESP_FAIL;
    }

    ret = nvs_get_u16(handle, NVS_KEY_TIMER_SECONDS, &config->timer_seconds);
    if (ret != ESP_OK && ret != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Failed to read timer_seconds: %s", esp_err_to_name(ret));
        nvs_close(handle);
        return ESP_FAIL;
    }

    ret = nvs_get_u8(handle, NVS_KEY_RETRIGGER_MODE, &config->retrigger_mode);
    if (ret != ESP_OK && ret != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Failed to read retrigger_mode: %s", esp_err_to_name(ret));
        nvs_close(handle);
        return ESP_FAIL;
    }

    ret = nvs_get_u8(handle, NVS_KEY_CONFIG_VERSION, &config->config_version);
    if (ret != ESP_OK && ret != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Failed to read config_version: %s", esp_err_to_name(ret));
        nvs_close(handle);
        return ESP_FAIL;
    }

    ret = nvs_get_u32(handle, NVS_KEY_CONFIG_CRC, &config->config_crc);
    if (ret != ESP_OK && ret != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Failed to read config_crc: %s", esp_err_to_name(ret));
        nvs_close(handle);
        return ESP_FAIL;
    }

    nvs_close(handle);

    // Calculate expected CRC and compare
    uint32_t expected_crc = nvs_calculate_config_crc(config);

    if (expected_crc != config->config_crc) {
        // SAFETY-CRITICAL: Force relay OFF FIRST before any error handling
        relay_force_off();

        ESP_LOGE(TAG, "NVS corruption detected: CRC mismatch (stored=0x%08lx, calc=0x%08lx)",
                 (unsigned long)config->config_crc, (unsigned long)expected_crc);

        // Set error LED double blink pattern (per FR4)
        led_set_pattern(LED_ERROR, LED_PATTERN_ERROR_DOUBLE);

        return ESP_ERR_INVALID_CRC;
    }

    ESP_LOGI(TAG, "Config loaded successfully (CRC: 0x%08lx)", (unsigned long)config->config_crc);
    return ESP_OK;
}

/**
 * Clear all configuration from NVS
 */
esp_err_t nvs_clear_config(void)
{
    if (!nvs_initialized) {
        ESP_LOGE(TAG, "NVS not initialized");
        return ESP_FAIL;
    }

    // Open NVS namespace
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (ret != ESP_OK) {
        if (ret == ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGD(TAG, "NVS namespace not found (nothing to clear)");
            return ESP_ERR_NVS_NOT_FOUND;
        }
        ESP_LOGE(TAG, "NVS open failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // Erase all config keys
    bool any_erased = false;

    if (nvs_erase_key(handle, NVS_KEY_SENSOR_MAC) == ESP_OK) any_erased = true;
    if (nvs_erase_key(handle, NVS_KEY_SENSOR_TYPE) == ESP_OK) any_erased = true;
    if (nvs_erase_key(handle, NVS_KEY_TIMER_SECONDS) == ESP_OK) any_erased = true;
    if (nvs_erase_key(handle, NVS_KEY_RETRIGGER_MODE) == ESP_OK) any_erased = true;
    if (nvs_erase_key(handle, NVS_KEY_CONFIG_VERSION) == ESP_OK) any_erased = true;
    if (nvs_erase_key(handle, NVS_KEY_CONFIG_CRC) == ESP_OK) any_erased = true;

    if (!any_erased) {
        ESP_LOGD(TAG, "No config keys found to erase");
        nvs_close(handle);
        return ESP_ERR_NVS_NOT_FOUND;
    }

    // Commit changes
    ret = nvs_commit(handle);
    nvs_close(handle);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NVS commit failed: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Config cleared from NVS");
    return ESP_OK;
}
