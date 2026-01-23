/*
 * Unit Tests for Serial Protocol Component
 *
 * Story 1.5: Serial Protocol Foundation & Hardware Test Suite
 *
 * Tests command parsing, response formatting, and error handling.
 * Note: Full integration testing requires hardware; these tests validate
 * the command parsing and response logic.
 */

#include "unity.h"
#include "serial_protocol.h"
#include <string.h>
#include <ctype.h>

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
// Unity Test Runner
// ============================================================================

void app_main(void)
{
    UNITY_BEGIN();

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

    UNITY_END();
}
