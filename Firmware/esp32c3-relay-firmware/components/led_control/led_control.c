/*
 * LED Control Component - Implementation
 *
 * Story 1.4: Implement LED Control (Status & Error LEDs)
 *
 * Uses gpio_reset_pin() + gpio_config() pattern per Story 1.2/1.3 learning.
 * Pattern task runs at priority 3 (lower than button monitor at 5).
 */

#include "led_control.h"
#include "relay_control.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "LED_CTRL";

// GPIO assignments (from gpio_config.h)
#define LED_STATUS_GPIO  10
#define LED_ERROR_GPIO   0

// Pattern timing in milliseconds
#define LED_BLINK_FAST_ON_MS     250
#define LED_BLINK_FAST_OFF_MS    250
#define LED_BLINK_SLOW_ON_MS     1000
#define LED_BLINK_SLOW_OFF_MS    1000
#define LED_ERROR_BLINK_MS       150   // Short blink for error patterns
#define LED_ERROR_GAP_MS         150   // Gap between blinks in error pattern
#define LED_ERROR_PAUSE_MS       1000  // Pause after error pattern completes

// Task configuration
#define LED_TASK_TICK_MS         10    // Pattern task tick interval
#define LED_TASK_STACK_SIZE      2048  // Stack size for pattern task
#define LED_TASK_PRIORITY        3     // Lower than button monitor (5)

// Module state
static bool led_initialized = false;
static led_pattern_t led_patterns[LED_COUNT] = {LED_PATTERN_OFF, LED_PATTERN_OFF};
static uint32_t led_timers[LED_COUNT] = {0, 0};
static uint8_t led_states[LED_COUNT] = {0, 0};        // Current GPIO level
static uint8_t led_blink_counts[LED_COUNT] = {0, 0};  // For error patterns

// GPIO lookup table
static const gpio_num_t led_gpios[LED_COUNT] = {
    LED_STATUS_GPIO,  // LED_STATUS
    LED_ERROR_GPIO    // LED_ERROR
};

// Story 1.8: Counted blink mode state
static bool counted_blink_active[LED_COUNT] = {false, false};
static uint8_t counted_blink_target[LED_COUNT] = {0, 0};
static uint8_t counted_blink_current[LED_COUNT] = {0, 0};
static led_pattern_t counted_blink_saved_pattern[LED_COUNT] = {LED_PATTERN_OFF, LED_PATTERN_OFF};
static void (*counted_blink_callback[LED_COUNT])(void) = {NULL, NULL};

/**
 * Get number of blinks for error pattern
 */
static uint8_t get_error_blink_count(led_pattern_t pattern)
{
    switch (pattern) {
        case LED_PATTERN_ERROR_SINGLE: return 1;
        case LED_PATTERN_ERROR_DOUBLE: return 2;
        case LED_PATTERN_ERROR_TRIPLE: return 3;
        default: return 0;
    }
}

/**
 * Process pattern for a single LED
 * Called every LED_TASK_TICK_MS
 */
static void process_led_pattern(led_id_t led, uint32_t elapsed_ms)
{
    led_pattern_t pattern = led_patterns[led];
    gpio_num_t gpio = led_gpios[led];

    switch (pattern) {
        case LED_PATTERN_OFF:
            if (led_states[led] != 0) {
                gpio_set_level(gpio, 0);
                led_states[led] = 0;
            }
            break;

        case LED_PATTERN_ON:
            if (led_states[led] != 1) {
                gpio_set_level(gpio, 1);
                led_states[led] = 1;
            }
            break;

        case LED_PATTERN_BLINK_FAST:
            led_timers[led] += elapsed_ms;
            if (led_states[led] == 0 && led_timers[led] >= LED_BLINK_FAST_OFF_MS) {
                gpio_set_level(gpio, 1);
                led_states[led] = 1;
                led_timers[led] = 0;
            } else if (led_states[led] == 1 && led_timers[led] >= LED_BLINK_FAST_ON_MS) {
                gpio_set_level(gpio, 0);
                led_states[led] = 0;
                led_timers[led] = 0;
            }
            break;

        case LED_PATTERN_BLINK_SLOW:
            led_timers[led] += elapsed_ms;
            if (led_states[led] == 0 && led_timers[led] >= LED_BLINK_SLOW_OFF_MS) {
                gpio_set_level(gpio, 1);
                led_states[led] = 1;
                led_timers[led] = 0;
            } else if (led_states[led] == 1 && led_timers[led] >= LED_BLINK_SLOW_ON_MS) {
                gpio_set_level(gpio, 0);
                led_states[led] = 0;
                led_timers[led] = 0;
            }
            break;

        case LED_PATTERN_ERROR_SINGLE:
        case LED_PATTERN_ERROR_DOUBLE:
        case LED_PATTERN_ERROR_TRIPLE:
        {
            uint8_t target_blinks = get_error_blink_count(pattern);
            led_timers[led] += elapsed_ms;

            // State machine for error blink pattern
            // blink_counts tracks which blink we're on (0 to target_blinks-1)
            // led_states: 0=off during blink, 1=on during blink, 2=off during pause

            if (led_states[led] == 0) {
                // LED is off, waiting to turn on for next blink
                if (led_timers[led] >= LED_ERROR_GAP_MS) {
                    gpio_set_level(gpio, 1);
                    led_states[led] = 1;
                    led_timers[led] = 0;
                }
            } else if (led_states[led] == 1) {
                // LED is on during blink
                if (led_timers[led] >= LED_ERROR_BLINK_MS) {
                    gpio_set_level(gpio, 0);
                    led_blink_counts[led]++;
                    led_timers[led] = 0;

                    if (led_blink_counts[led] >= target_blinks) {
                        // Done with blinks, enter pause
                        led_states[led] = 2;
                        led_blink_counts[led] = 0;
                    } else {
                        // More blinks to go
                        led_states[led] = 0;
                    }
                }
            } else {
                // led_states[led] == 2: Pause after all blinks
                if (led_timers[led] >= LED_ERROR_PAUSE_MS) {
                    led_states[led] = 0;
                    led_timers[led] = 0;
                }
            }
            break;
        }

        default:
            break;
    }
}

/**
 * Process counted blink for a single LED (Story 1.8)
 * Called every LED_TASK_TICK_MS when counted_blink_active is true
 *
 * State machine:
 *   state 1: LED ON during blink
 *   state 0: LED OFF between blinks
 *   state 2: LED OFF during post-blink pause (visual separation)
 */
static void process_counted_blink(led_id_t led, uint32_t elapsed_ms)
{
    if (!counted_blink_active[led]) {
        return;  // Not in counted blink mode
    }

    gpio_num_t gpio = led_gpios[led];
    led_timers[led] += elapsed_ms;

    if (led_states[led] == 1) {
        // LED is ON - wait for on duration
        if (led_timers[led] >= LED_BLINK_COUNT_ON_MS) {
            gpio_set_level(gpio, 0);
            led_timers[led] = 0;
            counted_blink_current[led]++;

            // Check if all blinks complete
            if (counted_blink_current[led] >= counted_blink_target[led]) {
                // Enter pause state for visual separation before restoring pattern
                led_states[led] = 2;
            } else {
                // More blinks to go
                led_states[led] = 0;
            }
        }
    } else if (led_states[led] == 0) {
        // LED is OFF between blinks - wait for off duration, then turn on for next blink
        if (led_timers[led] >= LED_BLINK_COUNT_OFF_MS) {
            gpio_set_level(gpio, 1);
            led_states[led] = 1;
            led_timers[led] = 0;
        }
    } else {
        // state 2: Post-blink pause - LED stays OFF for visual separation
        if (led_timers[led] >= LED_BLINK_COUNT_PAUSE_MS) {
            // Pause complete - restore pattern
            counted_blink_active[led] = false;
            led_set_pattern(led, counted_blink_saved_pattern[led]);

            ESP_LOGI(TAG, "LED %s counted blink complete",
                     led == LED_STATUS ? "STATUS" : "ERROR");

            // Call callback if provided
            if (counted_blink_callback[led] != NULL) {
                void (*cb)(void) = counted_blink_callback[led];
                counted_blink_callback[led] = NULL;
                cb();
            }
        }
    }
}

/**
 * LED pattern task
 * Runs patterns for all LEDs in non-blocking manner
 */
static void led_pattern_task(void *pvParameters)
{
    TickType_t last_wake_time = xTaskGetTickCount();

    while (1) {
        // Story 1.8: Process counted blinks first (takes priority over normal patterns)
        for (int i = 0; i < LED_COUNT; i++) {
            if (counted_blink_active[i]) {
                process_counted_blink(i, LED_TASK_TICK_MS);
            } else {
                process_led_pattern(i, LED_TASK_TICK_MS);
            }
        }

        // Delay until next tick
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(LED_TASK_TICK_MS));
    }
}

esp_err_t led_init(void)
{
    esp_err_t ret;

    if (led_initialized) {
        ESP_LOGW(TAG, "LED control already initialized");
        return ESP_OK;
    }

    // Reset GPIO pads to clear any boot-time configuration
    gpio_reset_pin(LED_STATUS_GPIO);
    gpio_reset_pin(LED_ERROR_GPIO);

    // Configure both GPIOs with full pad configuration
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << LED_STATUS_GPIO) | (1ULL << LED_ERROR_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure LED GPIOs: %s", esp_err_to_name(ret));
        relay_force_off();  // Safety first
        return ret;
    }

    // LEDs off initially
    gpio_set_level(LED_STATUS_GPIO, 0);
    gpio_set_level(LED_ERROR_GPIO, 0);

    // Initialize state
    for (int i = 0; i < LED_COUNT; i++) {
        led_patterns[i] = LED_PATTERN_OFF;
        led_timers[i] = 0;
        led_states[i] = 0;
        led_blink_counts[i] = 0;
    }

    // Create pattern task
    BaseType_t task_ret = xTaskCreate(
        led_pattern_task,
        "led_pattern",
        LED_TASK_STACK_SIZE,
        NULL,
        LED_TASK_PRIORITY,
        NULL
    );

    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create LED pattern task");
        relay_force_off();  // Safety first
        return ESP_FAIL;
    }

    led_initialized = true;
    ESP_LOGI(TAG, "LED control initialized (GPIO%d status, GPIO%d error)",
             LED_STATUS_GPIO, LED_ERROR_GPIO);

    return ESP_OK;
}

esp_err_t led_set_pattern(led_id_t led, led_pattern_t pattern)
{
    if (!led_initialized) {
        ESP_LOGE(TAG, "LED control not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (led >= LED_COUNT) {
        ESP_LOGE(TAG, "Invalid LED ID: %d", led);
        return ESP_ERR_INVALID_ARG;
    }

    if (pattern >= LED_PATTERN_COUNT) {
        ESP_LOGE(TAG, "Invalid pattern: %d", pattern);
        return ESP_ERR_INVALID_ARG;
    }

    // Reset pattern state when changing patterns
    led_patterns[led] = pattern;
    led_timers[led] = 0;
    led_blink_counts[led] = 0;

    // For solid patterns, set state immediately
    if (pattern == LED_PATTERN_OFF) {
        gpio_set_level(led_gpios[led], 0);
        led_states[led] = 0;
    } else if (pattern == LED_PATTERN_ON) {
        gpio_set_level(led_gpios[led], 1);
        led_states[led] = 1;
    } else {
        // For blinking patterns, start from off state
        gpio_set_level(led_gpios[led], 0);
        led_states[led] = 0;
    }

    ESP_LOGI(TAG, "LED %s pattern set to %d",
             led == LED_STATUS ? "STATUS" : "ERROR", pattern);

    return ESP_OK;
}

esp_err_t led_set_state(led_id_t led, bool on)
{
    return led_set_pattern(led, on ? LED_PATTERN_ON : LED_PATTERN_OFF);
}

esp_err_t led_get_pattern(led_id_t led, led_pattern_t *pattern)
{
    if (!led_initialized) {
        ESP_LOGE(TAG, "LED control not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (led >= LED_COUNT) {
        ESP_LOGE(TAG, "Invalid LED ID: %d", led);
        return ESP_ERR_INVALID_ARG;
    }

    if (pattern == NULL) {
        ESP_LOGE(TAG, "NULL pattern pointer");
        return ESP_ERR_INVALID_ARG;
    }

    *pattern = led_patterns[led];
    return ESP_OK;
}

esp_err_t led_blink_count(led_id_t led, uint8_t count, void (*callback)(void))
{
    if (!led_initialized) {
        ESP_LOGE(TAG, "LED control not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    if (led >= LED_COUNT) {
        ESP_LOGE(TAG, "Invalid LED ID: %d", led);
        return ESP_ERR_INVALID_ARG;
    }

    // Handle edge case: count = 0 (immediate callback, no blinks)
    if (count == 0) {
        if (callback != NULL) {
            callback();
        }
        return ESP_OK;
    }

    // Clamp count to maximum
    if (count > LED_BLINK_COUNT_MAX) {
        count = LED_BLINK_COUNT_MAX;
    }

    // Save current pattern for restoration
    led_get_pattern(led, &counted_blink_saved_pattern[led]);

    // Initialize counted blink state
    counted_blink_target[led] = count;
    counted_blink_current[led] = 0;
    counted_blink_callback[led] = callback;
    counted_blink_active[led] = true;

    // Start first blink (LED ON)
    gpio_set_level(led_gpios[led], 1);
    led_states[led] = 1;
    led_timers[led] = 0;

    ESP_LOGI(TAG, "LED %s counted blink started: %d blinks",
             led == LED_STATUS ? "STATUS" : "ERROR", count);

    return ESP_OK;
}
