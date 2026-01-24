/*
 * ESP32-C3 Relay Module - Unit Test Runner
 *
 * This application runs all component unit tests on the target hardware.
 *
 * To run tests:
 *   cd test_app
 *   idf.py build flash monitor
 */

#include "unity.h"
#include "esp_log.h"
#include "test_led_control.h"
#include "test_serial_protocol.h"
// Add other test headers as you create them:
// #include "test_button_input.h"
// #include "test_relay_control.h"
// etc.

static const char *TAG = "TEST_RUNNER";

void app_main(void)
{
    ESP_LOGI(TAG, "====================================");
    ESP_LOGI(TAG, "ESP32-C3 Relay Module - Unit Tests");
    ESP_LOGI(TAG, "====================================\n");

    UNITY_BEGIN();

    // LED Control Tests
    ESP_LOGI(TAG, "Running LED Control Tests...");
    run_led_control_tests();

    // Serial Protocol Tests (Story 1.5, 3.3)
    ESP_LOGI(TAG, "Running Serial Protocol Tests...");
    run_serial_protocol_tests();

    // Add other component test runners here as needed
    // ESP_LOGI(TAG, "Running Button Input Tests...");
    // run_button_input_tests();

    // ESP_LOGI(TAG, "Running Relay Control Tests...");
    // run_relay_control_tests();

    // ESP_LOGI(TAG, "Running BLE Scanner Tests...");
    // run_ble_scanner_tests();

    UNITY_END();

    ESP_LOGI(TAG, "\n====================================");
    ESP_LOGI(TAG, "All tests completed");
    ESP_LOGI(TAG, "====================================");
}
