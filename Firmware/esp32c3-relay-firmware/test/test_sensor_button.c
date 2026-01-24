/*
 * Button Event Handler - Unit Tests
 *
 * Story 2.3: Test button event decoding, event buffer, and real packet handling
 */

#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "sensor_button.h"

void setUp(void) {
    // Reset event buffer before each test
    button_event_clear();
}

void tearDown(void) {
    // Cleanup after each test
}

// Test 1: Single press decoding
void test_button_single_press(void) {
    uint8_t value[] = {0x01};
    char mac[] = "5C:C7:C1:F5:C9:AC";

    button_event_handler(mac, 0x3A, value, 1, -66);

    // Verify event logged to buffer
    button_event_record_t record;
    TEST_ASSERT_TRUE(button_event_get_last(&record));
    TEST_ASSERT_EQUAL(BUTTON_EVENT_SINGLE, record.event_type);
    TEST_ASSERT_EQUAL_STRING(mac, record.mac);
    TEST_ASSERT_EQUAL(-66, record.rssi);
}

// Test 2: All button event types
void test_button_all_event_types(void) {
    char mac[] = "AA:BB:CC:DD:EE:FF";

    uint8_t single[] = {0x01};
    button_event_handler(mac, 0x3A, single, 1, -50);

    uint8_t double_press[] = {0x02};
    button_event_handler(mac, 0x3A, double_press, 1, -51);

    uint8_t triple[] = {0x03};
    button_event_handler(mac, 0x3A, triple, 1, -52);

    uint8_t long_press[] = {0x04};
    button_event_handler(mac, 0x3A, long_press, 1, -53);

    // Verify buffer has 4 events
    TEST_ASSERT_EQUAL(4, button_event_get_count());
}

// Test 3: Unknown button event
void test_button_unknown_event(void) {
    uint8_t value[] = {0xFF};  // Unknown value
    char mac[] = "AA:BB:CC:DD:EE:FF";

    button_event_handler(mac, 0x3A, value, 1, -50);

    // Should still be logged (as unknown)
    button_event_record_t record;
    TEST_ASSERT_TRUE(button_event_get_last(&record));
    TEST_ASSERT_EQUAL(0xFF, record.event_type);
}

// Test 4: Invalid value length
void test_button_invalid_length(void) {
    uint8_t value[] = {0x01, 0x02};  // 2 bytes (should be 1)
    char mac[] = "AA:BB:CC:DD:EE:FF";

    button_event_handler(mac, 0x3A, value, 2, -50);

    // Should not be logged (invalid packet)
    TEST_ASSERT_EQUAL(0, button_event_get_count());
}

// Test 5: Circular buffer overflow
void test_button_event_buffer_circular(void) {
    char mac[] = "AA:BB:CC:DD:EE:FF";
    uint8_t value[] = {0x01};

    // Add 15 events (buffer holds 10)
    for (int i = 0; i < 15; i++) {
        button_event_handler(mac, 0x3A, value, 1, -50);
    }

    // Should only have 10 events
    TEST_ASSERT_EQUAL(10, button_event_get_count());
}

// Test 6: Real Shelly BLU Button packet (from hardware testing)
void test_real_shelly_button_packet(void) {
    // Captured from real Shelly BLU Button (Karl's hardware testing)
    char mac[] = "5C:C7:C1:F5:C9:AC";
    uint8_t button_value[] = {0x01};  // Single press
    int8_t rssi = -66;

    button_event_handler(mac, 0x3A, button_value, 1, rssi);

    button_event_record_t record;
    TEST_ASSERT_TRUE(button_event_get_last(&record));
    TEST_ASSERT_EQUAL(BUTTON_EVENT_SINGLE, record.event_type);
    TEST_ASSERT_EQUAL_STRING("5C:C7:C1:F5:C9:AC", record.mac);
    TEST_ASSERT_EQUAL(-66, record.rssi);
}

// Test 7: Event buffer retrieval order
void test_button_event_get_history(void) {
    char mac[] = "AA:BB:CC:DD:EE:FF";

    // Add 3 events
    uint8_t single[] = {0x01};
    button_event_handler(mac, 0x3A, single, 1, -50);

    uint8_t double_press[] = {0x02};
    button_event_handler(mac, 0x3A, double_press, 1, -51);

    uint8_t triple[] = {0x03};
    button_event_handler(mac, 0x3A, triple, 1, -52);

    // Get history (should be in reverse chronological order)
    button_event_record_t records[10];
    int count = button_event_get_history(records, 10);

    TEST_ASSERT_EQUAL(3, count);
    TEST_ASSERT_EQUAL(BUTTON_EVENT_TRIPLE, records[0].event_type);  // Most recent
    TEST_ASSERT_EQUAL(BUTTON_EVENT_DOUBLE, records[1].event_type);
    TEST_ASSERT_EQUAL(BUTTON_EVENT_SINGLE, records[2].event_type);  // Oldest
}

// Unity test runner
void app_main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_button_single_press);
    RUN_TEST(test_button_all_event_types);
    RUN_TEST(test_button_unknown_event);
    RUN_TEST(test_button_invalid_length);
    RUN_TEST(test_button_event_buffer_circular);
    RUN_TEST(test_real_shelly_button_packet);
    RUN_TEST(test_button_event_get_history);

    UNITY_END();
}
