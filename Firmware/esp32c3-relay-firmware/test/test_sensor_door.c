/*
 * Unit Tests for Door Event Handler
 *
 * Story 2.4: Decode Motion & Door Events + Implement MAC Filtering
 *
 * Tests door/window event decoding (Object ID 0x2D).
 */

#include "unity.h"
#include "sensor_door.h"
#include "sensor_button.h"  // For unified sensor_event functions
#include <string.h>

static const char *TEST_MAC = "BB:CC:DD:EE:FF:AA";

void setUp(void) {
    // Clear event buffer before each test
    sensor_event_clear();
}

void tearDown(void) {
    // Nothing to tear down
}

/**
 * Test: Door open event (0x01) decoded correctly
 */
void test_door_open_event(void) {
    uint8_t value[] = {0x01};  // Door open
    int8_t rssi = -55;

    // Call door handler
    door_event_handler(TEST_MAC, BTHOME_OBJ_DOOR, value, 1, rssi);

    // Verify event was logged
    sensor_event_record_t record;
    bool got_event = sensor_event_get_last(&record);
    TEST_ASSERT_TRUE_MESSAGE(got_event, "Event should be logged");

    // Verify event details
    TEST_ASSERT_EQUAL_STRING(TEST_MAC, record.mac);
    TEST_ASSERT_EQUAL(SENSOR_TYPE_DOOR, record.sensor_type);
    TEST_ASSERT_EQUAL(0x01, record.event_value);
    TEST_ASSERT_EQUAL(rssi, record.rssi);
}

/**
 * Test: Door closed event (0x00) decoded correctly
 */
void test_door_closed_event(void) {
    uint8_t value[] = {0x00};  // Door closed
    int8_t rssi = -58;

    // Call door handler
    door_event_handler(TEST_MAC, BTHOME_OBJ_DOOR, value, 1, rssi);

    // Verify event was logged
    sensor_event_record_t record;
    bool got_event = sensor_event_get_last(&record);
    TEST_ASSERT_TRUE_MESSAGE(got_event, "Event should be logged");

    // Verify event details
    TEST_ASSERT_EQUAL_STRING(TEST_MAC, record.mac);
    TEST_ASSERT_EQUAL(SENSOR_TYPE_DOOR, record.sensor_type);
    TEST_ASSERT_EQUAL(0x00, record.event_value);
    TEST_ASSERT_EQUAL(rssi, record.rssi);
}

/**
 * Test: Invalid length rejected
 */
void test_door_invalid_length(void) {
    uint8_t value[] = {0x01, 0x02, 0x03};  // 3 bytes (invalid)

    // Call door handler
    door_event_handler(TEST_MAC, BTHOME_OBJ_DOOR, value, 3, -55);

    // Verify NO event was logged (invalid length should be rejected)
    int count = sensor_event_get_count();
    TEST_ASSERT_EQUAL_MESSAGE(0, count, "Invalid length event should not be logged");
}

/**
 * Test: Door events logged to unified event buffer with other sensor types
 */
void test_door_uses_unified_buffer(void) {
    // Log a motion event first
    uint8_t motion_value[] = {0x01};  // Motion detected
    motion_event_handler(TEST_MAC, BTHOME_OBJ_MOTION, motion_value, 1, -60);

    // Then log a door event
    uint8_t door_value[] = {0x01};  // Door open
    door_event_handler(TEST_MAC, BTHOME_OBJ_DOOR, door_value, 1, -55);

    // Verify both events are in the buffer (unified)
    int count = sensor_event_get_count();
    TEST_ASSERT_EQUAL_MESSAGE(2, count, "Both motion and door events should be in unified buffer");

    // Get last event (most recent = door)
    sensor_event_record_t record;
    sensor_event_get_last(&record);
    TEST_ASSERT_EQUAL(SENSOR_TYPE_DOOR, record.sensor_type);
}

// Test runner
void app_main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_door_open_event);
    RUN_TEST(test_door_closed_event);
    RUN_TEST(test_door_invalid_length);
    RUN_TEST(test_door_uses_unified_buffer);
    UNITY_END();
}
