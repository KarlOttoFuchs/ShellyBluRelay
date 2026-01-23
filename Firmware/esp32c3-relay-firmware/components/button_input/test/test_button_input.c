/*
 * Unit Tests for Button Input Component
 *
 * Story 1.3: Implement Button Input Detection (GPIO9)
 *
 * These tests validate the button_input component API.
 * Run with: idf.py test
 */

#include "unity.h"
#include "button_input.h"

/**
 * Test: button_init() returns ESP_OK
 */
void test_button_init_returns_ok(void)
{
    esp_err_t result = button_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
}

/**
 * Test: button_read_state() returns valid state after init
 */
void test_button_read_state_after_init(void)
{
    button_init();
    bool pressed = true;  // Initialize to opposite of expected default
    esp_err_t result = button_read_state(&pressed);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    // Button should be released by default (not pressed)
    TEST_ASSERT_FALSE(pressed);
}

/**
 * Test: button_read_state() returns error with NULL pointer
 */
void test_button_read_state_null_pointer(void)
{
    button_init();
    esp_err_t result = button_read_state(NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, result);
}

/**
 * Test: button_is_pressed() returns boolean
 */
void test_button_is_pressed_returns_false_when_released(void)
{
    button_init();
    bool pressed = button_is_pressed();
    // Button should be released by default
    TEST_ASSERT_FALSE(pressed);
}

/**
 * Test: button_wait_for_press() returns timeout when button not pressed
 */
void test_button_wait_for_press_timeout(void)
{
    button_init();
    // Use short timeout for test (100ms)
    esp_err_t result = button_wait_for_press(100);
    TEST_ASSERT_EQUAL(ESP_ERR_TIMEOUT, result);
}

/**
 * Test: button_wait_for_release() returns OK when button not pressed
 */
void test_button_wait_for_release_immediate(void)
{
    button_init();
    // Button is already released, should return immediately
    esp_err_t result = button_wait_for_release(100);
    TEST_ASSERT_EQUAL(ESP_OK, result);
}

/**
 * Test: Multiple button_read_state() calls work correctly
 */
void test_button_multiple_reads(void)
{
    button_init();

    for (int i = 0; i < 5; i++) {
        bool pressed;
        esp_err_t result = button_read_state(&pressed);
        TEST_ASSERT_EQUAL(ESP_OK, result);
        // Without physical button press, should always be released
        TEST_ASSERT_FALSE(pressed);
    }
}

/**
 * Test: button_is_pressed() multiple calls are consistent
 */
void test_button_is_pressed_consistency(void)
{
    button_init();

    // Call multiple times - should be consistent
    bool first = button_is_pressed();
    bool second = button_is_pressed();
    bool third = button_is_pressed();

    TEST_ASSERT_EQUAL(first, second);
    TEST_ASSERT_EQUAL(second, third);
}

void app_main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_button_init_returns_ok);
    RUN_TEST(test_button_read_state_after_init);
    RUN_TEST(test_button_read_state_null_pointer);
    RUN_TEST(test_button_is_pressed_returns_false_when_released);
    RUN_TEST(test_button_wait_for_press_timeout);
    RUN_TEST(test_button_wait_for_release_immediate);
    RUN_TEST(test_button_multiple_reads);
    RUN_TEST(test_button_is_pressed_consistency);

    UNITY_END();
}
