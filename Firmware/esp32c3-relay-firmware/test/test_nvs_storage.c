/*
 * Unit Tests for NVS Storage Component
 *
 * Story 3.1: Implement NVS Storage Component with CRC Validation
 *
 * Tests NVS storage operations, CRC validation, and config persistence.
 */

#include "unity.h"
#include "nvs_storage.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "sensor_button.h"  // For sensor_type_t enum values
#include <string.h>

// Test MAC addresses
static const char *TEST_MAC_VALID = "5C:C7:C1:F5:C9:AC";
static const char *TEST_MAC_OTHER = "AA:BB:CC:DD:EE:FF";

void setUp(void) {
    // Initialize NVS for each test
    // Note: In real hardware tests, this may already be initialized
    nvs_storage_init();
}

void tearDown(void) {
    // Clear registered sensor after each test
    nvs_clear_registered_sensor();
}

/**
 * Test: Save and retrieve MAC address
 */
void test_nvs_save_retrieve_mac(void) {
    // Save MAC
    esp_err_t ret = nvs_set_registered_sensor_mac(TEST_MAC_VALID);
    TEST_ASSERT_EQUAL_MESSAGE(ESP_OK, ret, "Should save MAC successfully");

    // Retrieve MAC
    char retrieved_mac[18] = {0};
    ret = nvs_get_registered_sensor_mac(retrieved_mac, sizeof(retrieved_mac));
    TEST_ASSERT_EQUAL_MESSAGE(ESP_OK, ret, "Should retrieve MAC successfully");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(TEST_MAC_VALID, retrieved_mac, "Retrieved MAC should match saved MAC");
}

/**
 * Test: No MAC registered returns NOT_FOUND
 */
void test_nvs_no_mac_registered(void) {
    // Ensure no MAC is registered (tearDown clears it)
    char mac[18] = {0};
    esp_err_t ret = nvs_get_registered_sensor_mac(mac, sizeof(mac));
    TEST_ASSERT_EQUAL_MESSAGE(ESP_ERR_NVS_NOT_FOUND, ret, "Should return NOT_FOUND when no MAC registered");
}

/**
 * Test: nvs_is_sensor_registered returns correct state
 */
void test_nvs_is_sensor_registered(void) {
    // Initially no sensor registered
    TEST_ASSERT_FALSE_MESSAGE(nvs_is_sensor_registered(), "Should be false when no sensor registered");

    // Register a sensor
    nvs_set_registered_sensor_mac(TEST_MAC_VALID);
    TEST_ASSERT_TRUE_MESSAGE(nvs_is_sensor_registered(), "Should be true after registering sensor");

    // Clear registration
    nvs_clear_registered_sensor();
    TEST_ASSERT_FALSE_MESSAGE(nvs_is_sensor_registered(), "Should be false after clearing sensor");
}

/**
 * Test: Invalid MAC format rejected (wrong length)
 */
void test_nvs_invalid_mac_length(void) {
    const char *short_mac = "AA:BB:CC:DD";
    esp_err_t ret = nvs_set_registered_sensor_mac(short_mac);
    TEST_ASSERT_EQUAL_MESSAGE(ESP_ERR_INVALID_ARG, ret, "Should reject MAC with wrong length");
}

/**
 * Test: Invalid MAC format rejected (missing colons)
 */
void test_nvs_invalid_mac_format(void) {
    const char *no_colons = "AABBCCDDEEFF1234";  // 17 chars but no colons
    esp_err_t ret = nvs_set_registered_sensor_mac(no_colons);
    TEST_ASSERT_EQUAL_MESSAGE(ESP_ERR_INVALID_ARG, ret, "Should reject MAC without colons");
}

/**
 * Test: MAC overwrite works correctly
 */
void test_nvs_mac_overwrite(void) {
    // Register first MAC
    esp_err_t ret = nvs_set_registered_sensor_mac(TEST_MAC_VALID);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Overwrite with second MAC
    ret = nvs_set_registered_sensor_mac(TEST_MAC_OTHER);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Verify second MAC is stored
    char retrieved_mac[18] = {0};
    ret = nvs_get_registered_sensor_mac(retrieved_mac, sizeof(retrieved_mac));
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL_STRING(TEST_MAC_OTHER, retrieved_mac);
}

/**
 * Test: MAC filtering logic - matching MAC should be accepted
 */
void test_mac_filter_match(void) {
    // Register a sensor
    nvs_set_registered_sensor_mac(TEST_MAC_VALID);

    // Check if sensor is registered
    TEST_ASSERT_TRUE(nvs_is_sensor_registered());

    // Retrieve and compare (simulating filter logic)
    char registered_mac[18] = {0};
    esp_err_t ret = nvs_get_registered_sensor_mac(registered_mac, sizeof(registered_mac));
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Simulate incoming packet MAC matching
    bool should_process = (strcmp(TEST_MAC_VALID, registered_mac) == 0);
    TEST_ASSERT_TRUE_MESSAGE(should_process, "Matching MAC should be processed");
}

/**
 * Test: MAC filtering logic - non-matching MAC should be rejected
 */
void test_mac_filter_no_match(void) {
    // Register a sensor
    nvs_set_registered_sensor_mac(TEST_MAC_VALID);

    // Retrieve registered MAC
    char registered_mac[18] = {0};
    esp_err_t ret = nvs_get_registered_sensor_mac(registered_mac, sizeof(registered_mac));
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Simulate incoming packet with DIFFERENT MAC
    bool should_process = (strcmp(TEST_MAC_OTHER, registered_mac) == 0);
    TEST_ASSERT_FALSE_MESSAGE(should_process, "Non-matching MAC should NOT be processed");
}

/**
 * Test: MAC filtering when no sensor registered - should reject all
 */
void test_mac_filter_none_registered(void) {
    // Ensure no sensor registered
    nvs_clear_registered_sensor();

    // Try to get registered MAC
    char registered_mac[18] = {0};
    esp_err_t ret = nvs_get_registered_sensor_mac(registered_mac, sizeof(registered_mac));

    // Should return NOT_FOUND, meaning all packets should be ignored
    TEST_ASSERT_EQUAL_MESSAGE(ESP_ERR_NVS_NOT_FOUND, ret,
        "Should return NOT_FOUND when no sensor registered");
}

/**
 * Test: Clear registered sensor
 */
void test_nvs_clear_sensor(void) {
    // Register a sensor
    nvs_set_registered_sensor_mac(TEST_MAC_VALID);
    TEST_ASSERT_TRUE(nvs_is_sensor_registered());

    // Clear it
    esp_err_t ret = nvs_clear_registered_sensor();
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Verify it's gone
    TEST_ASSERT_FALSE(nvs_is_sensor_registered());
}

/*
 * ============================================================================
 * Story 3.1: Full Configuration with CRC Validation Tests
 * ============================================================================
 */

/**
 * Test: Config struct size and layout
 */
void test_config_struct_size(void) {
    // Verify struct exists and has expected fields
    sensor_config_t config;
    memset(&config, 0, sizeof(config));

    // Verify field sizes by assignment
    strncpy(config.sensor_mac, "AA:BB:CC:DD:EE:FF", sizeof(config.sensor_mac));
    config.sensor_type = SENSOR_TYPE_BUTTON;
    config.timer_seconds = 600;
    config.retrigger_mode = RETRIGGER_IGNORE;
    config.config_version = CONFIG_VERSION;
    config.config_crc = 0xDEADBEEF;

    TEST_ASSERT_EQUAL_STRING("AA:BB:CC:DD:EE:FF", config.sensor_mac);
    TEST_ASSERT_EQUAL(SENSOR_TYPE_BUTTON, config.sensor_type);
    TEST_ASSERT_EQUAL(600, config.timer_seconds);
    TEST_ASSERT_EQUAL(RETRIGGER_IGNORE, config.retrigger_mode);
    TEST_ASSERT_EQUAL(CONFIG_VERSION, config.config_version);
    TEST_ASSERT_EQUAL_HEX32(0xDEADBEEF, config.config_crc);
}

/**
 * Test: Enum values are correct
 */
void test_enum_values(void) {
    TEST_ASSERT_EQUAL(0, SENSOR_TYPE_NONE);
    TEST_ASSERT_EQUAL(1, SENSOR_TYPE_BUTTON);
    TEST_ASSERT_EQUAL(2, SENSOR_TYPE_MOTION);
    TEST_ASSERT_EQUAL(3, SENSOR_TYPE_DOOR);

    TEST_ASSERT_EQUAL(0, RETRIGGER_EXTEND);
    TEST_ASSERT_EQUAL(1, RETRIGGER_IGNORE);
}

/**
 * Test: Default config values
 */
void test_config_defaults(void) {
    sensor_config_t config;
    nvs_init_config_defaults(&config);

    TEST_ASSERT_EQUAL_STRING("", config.sensor_mac);
    TEST_ASSERT_EQUAL(SENSOR_TYPE_NONE, config.sensor_type);
    TEST_ASSERT_EQUAL(DEFAULT_TIMER_SECONDS, config.timer_seconds);
    TEST_ASSERT_EQUAL(DEFAULT_RETRIGGER_MODE, config.retrigger_mode);
    TEST_ASSERT_EQUAL(CONFIG_VERSION, config.config_version);
    TEST_ASSERT_EQUAL(0, config.config_crc);
}

/**
 * Test: CRC32 calculation consistency
 */
void test_crc32_consistency(void) {
    sensor_config_t config = {
        .sensor_mac = "5C:C7:C1:F5:C9:AC",
        .sensor_type = SENSOR_TYPE_BUTTON,
        .timer_seconds = 30,
        .retrigger_mode = RETRIGGER_EXTEND,
        .config_version = CONFIG_VERSION,
        .config_crc = 0
    };

    uint32_t crc1 = nvs_calculate_config_crc(&config);
    uint32_t crc2 = nvs_calculate_config_crc(&config);
    TEST_ASSERT_EQUAL_HEX32(crc1, crc2);
    TEST_ASSERT_NOT_EQUAL(0, crc1);  // CRC should not be zero
}

/**
 * Test: CRC32 changes when data changes
 */
void test_crc32_changes_with_data(void) {
    sensor_config_t config = {
        .sensor_mac = "5C:C7:C1:F5:C9:AC",
        .sensor_type = SENSOR_TYPE_BUTTON,
        .timer_seconds = 30,
        .retrigger_mode = RETRIGGER_EXTEND,
        .config_version = CONFIG_VERSION,
        .config_crc = 0
    };

    uint32_t crc_original = nvs_calculate_config_crc(&config);

    // Change timer_seconds
    config.timer_seconds = 60;
    uint32_t crc_changed = nvs_calculate_config_crc(&config);
    TEST_ASSERT_NOT_EQUAL(crc_original, crc_changed);

    // Restore and verify CRC matches original
    config.timer_seconds = 30;
    uint32_t crc_restored = nvs_calculate_config_crc(&config);
    TEST_ASSERT_EQUAL_HEX32(crc_original, crc_restored);
}

/**
 * Test: Save and load config with CRC validation
 */
void test_nvs_save_load_config(void) {
    sensor_config_t config_in = {
        .sensor_mac = "AA:BB:CC:DD:EE:FF",
        .sensor_type = SENSOR_TYPE_BUTTON,
        .timer_seconds = 60,
        .retrigger_mode = RETRIGGER_EXTEND,
        .config_version = CONFIG_VERSION,
        .config_crc = 0  // Will be calculated by save
    };

    esp_err_t ret = nvs_save_config(&config_in);
    TEST_ASSERT_EQUAL_MESSAGE(ESP_OK, ret, "Save should succeed");

    sensor_config_t config_out = {0};
    ret = nvs_load_config(&config_out);
    TEST_ASSERT_EQUAL_MESSAGE(ESP_OK, ret, "Load should succeed");

    TEST_ASSERT_EQUAL_STRING(config_in.sensor_mac, config_out.sensor_mac);
    TEST_ASSERT_EQUAL(config_in.sensor_type, config_out.sensor_type);
    TEST_ASSERT_EQUAL(config_in.timer_seconds, config_out.timer_seconds);
    TEST_ASSERT_EQUAL(config_in.retrigger_mode, config_out.retrigger_mode);
    TEST_ASSERT_EQUAL(config_in.config_version, config_out.config_version);
    TEST_ASSERT_NOT_EQUAL(0, config_out.config_crc);  // CRC was calculated
}

/**
 * Test: CRC mismatch detection
 */
void test_nvs_crc_mismatch(void) {
    // Save valid config
    sensor_config_t config = {
        .sensor_mac = "AA:BB:CC:DD:EE:FF",
        .sensor_type = SENSOR_TYPE_MOTION,
        .timer_seconds = 30,
        .retrigger_mode = RETRIGGER_IGNORE,
        .config_version = CONFIG_VERSION,
        .config_crc = 0
    };
    nvs_save_config(&config);

    // Corrupt CRC directly in NVS
    nvs_handle_t handle;
    esp_err_t ret = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    nvs_set_u32(handle, NVS_KEY_CONFIG_CRC, 0xDEADBEEF);  // Invalid CRC
    nvs_commit(handle);
    nvs_close(handle);

    // Load should detect corruption
    sensor_config_t loaded = {0};
    ret = nvs_load_config(&loaded);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_CRC, ret);
}

/**
 * Test: Config validation catches invalid timer
 */
void test_nvs_validate_invalid_timer(void) {
    sensor_config_t config = {
        .sensor_mac = "AA:BB:CC:DD:EE:FF",
        .sensor_type = SENSOR_TYPE_BUTTON,
        .timer_seconds = 999,  // Invalid: must be 1-600
        .retrigger_mode = RETRIGGER_EXTEND,
        .config_version = CONFIG_VERSION,
        .config_crc = 0
    };

    esp_err_t ret = nvs_validate_config(&config);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

/**
 * Test: Config validation catches timer = 0
 */
void test_nvs_validate_timer_zero(void) {
    sensor_config_t config = {
        .sensor_mac = "AA:BB:CC:DD:EE:FF",
        .sensor_type = SENSOR_TYPE_BUTTON,
        .timer_seconds = 0,  // Invalid: must be >= 1
        .retrigger_mode = RETRIGGER_EXTEND,
        .config_version = CONFIG_VERSION,
        .config_crc = 0
    };

    esp_err_t ret = nvs_validate_config(&config);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

/**
 * Test: Config validation catches lowercase MAC
 */
void test_nvs_validate_lowercase_mac(void) {
    sensor_config_t config = {
        .sensor_mac = "aa:bb:cc:dd:ee:ff",  // Invalid: lowercase
        .sensor_type = SENSOR_TYPE_BUTTON,
        .timer_seconds = 30,
        .retrigger_mode = RETRIGGER_EXTEND,
        .config_version = CONFIG_VERSION,
        .config_crc = 0
    };

    esp_err_t ret = nvs_validate_config(&config);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

/**
 * Test: Config validation passes for valid config
 */
void test_nvs_validate_valid_config(void) {
    sensor_config_t config = {
        .sensor_mac = "AA:BB:CC:DD:EE:FF",
        .sensor_type = SENSOR_TYPE_DOOR,
        .timer_seconds = 300,
        .retrigger_mode = RETRIGGER_IGNORE,
        .config_version = CONFIG_VERSION,
        .config_crc = 0
    };

    esp_err_t ret = nvs_validate_config(&config);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
}

/**
 * Test: Config validation with SENSOR_TYPE_NONE allows empty MAC
 */
void test_nvs_validate_none_type_empty_mac(void) {
    sensor_config_t config = {
        .sensor_mac = "",  // Empty is OK for SENSOR_TYPE_NONE
        .sensor_type = SENSOR_TYPE_NONE,
        .timer_seconds = 30,
        .retrigger_mode = RETRIGGER_EXTEND,
        .config_version = CONFIG_VERSION,
        .config_crc = 0
    };

    esp_err_t ret = nvs_validate_config(&config);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
}

/**
 * Test: Clear config erases all fields
 */
void test_nvs_clear_config(void) {
    // Save config first
    sensor_config_t config = {
        .sensor_mac = "AA:BB:CC:DD:EE:FF",
        .sensor_type = SENSOR_TYPE_DOOR,
        .timer_seconds = 120,
        .retrigger_mode = RETRIGGER_IGNORE,
        .config_version = CONFIG_VERSION,
        .config_crc = 0
    };
    nvs_save_config(&config);

    // Clear
    esp_err_t ret = nvs_clear_config();
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Load should return NOT_FOUND
    sensor_config_t loaded = {0};
    ret = nvs_load_config(&loaded);
    TEST_ASSERT_EQUAL(ESP_ERR_NVS_NOT_FOUND, ret);
}

/**
 * Test: Invalid sensor_type rejected
 */
void test_nvs_validate_invalid_sensor_type(void) {
    sensor_config_t config = {
        .sensor_mac = "AA:BB:CC:DD:EE:FF",
        .sensor_type = 99,  // Invalid
        .timer_seconds = 30,
        .retrigger_mode = RETRIGGER_EXTEND,
        .config_version = CONFIG_VERSION,
        .config_crc = 0
    };

    esp_err_t ret = nvs_validate_config(&config);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

/**
 * Test: Invalid retrigger_mode rejected
 */
void test_nvs_validate_invalid_retrigger_mode(void) {
    sensor_config_t config = {
        .sensor_mac = "AA:BB:CC:DD:EE:FF",
        .sensor_type = SENSOR_TYPE_BUTTON,
        .timer_seconds = 30,
        .retrigger_mode = 99,  // Invalid
        .config_version = CONFIG_VERSION,
        .config_crc = 0
    };

    esp_err_t ret = nvs_validate_config(&config);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

/**
 * Test: Backward compatibility - MAC-only functions still work
 */
void test_backward_compatibility_mac_functions(void) {
    // Clear any existing config
    nvs_clear_config();

    // Use legacy MAC-only function
    esp_err_t ret = nvs_set_registered_sensor_mac(TEST_MAC_VALID);
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    // Should be able to retrieve via legacy function
    char mac_out[18] = {0};
    ret = nvs_get_registered_sensor_mac(mac_out, sizeof(mac_out));
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL_STRING(TEST_MAC_VALID, mac_out);

    // nvs_is_sensor_registered should return true
    TEST_ASSERT_TRUE(nvs_is_sensor_registered());
}

// Test runner
void app_main(void) {
    UNITY_BEGIN();

    // Story 2.4: Legacy MAC-only tests (backward compatibility)
    RUN_TEST(test_nvs_save_retrieve_mac);
    RUN_TEST(test_nvs_no_mac_registered);
    RUN_TEST(test_nvs_is_sensor_registered);
    RUN_TEST(test_nvs_invalid_mac_length);
    RUN_TEST(test_nvs_invalid_mac_format);
    RUN_TEST(test_nvs_mac_overwrite);
    RUN_TEST(test_mac_filter_match);
    RUN_TEST(test_mac_filter_no_match);
    RUN_TEST(test_mac_filter_none_registered);
    RUN_TEST(test_nvs_clear_sensor);

    // Story 3.1: Config struct and enum tests
    RUN_TEST(test_config_struct_size);
    RUN_TEST(test_enum_values);
    RUN_TEST(test_config_defaults);

    // Story 3.1: CRC32 calculation tests
    RUN_TEST(test_crc32_consistency);
    RUN_TEST(test_crc32_changes_with_data);

    // Story 3.1: Full config save/load/validate tests
    RUN_TEST(test_nvs_save_load_config);
    RUN_TEST(test_nvs_crc_mismatch);
    RUN_TEST(test_nvs_validate_invalid_timer);
    RUN_TEST(test_nvs_validate_timer_zero);
    RUN_TEST(test_nvs_validate_lowercase_mac);
    RUN_TEST(test_nvs_validate_valid_config);
    RUN_TEST(test_nvs_validate_none_type_empty_mac);
    RUN_TEST(test_nvs_clear_config);
    RUN_TEST(test_nvs_validate_invalid_sensor_type);
    RUN_TEST(test_nvs_validate_invalid_retrigger_mode);

    // Story 3.1: Backward compatibility
    RUN_TEST(test_backward_compatibility_mac_functions);

    UNITY_END();
}
