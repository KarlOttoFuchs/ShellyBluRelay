/*
 * System State Machine - Public API
 *
 * Story 3.2: Implement 30-Second Learning Mode
 *
 * Defines the system state machine for the ESP32-C3 Relay Module.
 * States per architecture.md Decision 2.
 */

#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <stdint.h>

/**
 * System state enumeration (per architecture.md Decision 2)
 */
typedef enum {
    STATE_UNCONFIGURED = 0,   // No sensor registered, waiting for configuration
    STATE_LEARNING,           // 30-second learning mode active
    STATE_LISTENING,          // Sensor registered, monitoring BLE packets
    STATE_ACTIVE,             // Relay energized, timer running
} system_state_t;

/**
 * Get current system state
 *
 * @return Current system state
 */
system_state_t state_get_current(void);

/**
 * Set system state with logging
 *
 * Logs state transitions via ESP_LOGI.
 *
 * @param new_state State to transition to
 */
void state_set(system_state_t new_state);

/**
 * Get string representation of state
 *
 * @param state System state
 * @return String name of state (e.g., "UNCONFIGURED", "LEARNING")
 */
const char* state_to_string(system_state_t state);

#endif // STATE_MACHINE_H
