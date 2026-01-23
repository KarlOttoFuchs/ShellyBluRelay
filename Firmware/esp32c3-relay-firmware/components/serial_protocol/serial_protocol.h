/*
 * Serial Protocol Component - Public API Header
 *
 * Story 1.5: Serial Protocol Foundation & Hardware Test Suite
 *
 * This component implements the serial command protocol for hardware validation
 * and future configuration commands. It provides a text-based command interface
 * over UART (115200 baud, 8N1) for testing relay, LEDs, and button.
 *
 * Protocol Format (per architecture.md):
 *   Command:  COMMAND [ARG1] [ARG2]\n
 *   Success:  OK|data\n
 *   Error:    ERROR|code|message\n
 *
 * Usage:
 *   serial_protocol_init();  // Initialize UART and start command task
 *
 * Available Commands:
 *   PING                           - Test connectivity (responds: pong)
 *   TEST_RELAY [ON|OFF]            - Control relay
 *   TEST_LED [STATUS|ERROR] [ON|OFF|BLINK] - Control LEDs
 *   TEST_BUTTON                    - Read button state
 *   HELP                           - Show available commands
 */

#ifndef SERIAL_PROTOCOL_H
#define SERIAL_PROTOCOL_H

#include "esp_err.h"

/**
 * Initialize the serial protocol component
 *
 * Configures UART0 for command reception (115200 baud, 8N1) and creates
 * the serial protocol task for processing incoming commands.
 *
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t serial_protocol_init(void);

/**
 * Send an OK response
 *
 * Sends "OK|data\n" to the serial output.
 *
 * @param data The data string to include in the response
 */
void serial_send_ok(const char *data);

/**
 * Send an error response
 *
 * Sends "ERROR|code|message\n" to the serial output.
 *
 * @param code The error code (e.g., "invalid_command", "invalid_argument")
 * @param message Human-readable error message
 */
void serial_send_error(const char *code, const char *message);

/**
 * Send a raw string to serial output
 *
 * Sends the string directly without formatting.
 * Used for multi-line responses like HELP.
 *
 * @param str The string to send (should include newline if needed)
 */
void serial_send_raw(const char *str);

#endif // SERIAL_PROTOCOL_H
