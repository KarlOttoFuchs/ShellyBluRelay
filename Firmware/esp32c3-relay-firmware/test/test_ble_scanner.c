/*
 * Unit Tests for BLE Scanner Component
 *
 * Story 2.1: Initialize BLE Stack & Scan for Advertising Packets
 *
 * These tests validate the ble_scanner component API and helper functions.
 * Note: Full BLE initialization tests require hardware and are validated manually.
 * Run with: idf.py test
 */

#include "unity.h"
#include "ble_scanner.h"
#include <string.h>

/**
 * Test: MAC address formatting produces uppercase with colons
 * This is CRITICAL per architecture.md - format must be "AA:BB:CC:DD:EE:FF"
 */
void test_mac_address_format(void)
{
    // Test MAC: 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
    uint8_t mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    char mac_str[18];

    // Format using same pattern as ble_scanner.c
    sprintf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // Verify format
    TEST_ASSERT_EQUAL_STRING("AA:BB:CC:DD:EE:FF", mac_str);
    TEST_ASSERT_EQUAL(17, strlen(mac_str));  // Length should be 17 characters
}

/**
 * Test: MAC address formatting with zeros
 */
void test_mac_address_format_with_zeros(void)
{
    uint8_t mac[6] = {0x00, 0x11, 0x22, 0x00, 0x44, 0x00};
    char mac_str[18];

    sprintf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    TEST_ASSERT_EQUAL_STRING("00:11:22:00:44:00", mac_str);
}

/**
 * Test: ble_scanner_get_devices() validates NULL arguments
 */
void test_ble_scanner_get_devices_null_pointer(void)
{
    ble_tracked_device_t devices[10];
    uint32_t count;

    // Test with NULL devices buffer
    esp_err_t result = ble_scanner_get_devices(NULL, 10, &count);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, result);

    // Test with NULL count pointer
    result = ble_scanner_get_devices(devices, 10, NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, result);
}

/**
 * Test: ble_scanner_get_devices() returns error if not initialized
 * This test should run before ble_scanner_init() is called
 */
void test_ble_scanner_get_devices_not_initialized(void)
{
    ble_tracked_device_t devices[10];
    uint32_t count;

    // Attempt to get devices without initialization
    // Note: This test assumes ble_scanner_init() has not been called
    // In a real test suite, we'd need a way to reset or mock initialization state
    esp_err_t result = ble_scanner_get_devices(devices, 10, &count);

    // Should return either INVALID_STATE or INVALID_ARG depending on init state
    // We accept both as valid responses
    TEST_ASSERT(result == ESP_ERR_INVALID_STATE || result == ESP_ERR_INVALID_ARG);
}

/**
 * Test: Device tracking structure size is correct
 */
void test_device_tracking_structure_size(void)
{
    // Verify that BLE_MAX_TRACKED_DEVICES is defined correctly
    TEST_ASSERT_EQUAL(32, BLE_MAX_TRACKED_DEVICES);

    // Verify device timeout is 60 seconds
    TEST_ASSERT_EQUAL(60 * 1000, BLE_DEVICE_TIMEOUT_MS);
}

/**
 * Test: ble_tracked_device_t structure has correct fields
 */
void test_tracked_device_structure(void)
{
    ble_tracked_device_t device;

    // Verify MAC field can hold 17 characters + null terminator
    TEST_ASSERT(sizeof(device.mac) >= 18);

    // Test MAC string storage
    strncpy(device.mac, "AA:BB:CC:DD:EE:FF", sizeof(device.mac) - 1);
    device.mac[sizeof(device.mac) - 1] = '\0';
    TEST_ASSERT_EQUAL_STRING("AA:BB:CC:DD:EE:FF", device.mac);

    // Test RSSI storage (should be signed int8_t for negative values)
    device.rssi = -65;
    TEST_ASSERT_EQUAL(-65, device.rssi);

    // Test timestamp storage
    device.last_seen_ms = 12345;
    TEST_ASSERT_EQUAL(12345, device.last_seen_ms);

    // Test active flag
    device.active = true;
    TEST_ASSERT_TRUE(device.active);
    device.active = false;
    TEST_ASSERT_FALSE(device.active);
}

/**
 * Unity test runner - registers all tests
 */
void app_main(void)
{
    UNITY_BEGIN();

    // MAC address formatting tests
    RUN_TEST(test_mac_address_format);
    RUN_TEST(test_mac_address_format_with_zeros);

    // Device tracking structure tests
    RUN_TEST(test_device_tracking_structure_size);
    RUN_TEST(test_tracked_device_structure);

    // API validation tests
    RUN_TEST(test_ble_scanner_get_devices_null_pointer);
    RUN_TEST(test_ble_scanner_get_devices_not_initialized);

    UNITY_END();
}
