/*
 * Unit Tests for Motion Event Handler
 *
 * Story 2.4: Decode Motion & Door Events + Implement MAC Filtering
 *
 * Tests motion event decoding (Object ID 0x21).
 */

#include "unity.h"
#include "sensor_motion.h"
#include "sensor_button.h"  // For unified sensor_event functions
#include <string.h>

static const char *TEST_MAC = "AA:BB:CC:DD:EE:FF";

void setUp(void) {
    // Clear event buffer before each test
    sensor_event_clear();
}

void tearDown(void) {
    // Nothing to tear down
}

/**
 * Test: Motion detected event (0x01) decoded correctly
 */
void test_motion_detected_event(void) {
    uint8_t value[] = {0x01};  // Motion detected
    int8_t rssi = -60;

    // Call motion handler
    motion_event_handler(TEST_MAC, BTHOME_OBJ_MOTION, value, 1, rssi);

    // Verify event was logged
    sensor_event_record_t record;
    bool got_event = sensor_event_get_last(&record);
    TEST_ASSERT_TRUE_MESSAGE(got_event, "Event should be logged");

    // Verify event details
    TEST_ASSERT_EQUAL_STRING(TEST_MAC, record.mac);
    TEST_ASSERT_EQUAL(SENSOR_TYPE_MOTION, record.sensor_type);
    TEST_ASSERT_EQUAL(0x01, record.event_value);
    TEST_ASSERT_EQUAL(rssi, record.rssi);
}

/**
 * Test: Motion timeout event (0x00) decoded correctly
 */
void test_motion_timeout_event(void) {
    uint8_t value[] = {0x00};  // Motion timeout
    int8_t rssi = -65;

    // Call motion handler
    motion_event_handler(TEST_MAC, BTHOME_OBJ_MOTION, value, 1, rssi);

    // Verify event was logged
    sensor_event_record_t record;
    bool got_event = sensor_event_get_last(&record);
    TEST_ASSERT_TRUE_MESSAGE(got_event, "Event should be logged");

    // Verify event details
    TEST_ASSERT_EQUAL_STRING(TEST_MAC, record.mac);
    TEST_ASSERT_EQUAL(SENSOR_TYPE_MOTION, record.sensor_type);
    TEST_ASSERT_EQUAL(0x00, record.event_value);
    TEST_ASSERT_EQUAL(rssi, record.rssi);
}

/**
 * Test: Invalid length rejected
 */
void test_motion_invalid_length(void) {
    uint8_t value[] = {0x01, 0x02};  // 2 bytes (invalid)

    // Call motion handler
    motion_event_handler(TEST_MAC, BTHOME_OBJ_MOTION, value, 2, -60);

    // Verify NO event was logged (invalid length should be rejected)
    int count = sensor_event_get_count();
    TEST_ASSERT_EQUAL_MESSAGE(0, count, "Invalid length event should not be logged");
}

/**
 * Test: Motion events logged to unified event buffer
 */
void test_motion_uses_unified_buffer(void) {
    // Log a button event first
    uint8_t button_value[] = {0x01};  // Single press
    button_event_handler(TEST_MAC, BTHOME_OBJ_BUTTON, button_value, 1, -50);

    // Then log a motion event
    uint8_t motion_value[] = {0x01};  // Motion detected
    motion_event_handler(TEST_MAC, BTHOME_OBJ_MOTION, motion_value, 1, -60);

    // Verify both events are in the buffer (unified)
    int count = sensor_event_get_count();
    TEST_ASSERT_EQUAL_MESSAGE(2, count, "Both button and motion events should be in unified buffer");

    // Get last event (most recent = motion)
    sensor_event_record_t record;
    sensor_event_get_last(&record);
    TEST_ASSERT_EQUAL(SENSOR_TYPE_MOTION, record.sensor_type);
}

// Test runner
void app_main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_motion_detected_event);
    RUN_TEST(test_motion_timeout_event);
    RUN_TEST(test_motion_invalid_length);
    RUN_TEST(test_motion_uses_unified_buffer);
    UNITY_END();
}
