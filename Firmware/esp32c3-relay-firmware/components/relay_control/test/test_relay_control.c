/*
 * Unit Tests for Relay Control Component
 *
 * Story 1.2: Implement Relay Control (GPIO7)
 *
 * These tests validate the relay_control component API.
 * Run with: idf.py test
 */

#include "unity.h"
#include "relay_control.h"

/**
 * Test: relay_init() returns ESP_OK
 */
void test_relay_init_returns_ok(void)
{
    esp_err_t result = relay_init();
    TEST_ASSERT_EQUAL(ESP_OK, result);
}

/**
 * Test: relay state defaults to OFF after init
 */
void test_relay_defaults_to_off(void)
{
    relay_init();
    bool state = relay_get_state();
    TEST_ASSERT_FALSE(state);
}

/**
 * Test: relay_set_state(true) sets state to ON
 */
void test_relay_set_state_on(void)
{
    relay_init();
    esp_err_t result = relay_set_state(true);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_TRUE(relay_get_state());
}

/**
 * Test: relay_set_state(false) sets state to OFF
 */
void test_relay_set_state_off(void)
{
    relay_init();
    relay_set_state(true);  // First turn ON
    esp_err_t result = relay_set_state(false);
    TEST_ASSERT_EQUAL(ESP_OK, result);
    TEST_ASSERT_FALSE(relay_get_state());
}

/**
 * Test: relay_get_state() returns correct state after set
 */
void test_relay_get_state_accuracy(void)
{
    relay_init();

    // Test OFF state
    relay_set_state(false);
    TEST_ASSERT_FALSE(relay_get_state());

    // Test ON state
    relay_set_state(true);
    TEST_ASSERT_TRUE(relay_get_state());

    // Test back to OFF
    relay_set_state(false);
    TEST_ASSERT_FALSE(relay_get_state());
}

/**
 * Test: relay_force_off() sets state to OFF regardless of current state
 */
void test_relay_force_off_from_on_state(void)
{
    relay_init();
    relay_set_state(true);  // Set relay ON
    TEST_ASSERT_TRUE(relay_get_state());

    relay_force_off();  // Force OFF
    TEST_ASSERT_FALSE(relay_get_state());
}

/**
 * Test: relay_force_off() works even when already OFF
 */
void test_relay_force_off_from_off_state(void)
{
    relay_init();
    relay_set_state(false);  // Ensure OFF
    TEST_ASSERT_FALSE(relay_get_state());

    relay_force_off();  // Force OFF (should still work)
    TEST_ASSERT_FALSE(relay_get_state());
}

/**
 * Test: Multiple ON/OFF cycles work correctly
 */
void test_relay_multiple_cycles(void)
{
    relay_init();

    for (int i = 0; i < 5; i++) {
        relay_set_state(true);
        TEST_ASSERT_TRUE(relay_get_state());

        relay_set_state(false);
        TEST_ASSERT_FALSE(relay_get_state());
    }
}

void app_main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_relay_init_returns_ok);
    RUN_TEST(test_relay_defaults_to_off);
    RUN_TEST(test_relay_set_state_on);
    RUN_TEST(test_relay_set_state_off);
    RUN_TEST(test_relay_get_state_accuracy);
    RUN_TEST(test_relay_force_off_from_on_state);
    RUN_TEST(test_relay_force_off_from_off_state);
    RUN_TEST(test_relay_multiple_cycles);

    UNITY_END();
}
