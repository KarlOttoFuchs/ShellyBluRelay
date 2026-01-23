/*
 * Unit Tests for LED Control Component
 *
 * Story 1.4: Implement LED Control (Status & Error LEDs)
 *
 * These tests validate the led_control component API.
 * Run with: idf.py test
 */

#include "unity.h"
#include "led_control.h"

/**
 * Test: led_init() returns ESP_OK
 */
void test_led_init_returns_ok(void)
{
    esp_err_t result = led_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
}

/**
 * Test: led_init() can be called multiple times (idempotent)
 */
void test_led_init_idempotent(void)
{
    esp_err_t result1 = led_init();
    esp_err_t result2 = led_init();
    TEST_ASSERT_EQUAL(ESP_OK, result1);
    TEST_ASSERT_EQUAL(ESP_OK, result2);
}

/**
 * Test: led_set_pattern() accepts LED_PATTERN_OFF
 */
void test_led_set_pattern_off(void)
{
    led_init();
    esp_err_t result = led_set_pattern(LED_STATUS, LED_PATTERN_OFF);
    TEST_ASSERT_EQUAL(ESP_OK, result);
}

/**
 * Test: led_set_pattern() accepts LED_PATTERN_ON
 */
void test_led_set_pattern_on(void)
{
    led_init();
    esp_err_t result = led_set_pattern(LED_STATUS, LED_PATTERN_ON);
    TEST_ASSERT_EQUAL(ESP_OK, result);
}

/**
 * Test: led_set_pattern() accepts LED_PATTERN_BLINK_FAST
 */
void test_led_set_pattern_blink_fast(void)
{
    led_init();
    esp_err_t result = led_set_pattern(LED_STATUS, LED_PATTERN_BLINK_FAST);
    TEST_ASSERT_EQUAL(ESP_OK, result);
}

/**
 * Test: led_set_pattern() accepts LED_PATTERN_BLINK_SLOW
 */
void test_led_set_pattern_blink_slow(void)
{
    led_init();
    esp_err_t result = led_set_pattern(LED_STATUS, LED_PATTERN_BLINK_SLOW);
    TEST_ASSERT_EQUAL(ESP_OK, result);
}

/**
 * Test: led_set_pattern() accepts error patterns
 */
void test_led_set_pattern_error_patterns(void)
{
    led_init();

    esp_err_t result1 = led_set_pattern(LED_ERROR, LED_PATTERN_ERROR_SINGLE);
    TEST_ASSERT_EQUAL(ESP_OK, result1);

    esp_err_t result2 = led_set_pattern(LED_ERROR, LED_PATTERN_ERROR_DOUBLE);
    TEST_ASSERT_EQUAL(ESP_OK, result2);

    esp_err_t result3 = led_set_pattern(LED_ERROR, LED_PATTERN_ERROR_TRIPLE);
    TEST_ASSERT_EQUAL(ESP_OK, result3);
}

/**
 * Test: led_set_pattern() works for both LEDs
 */
void test_led_set_pattern_both_leds(void)
{
    led_init();

    esp_err_t result_status = led_set_pattern(LED_STATUS, LED_PATTERN_BLINK_SLOW);
    esp_err_t result_error = led_set_pattern(LED_ERROR, LED_PATTERN_ON);

    TEST_ASSERT_EQUAL(ESP_OK, result_status);
    TEST_ASSERT_EQUAL(ESP_OK, result_error);
}

/**
 * Test: led_set_pattern() returns error for invalid LED ID
 */
void test_led_set_pattern_invalid_led(void)
{
    led_init();
    esp_err_t result = led_set_pattern((led_id_t)99, LED_PATTERN_ON);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, result);
}

/**
 * Test: led_set_pattern() returns error for invalid pattern
 */
void test_led_set_pattern_invalid_pattern(void)
{
    led_init();
    esp_err_t result = led_set_pattern(LED_STATUS, (led_pattern_t)99);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, result);
}

/**
 * Test: led_set_state() sets ON correctly
 */
void test_led_set_state_on(void)
{
    led_init();
    esp_err_t result = led_set_state(LED_STATUS, true);
    TEST_ASSERT_EQUAL(ESP_OK, result);

    led_pattern_t pattern;
    led_get_pattern(LED_STATUS, &pattern);
    TEST_ASSERT_EQUAL(LED_PATTERN_ON, pattern);
}

/**
 * Test: led_set_state() sets OFF correctly
 */
void test_led_set_state_off(void)
{
    led_init();
    esp_err_t result = led_set_state(LED_ERROR, false);
    TEST_ASSERT_EQUAL(ESP_OK, result);

    led_pattern_t pattern;
    led_get_pattern(LED_ERROR, &pattern);
    TEST_ASSERT_EQUAL(LED_PATTERN_OFF, pattern);
}

/**
 * Test: led_set_state() returns error for invalid LED ID
 */
void test_led_set_state_invalid_led(void)
{
    led_init();
    esp_err_t result = led_set_state((led_id_t)99, true);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, result);
}

/**
 * Test: led_get_pattern() returns current pattern
 */
void test_led_get_pattern(void)
{
    led_init();

    led_set_pattern(LED_STATUS, LED_PATTERN_BLINK_FAST);

    led_pattern_t pattern;
    esp_err_t result = led_get_pattern(LED_STATUS, &pattern);

    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_EQUAL(LED_PATTERN_BLINK_FAST, pattern);
}

/**
 * Test: led_get_pattern() returns error for invalid LED ID
 */
void test_led_get_pattern_invalid_led(void)
{
    led_init();
    led_pattern_t pattern;
    esp_err_t result = led_get_pattern((led_id_t)99, &pattern);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, result);
}

/**
 * Test: led_get_pattern() returns error for NULL pointer
 */
void test_led_get_pattern_null_pointer(void)
{
    led_init();
    esp_err_t result = led_get_pattern(LED_STATUS, NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, result);
}

/**
 * Test: Multiple pattern changes work correctly
 */
void test_led_pattern_changes(void)
{
    led_init();
    led_pattern_t pattern;

    led_set_pattern(LED_STATUS, LED_PATTERN_ON);
    led_get_pattern(LED_STATUS, &pattern);
    TEST_ASSERT_EQUAL(LED_PATTERN_ON, pattern);

    led_set_pattern(LED_STATUS, LED_PATTERN_BLINK_SLOW);
    led_get_pattern(LED_STATUS, &pattern);
    TEST_ASSERT_EQUAL(LED_PATTERN_BLINK_SLOW, pattern);

    led_set_pattern(LED_STATUS, LED_PATTERN_OFF);
    led_get_pattern(LED_STATUS, &pattern);
    TEST_ASSERT_EQUAL(LED_PATTERN_OFF, pattern);
}

/**
 * Test: Independent LED patterns
 */
void test_led_independent_patterns(void)
{
    led_init();
    led_pattern_t status_pattern, error_pattern;

    led_set_pattern(LED_STATUS, LED_PATTERN_BLINK_SLOW);
    led_set_pattern(LED_ERROR, LED_PATTERN_ERROR_DOUBLE);

    led_get_pattern(LED_STATUS, &status_pattern);
    led_get_pattern(LED_ERROR, &error_pattern);

    TEST_ASSERT_EQUAL(LED_PATTERN_BLINK_SLOW, status_pattern);
    TEST_ASSERT_EQUAL(LED_PATTERN_ERROR_DOUBLE, error_pattern);
}

void app_main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_led_init_returns_ok);
    RUN_TEST(test_led_init_idempotent);
    RUN_TEST(test_led_set_pattern_off);
    RUN_TEST(test_led_set_pattern_on);
    RUN_TEST(test_led_set_pattern_blink_fast);
    RUN_TEST(test_led_set_pattern_blink_slow);
    RUN_TEST(test_led_set_pattern_error_patterns);
    RUN_TEST(test_led_set_pattern_both_leds);
    RUN_TEST(test_led_set_pattern_invalid_led);
    RUN_TEST(test_led_set_pattern_invalid_pattern);
    RUN_TEST(test_led_set_state_on);
    RUN_TEST(test_led_set_state_off);
    RUN_TEST(test_led_set_state_invalid_led);
    RUN_TEST(test_led_get_pattern);
    RUN_TEST(test_led_get_pattern_invalid_led);
    RUN_TEST(test_led_get_pattern_null_pointer);
    RUN_TEST(test_led_pattern_changes);
    RUN_TEST(test_led_independent_patterns);

    UNITY_END();
}
