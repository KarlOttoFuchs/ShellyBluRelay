/*
 * BTHome v2 Parser - Unit Tests
 *
 * Story 2.2: Test BTHome packet parsing, handler registry, battery extraction
 */

#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "bthome_parser.h"

// Test handler callback tracker
static bool test_handler_called = false;
static uint8_t test_handler_object_id = 0;
static uint8_t test_handler_value = 0;

// Test handler callback
static void test_handler(const char *mac, uint8_t object_id,
                         const uint8_t *value, size_t value_len, int8_t rssi)
{
    test_handler_called = true;
    test_handler_object_id = object_id;
    if (value_len > 0) {
        test_handler_value = value[0];
    }
}

void setUp(void) {
    // Reset test state before each test
    test_handler_called = false;
    test_handler_object_id = 0;
    test_handler_value = 0;

    // Initialize parser
    bthome_init();
}

void tearDown(void) {
    // Cleanup after each test
}

// Test 1: BTHome UUID Detection - Valid BTHome Packet
void test_bthome_uuid_detection_valid(void) {
    // Real Shelly BLU Button packet structure (simplified)
    // Device info byte: 0x40 = version 2 (bits 5-7 = 010), no trigger, no encryption
    uint8_t packet[] = {
        0x02, 0x01, 0x06,                    // Flags
        0x03, 0x03, 0xD2, 0xFC,              // Service UUID List: 0xFCD2 (BTHome v2)
        0x06, 0x16, 0xD2, 0xFC,              // Service Data (UUID 0xFCD2, 6 bytes data)
        0x40,                                // device_info: v2, trigger=0, encrypted=0
        0x01, 0x64,                          // Object ID 0x01 (Battery), value=100%
    };

    const char *mac = "AA:BB:CC:DD:EE:FF";
    esp_err_t ret = bthome_parse_packet(mac, packet, sizeof(packet), -50);

    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL(100, bthome_get_battery_level());
}

// Test 2: BTHome UUID Detection - Non-BTHome Packet
void test_bthome_uuid_detection_invalid(void) {
    // Packet with different service UUID (not BTHome)
    uint8_t packet[] = {
        0x02, 0x01, 0x06,           // Flags
        0x03, 0x03, 0xFF, 0xFF,     // Service UUID List: 0xFFFF (not BTHome)
    };

    const char *mac = "AA:BB:CC:DD:EE:FF";
    esp_err_t ret = bthome_parse_packet(mac, packet, sizeof(packet), -50);

    TEST_ASSERT_EQUAL(ESP_ERR_NOT_FOUND, ret);
}

// Test 3: Device Info Parsing - Unencrypted v2 Packet
void test_device_info_unencrypted(void) {
    // BTHome v2 packet with device_info = 0x40 (v2, no encryption, no trigger)
    // Bits 5-7 = 010 (v2), bit 2 = 0 (no trigger), bit 0 = 0 (no encryption)
    uint8_t packet[] = {
        0x06, 0x16, 0xD2, 0xFC,     // Service Data header
        0x40,                       // device_info: v2, encrypted=0, trigger=0
        0x01, 0x50,                 // Battery 80%
    };

    const char *mac = "AA:BB:CC:DD:EE:FF";
    esp_err_t ret = bthome_parse_packet(mac, packet, sizeof(packet), -50);

    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL(80, bthome_get_battery_level());
}

// Test 4: Device Info Parsing - Encrypted Packet (Should Reject)
void test_device_info_encrypted(void) {
    // BTHome v2 packet with device_info = 0x41 (v2, encrypted bit set)
    // Bits 5-7 = 010 (v2), bit 0 = 1 (encrypted)
    uint8_t packet[] = {
        0x06, 0x16, 0xD2, 0xFC,     // Service Data header
        0x41,                       // device_info: v2, encrypted=1
        0x01, 0x64,                 // Battery 100%
    };

    const char *mac = "AA:BB:CC:DD:EE:FF";
    esp_err_t ret = bthome_parse_packet(mac, packet, sizeof(packet), -50);

    // Should return ESP_ERR_NOT_SUPPORTED for encrypted packets
    TEST_ASSERT_EQUAL(ESP_ERR_NOT_SUPPORTED, ret);
}

// Test 5: Device Info Parsing - Wrong Version (Should Reject)
void test_device_info_wrong_version(void) {
    // BTHome packet with device_info = 0x00 (version 0, not v2)
    // Should be rejected since we only support BTHome v2
    uint8_t packet[] = {
        0x06, 0x16, 0xD2, 0xFC,     // Service Data header
        0x00,                       // device_info: version=0 (not v2)
        0x01, 0x64,                 // Battery 100%
    };

    const char *mac = "AA:BB:CC:DD:EE:FF";
    esp_err_t ret = bthome_parse_packet(mac, packet, sizeof(packet), -50);

    // Should return ESP_ERR_NOT_SUPPORTED for non-v2 packets
    TEST_ASSERT_EQUAL(ESP_ERR_NOT_SUPPORTED, ret);
}

// Test 6: Handler Registration
void test_handler_registration(void) {
    esp_err_t ret = bthome_register_handler(0x3A, test_handler);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
}

// Test 7: Handler Registration - NULL Handler
void test_handler_registration_null(void) {
    esp_err_t ret = bthome_register_handler(0x3A, NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

// Test 8: Handler Callback Invocation
void test_handler_callback(void) {
    // Register handler for Button event (Object ID 0x3A)
    bthome_register_handler(0x3A, test_handler);

    // Packet with button single press
    // device_info: 0x44 = v2 (bits 5-7 = 010), trigger=1 (bit 2), encrypted=0
    uint8_t packet[] = {
        0x06, 0x16, 0xD2, 0xFC,     // Service Data header
        0x44,                       // device_info: v2, trigger=1
        0x3A, 0x01,                 // Object ID 0x3A (Button), value=0x01 (single press)
    };

    const char *mac = "AA:BB:CC:DD:EE:FF";
    bthome_parse_packet(mac, packet, sizeof(packet), -50);

    TEST_ASSERT_TRUE(test_handler_called);
    TEST_ASSERT_EQUAL(0x3A, test_handler_object_id);
    TEST_ASSERT_EQUAL(0x01, test_handler_value);
}

// Test 9: Battery Extraction
void test_battery_extraction(void) {
    uint8_t packet[] = {
        0x06, 0x16, 0xD2, 0xFC,
        0x40,           // device_info: v2, no trigger, no encryption
        0x01, 0x55,     // Battery 85%
    };

    const char *mac = "AA:BB:CC:DD:EE:FF";
    bthome_parse_packet(mac, packet, sizeof(packet), -50);

    TEST_ASSERT_EQUAL(85, bthome_get_battery_level());
}

// Test 10: Multiple OLV Triplets
void test_multiple_olv_triplets(void) {
    // Register handler for button
    bthome_register_handler(0x3A, test_handler);

    // Packet with Battery AND Button
    // device_info: 0x44 = v2, trigger=1 (button event)
    uint8_t packet[] = {
        0x09, 0x16, 0xD2, 0xFC,     // Service Data header
        0x44,                       // device_info: v2, trigger=1
        0x01, 0x64,                 // Battery 100%
        0x3A, 0x02,                 // Button double press
    };

    const char *mac = "AA:BB:CC:DD:EE:FF";
    bthome_parse_packet(mac, packet, sizeof(packet), -50);

    // Both battery AND button handler should be called
    TEST_ASSERT_EQUAL(100, bthome_get_battery_level());
    TEST_ASSERT_TRUE(test_handler_called);
    TEST_ASSERT_EQUAL(0x02, test_handler_value);  // Double press
}

// Test 11: Real Shelly BLU Button Packet
void test_real_shelly_button_packet(void) {
    // Register button handler
    bthome_register_handler(0x3A, test_handler);

    // Captured from real Shelly BLU Button device
    // device_info: 0x44 = v2 (bits 5-7 = 010), trigger=1 (bit 2 = 1)
    uint8_t packet[] = {
        0x02, 0x01, 0x06,                    // Flags
        0x03, 0x03, 0xD2, 0xFC,              // Service UUID
        0x09, 0x16, 0xD2, 0xFC,              // Service Data header
        0x44,                                // device_info: v2, trigger=1
        0x01, 0x64,                          // Battery 100%
        0x3A, 0x01                           // Button single press
    };

    const char *mac = "AA:BB:CC:DD:EE:FF";
    esp_err_t ret = bthome_parse_packet(mac, packet, sizeof(packet), -50);

    TEST_ASSERT_EQUAL(ESP_OK, ret);
    TEST_ASSERT_EQUAL(100, bthome_get_battery_level());
    TEST_ASSERT_TRUE(test_handler_called);
    TEST_ASSERT_EQUAL(0x3A, test_handler_object_id);
    TEST_ASSERT_EQUAL(0x01, test_handler_value);
}

// Test 12: Invalid Packet - NULL MAC
void test_invalid_null_mac(void) {
    uint8_t packet[] = {0x06, 0x16, 0xD2, 0xFC, 0x40, 0x01, 0x64};

    esp_err_t ret = bthome_parse_packet(NULL, packet, sizeof(packet), -50);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

// Test 13: Invalid Packet - NULL Data
void test_invalid_null_data(void) {
    const char *mac = "AA:BB:CC:DD:EE:FF";

    esp_err_t ret = bthome_parse_packet(mac, NULL, 10, -50);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

// Test 14: Invalid Packet - Zero Length
void test_invalid_zero_length(void) {
    uint8_t packet[] = {0x06, 0x16, 0xD2, 0xFC, 0x40, 0x01, 0x64};
    const char *mac = "AA:BB:CC:DD:EE:FF";

    esp_err_t ret = bthome_parse_packet(mac, packet, 0, -50);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
}

// Main test runner
void app_main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_bthome_uuid_detection_valid);
    RUN_TEST(test_bthome_uuid_detection_invalid);
    RUN_TEST(test_device_info_unencrypted);
    RUN_TEST(test_device_info_encrypted);
    RUN_TEST(test_device_info_wrong_version);
    RUN_TEST(test_handler_registration);
    RUN_TEST(test_handler_registration_null);
    RUN_TEST(test_handler_callback);
    RUN_TEST(test_battery_extraction);
    RUN_TEST(test_multiple_olv_triplets);
    RUN_TEST(test_real_shelly_button_packet);
    RUN_TEST(test_invalid_null_mac);
    RUN_TEST(test_invalid_null_data);
    RUN_TEST(test_invalid_zero_length);

    UNITY_END();
}
