/*
 * System State Machine - Implementation
 *
 * Story 3.2: Implement 30-Second Learning Mode
 */

#include "state_machine.h"
#include "esp_log.h"

static const char *TAG = "STATE";

// Global system state - starts in UNCONFIGURED
static system_state_t current_state = STATE_UNCONFIGURED;

/**
 * State name lookup table
 */
static const char* state_names[] = {
    "UNCONFIGURED",   // STATE_UNCONFIGURED = 0
    "LEARNING",       // STATE_LEARNING = 1
    "LISTENING",      // STATE_LISTENING = 2
    "ACTIVE",         // STATE_ACTIVE = 3
};

system_state_t state_get_current(void)
{
    return current_state;
}

void state_set(system_state_t new_state)
{
    if (new_state > STATE_ACTIVE) {
        ESP_LOGE(TAG, "Invalid state: %d", new_state);
        return;
    }

    if (current_state != new_state) {
        ESP_LOGI(TAG, "State transition: %s -> %s",
                 state_names[current_state], state_names[new_state]);
        current_state = new_state;
    }
}

const char* state_to_string(system_state_t state)
{
    if (state > STATE_ACTIVE) {
        return "UNKNOWN";
    }
    return state_names[state];
}
