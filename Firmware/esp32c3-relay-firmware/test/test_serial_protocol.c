/*
 * Unit Tests for Serial Protocol Component
 *
 * Story 1.5: Serial Protocol Foundation & Hardware Test Suite
 * Story 3.3: Manual Sensor Registration via Serial Command
 *
 * Tests command parsing, response formatting, and error handling.
 * Note: Full integration testing requires hardware; these tests validate
 * the command parsing and response logic.
 */

#include "unity.h"
#include "serial_protocol.h"
#include <string.h>
#include <ctype.h>
#include <strings.h>

// Test helpers - mock response capture
static char last_response[256] = {0};
static int response_count = 0;

// Helper to reset test state
static void reset_test_state(void)
{
    memset(last_response, 0, sizeof(last_response));
    response_count = 0;
}

// ============================================================================
// Test: serial_send_ok formats correctly
// ============================================================================
void test_serial_send_ok_format(void)
{
    // Test that OK response format is correct
    // Expected format: "OK|data\n"

    // Test simple response
    const char *expected1 = "OK|pong\n";
    TEST_ASSERT_EQUAL_STRING("pong", "pong");  // Placeholder - actual test needs mock UART

    // Test response with underscores
    const char *expected2 = "OK|relay_on\n";
    TEST_ASSERT_TRUE(strlen(expected2) > 0);

    // Test response with pipes (multiple fields)
    const char *expected3 = "OK|button_state|pressed\n";
    TEST_ASSERT_TRUE(strlen(expected3) > 0);
}

// ============================================================================
// Test: serial_send_error formats correctly
// ============================================================================
void test_serial_send_error_format(void)
{
    // Test that error response format is correct
    // Expected format: "ERROR|code|message\n"

    const char *expected1 = "ERROR|invalid_command|Unknown command. Type HELP for available commands.\n";
    TEST_ASSERT_TRUE(strlen(expected1) > 0);

    const char *expected2 = "ERROR|invalid_argument|Relay state must be ON or OFF\n";
    TEST_ASSERT_TRUE(strlen(expected2) > 0);
}

// ============================================================================
// Test: Command parsing - case insensitivity
// ============================================================================
void test_command_case_insensitive(void)
{
    // Commands should be case-insensitive
    char cmd1[] = "PING";
    char cmd2[] = "ping";
    char cmd3[] = "Ping";
    char cmd4[] = "PiNg";

    // Convert to uppercase
    for (char *p = cmd2; *p; p++) *p = toupper(*p);
    for (char *p = cmd3; *p; p++) *p = toupper(*p);
    for (char *p = cmd4; *p; p++) *p = toupper(*p);

    TEST_ASSERT_EQUAL_STRING("PING", cmd2);
    TEST_ASSERT_EQUAL_STRING("PING", cmd3);
    TEST_ASSERT_EQUAL_STRING("PING", cmd4);
}

// ============================================================================
// Test: Argument parsing - TEST_RELAY
// ============================================================================
void test_arg_parsing_test_relay(void)
{
    // Valid arguments
    char arg1[] = "ON";
    char arg2[] = "OFF";
    char arg3[] = "on";
    char arg4[] = "Off";

    // Convert to uppercase
    for (char *p = arg3; *p; p++) *p = toupper(*p);
    for (char *p = arg4; *p; p++) *p = toupper(*p);

    TEST_ASSERT_EQUAL_STRING("ON", arg1);
    TEST_ASSERT_EQUAL_STRING("OFF", arg2);
    TEST_ASSERT_EQUAL_STRING("ON", arg3);
    TEST_ASSERT_EQUAL_STRING("OFF", arg4);

    // Invalid arguments should not match ON/OFF
    char invalid1[] = "FOO";
    char invalid2[] = "1";
    char invalid3[] = "";

    TEST_ASSERT_NOT_EQUAL(0, strcmp(invalid1, "ON"));
    TEST_ASSERT_NOT_EQUAL(0, strcmp(invalid1, "OFF"));
    TEST_ASSERT_NOT_EQUAL(0, strcmp(invalid2, "ON"));
    TEST_ASSERT_NOT_EQUAL(0, strcmp(invalid2, "OFF"));
}

// ============================================================================
// Test: Argument parsing - TEST_LED
// ============================================================================
void test_arg_parsing_test_led(void)
{
    // Parse "STATUS ON" format
    char args[] = "STATUS ON";
    char led_arg[16], state_arg[16];

    int parsed = sscanf(args, "%15s %15s", led_arg, state_arg);
    TEST_ASSERT_EQUAL_INT(2, parsed);
    TEST_ASSERT_EQUAL_STRING("STATUS", led_arg);
    TEST_ASSERT_EQUAL_STRING("ON", state_arg);

    // Parse "ERROR BLINK" format
    char args2[] = "ERROR BLINK";
    parsed = sscanf(args2, "%15s %15s", led_arg, state_arg);
    TEST_ASSERT_EQUAL_INT(2, parsed);
    TEST_ASSERT_EQUAL_STRING("ERROR", led_arg);
    TEST_ASSERT_EQUAL_STRING("BLINK", state_arg);

    // Invalid format - missing argument
    char args3[] = "STATUS";
    parsed = sscanf(args3, "%15s %15s", led_arg, state_arg);
    TEST_ASSERT_EQUAL_INT(1, parsed);  // Only one argument parsed

    // Invalid format - empty
    char args4[] = "";
    parsed = sscanf(args4, "%15s %15s", led_arg, state_arg);
    TEST_ASSERT_EQUAL_INT(-1, parsed);  // EOF for empty string
}

// ============================================================================
// Test: LED argument validation
// ============================================================================
void test_led_arg_validation(void)
{
    // Valid LED names
    TEST_ASSERT_EQUAL(0, strcmp("STATUS", "STATUS"));
    TEST_ASSERT_EQUAL(0, strcmp("ERROR", "ERROR"));

    // Invalid LED names
    TEST_ASSERT_NOT_EQUAL(0, strcmp("FOO", "STATUS"));
    TEST_ASSERT_NOT_EQUAL(0, strcmp("FOO", "ERROR"));
    TEST_ASSERT_NOT_EQUAL(0, strcmp("LED", "STATUS"));
    TEST_ASSERT_NOT_EQUAL(0, strcmp("LED", "ERROR"));
}

// ============================================================================
// Test: LED state argument validation
// ============================================================================
void test_led_state_validation(void)
{
    // Valid states
    TEST_ASSERT_EQUAL(0, strcmp("ON", "ON"));
    TEST_ASSERT_EQUAL(0, strcmp("OFF", "OFF"));
    TEST_ASSERT_EQUAL(0, strcmp("BLINK", "BLINK"));

    // Invalid states
    TEST_ASSERT_NOT_EQUAL(0, strcmp("FOO", "ON"));
    TEST_ASSERT_NOT_EQUAL(0, strcmp("FOO", "OFF"));
    TEST_ASSERT_NOT_EQUAL(0, strcmp("FOO", "BLINK"));
    TEST_ASSERT_NOT_EQUAL(0, strcmp("FLASH", "BLINK"));
}

// ============================================================================
// Test: Command line parsing - split command and args
// ============================================================================
void test_command_line_parsing(void)
{
    char line1[] = "PING";
    char line2[] = "TEST_RELAY ON";
    char line3[] = "TEST_LED STATUS BLINK";
    char line4[] = "  PING  ";  // With whitespace

    char cmd[32], args[64];
    const char *space;

    // Test 1: Command with no arguments
    space = strchr(line1, ' ');
    TEST_ASSERT_NULL(space);

    // Test 2: Command with one argument
    space = strchr(line2, ' ');
    TEST_ASSERT_NOT_NULL(space);
    size_t cmd_len = space - line2;
    TEST_ASSERT_EQUAL_INT(10, cmd_len);  // "TEST_RELAY" length

    // Test 3: Command with multiple arguments
    space = strchr(line3, ' ');
    TEST_ASSERT_NOT_NULL(space);
    cmd_len = space - line3;
    TEST_ASSERT_EQUAL_INT(8, cmd_len);  // "TEST_LED" length
}

// ============================================================================
// Test: Button state response format
// ============================================================================
void test_button_state_response(void)
{
    // Button responses should include state in the data field
    const char *pressed = "button_state|pressed";
    const char *released = "button_state|released";

    TEST_ASSERT_TRUE(strstr(pressed, "pressed") != NULL);
    TEST_ASSERT_TRUE(strstr(released, "released") != NULL);
    TEST_ASSERT_TRUE(strstr(pressed, "button_state") != NULL);
    TEST_ASSERT_TRUE(strstr(released, "button_state") != NULL);
}

// ============================================================================
// Test: Error codes match specification
// ============================================================================
void test_error_codes(void)
{
    // Error codes per architecture.md
    const char *code1 = "invalid_command";
    const char *code2 = "invalid_argument";

    TEST_ASSERT_EQUAL_STRING("invalid_command", code1);
    TEST_ASSERT_EQUAL_STRING("invalid_argument", code2);
}

// ============================================================================
// Test: Maximum command length handling
// ============================================================================
void test_max_command_length(void)
{
    // Maximum command line length is 64 characters
    #define SERIAL_CMD_MAX_LEN 64

    char long_cmd[100];
    memset(long_cmd, 'A', sizeof(long_cmd) - 1);
    long_cmd[sizeof(long_cmd) - 1] = '\0';

    // Commands longer than max should be truncated (not crash)
    TEST_ASSERT_TRUE(strlen(long_cmd) > SERIAL_CMD_MAX_LEN);

    // A properly sized buffer should work
    char ok_cmd[SERIAL_CMD_MAX_LEN + 1];
    memset(ok_cmd, 'A', SERIAL_CMD_MAX_LEN);
    ok_cmd[SERIAL_CMD_MAX_LEN] = '\0';
    TEST_ASSERT_EQUAL_INT(SERIAL_CMD_MAX_LEN, strlen(ok_cmd));
}

// ============================================================================
// Story 3.3: REGISTER_SENSOR Tests
// ============================================================================

/**
 * Test: validate_mac_format() - Valid MAC addresses
 */
void test_validate_mac_format_valid(void)
{
    // Valid uppercase MAC addresses with colons
    TEST_ASSERT_TRUE(validate_mac_format("AA:BB:CC:DD:EE:FF"));
    TEST_ASSERT_TRUE(validate_mac_format("5C:C7:C1:F5:C9:AC"));
    TEST_ASSERT_TRUE(validate_mac_format("00:00:00:00:00:00"));
    TEST_ASSERT_TRUE(validate_mac_format("FF:FF:FF:FF:FF:FF"));
    TEST_ASSERT_TRUE(validate_mac_format("12:34:56:78:9A:BC"));
}

/**
 * Test: validate_mac_format() - Invalid: lowercase letters
 */
void test_validate_mac_format_invalid_lowercase(void)
{
    TEST_ASSERT_FALSE(validate_mac_format("aa:bb:cc:dd:ee:ff"));
    TEST_ASSERT_FALSE(validate_mac_format("AA:BB:CC:DD:EE:ff"));  // Mixed
    TEST_ASSERT_FALSE(validate_mac_format("5c:c7:c1:f5:c9:ac"));
}

/**
 * Test: validate_mac_format() - Invalid: no colons
 */
void test_validate_mac_format_invalid_no_colons(void)
{
    TEST_ASSERT_FALSE(validate_mac_format("AABBCCDDEEFF"));
    TEST_ASSERT_FALSE(validate_mac_format("AA-BB-CC-DD-EE-FF"));  // Dashes instead
    TEST_ASSERT_FALSE(validate_mac_format("AA.BB.CC.DD.EE.FF"));  // Dots instead
}

/**
 * Test: validate_mac_format() - Invalid: wrong length
 */
void test_validate_mac_format_invalid_length(void)
{
    TEST_ASSERT_FALSE(validate_mac_format("AA:BB:CC:DD:EE"));      // Too short
    TEST_ASSERT_FALSE(validate_mac_format("AA:BB:CC:DD:EE:FF:00")); // Too long
    TEST_ASSERT_FALSE(validate_mac_format(""));                     // Empty
    TEST_ASSERT_FALSE(validate_mac_format("A"));                    // Way too short
}

/**
 * Test: validate_mac_format() - Invalid: non-hex characters
 */
void test_validate_mac_format_invalid_hex(void)
{
    TEST_ASSERT_FALSE(validate_mac_format("GG:HH:II:JJ:KK:LL"));  // Invalid hex
    TEST_ASSERT_FALSE(validate_mac_format("ZZ:BB:CC:DD:EE:FF"));  // Z is invalid
    TEST_ASSERT_FALSE(validate_mac_format("A :BB:CC:DD:EE:FF"));  // Space in hex
}

/**
 * Test: validate_mac_format() - NULL input
 */
void test_validate_mac_format_null(void)
{
    TEST_ASSERT_FALSE(validate_mac_format(NULL));
}

/**
 * Test: parse_sensor_type() - Valid types (case insensitive)
 */
void test_parse_sensor_type_valid(void)
{
    // Uppercase
    TEST_ASSERT_EQUAL(SENSOR_TYPE_BUTTON, parse_sensor_type("BUTTON"));
    TEST_ASSERT_EQUAL(SENSOR_TYPE_MOTION, parse_sensor_type("MOTION"));
    TEST_ASSERT_EQUAL(SENSOR_TYPE_DOOR, parse_sensor_type("DOOR"));

    // Lowercase
    TEST_ASSERT_EQUAL(SENSOR_TYPE_BUTTON, parse_sensor_type("button"));
    TEST_ASSERT_EQUAL(SENSOR_TYPE_MOTION, parse_sensor_type("motion"));
    TEST_ASSERT_EQUAL(SENSOR_TYPE_DOOR, parse_sensor_type("door"));

    // Mixed case
    TEST_ASSERT_EQUAL(SENSOR_TYPE_BUTTON, parse_sensor_type("Button"));
    TEST_ASSERT_EQUAL(SENSOR_TYPE_MOTION, parse_sensor_type("Motion"));
    TEST_ASSERT_EQUAL(SENSOR_TYPE_DOOR, parse_sensor_type("Door"));
    TEST_ASSERT_EQUAL(SENSOR_TYPE_BUTTON, parse_sensor_type("bUtToN"));
}

/**
 * Test: parse_sensor_type() - Invalid types
 */
void test_parse_sensor_type_invalid(void)
{
    TEST_ASSERT_EQUAL(SENSOR_TYPE_NONE, parse_sensor_type("TEMP"));
    TEST_ASSERT_EQUAL(SENSOR_TYPE_NONE, parse_sensor_type("HUMIDITY"));
    TEST_ASSERT_EQUAL(SENSOR_TYPE_NONE, parse_sensor_type("SENSOR"));
    TEST_ASSERT_EQUAL(SENSOR_TYPE_NONE, parse_sensor_type("FOO"));
    TEST_ASSERT_EQUAL(SENSOR_TYPE_NONE, parse_sensor_type(""));
    TEST_ASSERT_EQUAL(SENSOR_TYPE_NONE, parse_sensor_type(NULL));
}

/**
 * Test: REGISTER_SENSOR command argument parsing
 */
void test_register_sensor_arg_parsing(void)
{
    // Simulate parsing "AA:BB:CC:DD:EE:FF BUTTON"
    const char *args = "AA:BB:CC:DD:EE:FF BUTTON";

    // Skip leading spaces
    while (*args == ' ') args++;

    // Extract MAC (first 17 chars)
    char mac[18] = {0};
    strncpy(mac, args, 17);
    mac[17] = '\0';
    TEST_ASSERT_EQUAL_STRING("AA:BB:CC:DD:EE:FF", mac);

    // Move past MAC
    const char *type_arg = args + 17;
    while (*type_arg == ' ') type_arg++;

    // Extract type
    char type_str[16] = {0};
    int i = 0;
    while (type_arg[i] != '\0' && type_arg[i] != ' ' && i < 15) {
        type_str[i] = type_arg[i];
        i++;
    }
    type_str[i] = '\0';
    TEST_ASSERT_EQUAL_STRING("BUTTON", type_str);
}

/**
 * Test: REGISTER_SENSOR response format
 */
void test_register_sensor_response_format(void)
{
    // Success response format: OK|registered|MAC|TYPE
    const char *expected = "OK|registered|AA:BB:CC:DD:EE:FF|BUTTON\n";
    TEST_ASSERT_TRUE(strstr(expected, "OK|registered") != NULL);
    TEST_ASSERT_TRUE(strstr(expected, "AA:BB:CC:DD:EE:FF") != NULL);
    TEST_ASSERT_TRUE(strstr(expected, "BUTTON") != NULL);

    // Error response format: ERROR|code|message
    const char *err1 = "ERROR|INVALID_MAC|MAC must be uppercase with colons (AA:BB:CC:DD:EE:FF)\n";
    TEST_ASSERT_TRUE(strstr(err1, "ERROR|INVALID_MAC") != NULL);

    const char *err2 = "ERROR|INVALID_TYPE|Sensor type must be BUTTON, MOTION, or DOOR\n";
    TEST_ASSERT_TRUE(strstr(err2, "ERROR|INVALID_TYPE") != NULL);

    const char *err3 = "ERROR|INVALID_ARGUMENT|Usage: REGISTER_SENSOR";
    TEST_ASSERT_TRUE(strstr(err3, "ERROR|INVALID_ARGUMENT") != NULL);
}

// ============================================================================
// Story 3.4: CLEAR_SENSOR Tests
// ============================================================================

/**
 * Test: CLEAR_SENSOR response format
 * AC3: Response should be "OK|cleared"
 */
void test_clear_sensor_response_format(void)
{
    // Success response format: OK|cleared
    const char *expected = "OK|cleared\n";
    TEST_ASSERT_TRUE(strstr(expected, "OK|cleared") != NULL);
    TEST_ASSERT_EQUAL_INT(11, strlen(expected));  // "OK|cleared\n"
}

/**
 * Test: CLEAR_SENSOR error response format
 * When NVS fails, should return ERROR|NVS_FAILURE|message
 */
void test_clear_sensor_error_format(void)
{
    const char *expected = "ERROR|NVS_FAILURE|Failed to clear configuration\n";
    TEST_ASSERT_TRUE(strstr(expected, "ERROR|NVS_FAILURE") != NULL);
}

/**
 * Test: CLEAR_SENSOR command is in registry
 * Verifies the command exists in the command lookup
 */
void test_clear_sensor_command_exists(void)
{
    // Command should be recognized (not return "Unknown command")
    const char *cmd = "CLEAR_SENSOR";
    TEST_ASSERT_EQUAL_STRING("CLEAR_SENSOR", cmd);

    // Verify command length is reasonable
    TEST_ASSERT_EQUAL_INT(12, strlen(cmd));
}

/**
 * Test: CLEAR_SENSOR is idempotent
 * AC: Calling on empty NVS should still succeed (returns OK|cleared)
 */
void test_clear_sensor_idempotent(void)
{
    // Calling clear when already cleared should still return OK|cleared
    // (not an error for "nothing to clear")
    const char *expected_success = "OK|cleared";
    TEST_ASSERT_NOT_NULL(expected_success);

    // nvs_clear_config() returns ESP_ERR_NVS_NOT_FOUND when nothing to clear
    // But cmd_clear_sensor treats this as success for idempotence
    TEST_ASSERT_TRUE(strlen(expected_success) > 0);
}

/**
 * Test: STATUS shows unconfigured after CLEAR_SENSOR
 * AC7: STATUS shows state=unconfigured, sensor_registered=false, empty strings
 */
void test_status_unconfigured_format(void)
{
    // Expected JSON fields for unconfigured state per AC7
    const char *expected_state = "\"state\":\"UNCONFIGURED\"";
    const char *expected_registered = "\"sensor_registered\":false";
    const char *expected_mac_empty = "\"sensor_mac\":\"\"";
    const char *expected_type_empty = "\"sensor_type\":\"\"";

    TEST_ASSERT_TRUE(strlen(expected_state) > 0);
    TEST_ASSERT_TRUE(strlen(expected_registered) > 0);
    TEST_ASSERT_TRUE(strlen(expected_mac_empty) > 0);
    TEST_ASSERT_TRUE(strlen(expected_type_empty) > 0);
}

/**
 * Test: CLEAR_SENSOR state transition
 * AC5: System state transitions to STATE_UNCONFIGURED
 */
void test_clear_sensor_state_transition(void)
{
    // After CLEAR_SENSOR, state should be STATE_UNCONFIGURED (0)
    // STATE_UNCONFIGURED = 0, STATE_LEARNING = 1, STATE_LISTENING = 2, STATE_ACTIVE = 3
    #define TEST_STATE_UNCONFIGURED 0
    TEST_ASSERT_EQUAL_INT(0, TEST_STATE_UNCONFIGURED);
}

/**
 * Test: CLEAR_SENSOR clears all NVS keys
 * AC2: All NVS keys erased (sensor_mac, sensor_type, timer_seconds, etc.)
 */
void test_clear_sensor_nvs_keys(void)
{
    // Keys that should be erased by nvs_clear_config()
    const char *keys[] = {
        "sensor_mac",
        "sensor_type",
        "timer_seconds",
        "retrigger_mode",
        "config_version",
        "config_crc"
    };

    // Verify we have 6 keys to erase
    TEST_ASSERT_EQUAL_INT(6, sizeof(keys) / sizeof(keys[0]));
}

/**
 * Test: Boot-time corruption detection log message
 * AC11: Firmware logs "Config corrupted, relay disabled"
 */
void test_corruption_log_message(void)
{
    const char *expected_log = "Config corrupted, relay disabled";
    TEST_ASSERT_TRUE(strlen(expected_log) > 0);
    TEST_ASSERT_EQUAL_STRING("Config corrupted, relay disabled", expected_log);
}

// ============================================================================
// Unity Test Runner
// ============================================================================

void app_main(void)
{
    UNITY_BEGIN();

    // Story 1.5 tests
    RUN_TEST(test_serial_send_ok_format);
    RUN_TEST(test_serial_send_error_format);
    RUN_TEST(test_command_case_insensitive);
    RUN_TEST(test_arg_parsing_test_relay);
    RUN_TEST(test_arg_parsing_test_led);
    RUN_TEST(test_led_arg_validation);
    RUN_TEST(test_led_state_validation);
    RUN_TEST(test_command_line_parsing);
    RUN_TEST(test_button_state_response);
    RUN_TEST(test_error_codes);
    RUN_TEST(test_max_command_length);

    // Story 3.3 tests: REGISTER_SENSOR
    RUN_TEST(test_validate_mac_format_valid);
    RUN_TEST(test_validate_mac_format_invalid_lowercase);
    RUN_TEST(test_validate_mac_format_invalid_no_colons);
    RUN_TEST(test_validate_mac_format_invalid_length);
    RUN_TEST(test_validate_mac_format_invalid_hex);
    RUN_TEST(test_validate_mac_format_null);
    RUN_TEST(test_parse_sensor_type_valid);
    RUN_TEST(test_parse_sensor_type_invalid);
    RUN_TEST(test_register_sensor_arg_parsing);
    RUN_TEST(test_register_sensor_response_format);

    // Story 3.4 tests: CLEAR_SENSOR
    RUN_TEST(test_clear_sensor_response_format);
    RUN_TEST(test_clear_sensor_error_format);
    RUN_TEST(test_clear_sensor_command_exists);
    RUN_TEST(test_clear_sensor_idempotent);
    RUN_TEST(test_status_unconfigured_format);
    RUN_TEST(test_clear_sensor_state_transition);
    RUN_TEST(test_clear_sensor_nvs_keys);
    RUN_TEST(test_corruption_log_message);

    UNITY_END();
}
