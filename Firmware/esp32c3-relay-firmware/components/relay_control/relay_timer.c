/*
 * Relay Timer Component - Implementation
 *
 * Story 4A.2: Automatic Timer De-energize with Configurable Duration
 *
 * Uses esp_timer for accurate countdown. When timer expires:
 * - De-energizes relay (GPIO7 LOW)
 * - Transitions state to LISTENING
 * - Sets LED to slow blink pattern
 */

#include "relay_timer.h"
#include "relay_control.h"
#include "state_machine.h"
#include "led_control.h"
#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "RELAY_TIMER";

// Timer handle
static esp_timer_handle_t relay_timer = NULL;

// Timer tracking variables
static uint32_t timer_start_ms = 0;
static uint16_t timer_duration_sec = 0;

// Timer limits (per FR19)
#define TIMER_MIN_SECONDS  1
#define TIMER_MAX_SECONDS  600

/**
 * Timer expiry callback
 *
 * Called by esp_timer when countdown reaches 0.
 * Executes in timer task context.
 */
static void relay_timer_callback(void *arg)
{
    (void)arg;  // Unused

    ESP_LOGI(TAG, "Timer expired");

    // De-energize relay (AC4)
    relay_set_state(false);
    ESP_LOGI(TAG, "Relay deactivated");

    // Transition to LISTENING state (AC5)
    state_set(STATE_LISTENING);

    // Set LED to slow blink pattern (AC5)
    led_set_pattern(LED_STATUS, LED_PATTERN_BLINK_SLOW);

    // Mark timer as inactive
    if (relay_timer != NULL) {
        esp_timer_delete(relay_timer);
        relay_timer = NULL;
    }
    timer_duration_sec = 0;
}

esp_err_t relay_timer_start(uint16_t seconds)
{
    // Validate duration (FR19: 1-600 seconds)
    if (seconds < TIMER_MIN_SECONDS || seconds > TIMER_MAX_SECONDS) {
        ESP_LOGE(TAG, "Invalid timer duration: %u (valid: %d-%d)",
                 seconds, TIMER_MIN_SECONDS, TIMER_MAX_SECONDS);
        return ESP_ERR_INVALID_ARG;
    }

    // Stop existing timer if running
    if (relay_timer != NULL) {
        esp_timer_stop(relay_timer);
        esp_timer_delete(relay_timer);
        relay_timer = NULL;
    }

    // Store timer parameters
    timer_duration_sec = seconds;
    timer_start_ms = (uint32_t)(esp_timer_get_time() / 1000);

    // Create timer
    esp_timer_create_args_t timer_args = {
        .callback = relay_timer_callback,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "relay_timer"
    };

    esp_err_t ret = esp_timer_create(&timer_args, &relay_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create timer: %s", esp_err_to_name(ret));
        timer_duration_sec = 0;
        return ESP_FAIL;
    }

    // Start timer (convert seconds to microseconds)
    ret = esp_timer_start_once(relay_timer, (uint64_t)seconds * 1000000ULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start timer: %s", esp_err_to_name(ret));
        esp_timer_delete(relay_timer);
        relay_timer = NULL;
        timer_duration_sec = 0;
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Timer started: %u seconds", seconds);
    return ESP_OK;
}

esp_err_t relay_timer_stop(void)
{
    if (relay_timer != NULL) {
        esp_timer_stop(relay_timer);
        esp_timer_delete(relay_timer);
        relay_timer = NULL;
        ESP_LOGI(TAG, "Timer stopped");
    }

    timer_duration_sec = 0;
    timer_start_ms = 0;

    return ESP_OK;
}

uint16_t relay_timer_get_remaining_sec(void)
{
    if (relay_timer == NULL || timer_duration_sec == 0) {
        return 0;
    }

    uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);
    uint32_t elapsed_ms = now_ms - timer_start_ms;
    uint32_t elapsed_sec = elapsed_ms / 1000;

    if (elapsed_sec >= timer_duration_sec) {
        return 0;
    }

    return timer_duration_sec - (uint16_t)elapsed_sec;
}

bool relay_timer_is_active(void)
{
    return (relay_timer != NULL);
}
