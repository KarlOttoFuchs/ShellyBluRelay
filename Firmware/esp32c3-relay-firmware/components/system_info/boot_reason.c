/*
 * Boot Reason Tracking - Implementation
 *
 * Story 4B.4: Tracks and reports boot/reset reason for diagnostics
 */

#include "boot_reason.h"
#include "esp_log.h"

static const char *TAG = "BOOT_REASON";

// Store boot reason at startup for later retrieval
static esp_reset_reason_t s_boot_reason = ESP_RST_UNKNOWN;
static bool s_initialized = false;

void boot_reason_init(void)
{
    if (s_initialized) {
        return;  // Already initialized
    }

    // Capture boot reason immediately (AC1)
    s_boot_reason = esp_reset_reason();
    s_initialized = true;

    // Log warning if previous boot was crash or watchdog (AC2)
    if (s_boot_reason == ESP_RST_PANIC || s_boot_reason == ESP_RST_TASK_WDT ||
        s_boot_reason == ESP_RST_INT_WDT) {
        ESP_LOGW(TAG, "Previous boot: %s (reason=%d)", get_boot_reason_string(), (int)s_boot_reason);
    } else {
        ESP_LOGI(TAG, "Boot reason: %s (reason=%d)", get_boot_reason_string(), (int)s_boot_reason);
    }
}

const char* get_boot_reason_string(void)
{
    // Map ESP-IDF reset reasons to human-readable strings (AC4)
    switch (s_boot_reason) {
        case ESP_RST_POWERON:   return "POWER_ON";
        case ESP_RST_SW:        return "SOFTWARE";
        case ESP_RST_PANIC:     return "PANIC";
        case ESP_RST_TASK_WDT:  return "WATCHDOG";
        case ESP_RST_INT_WDT:   return "WATCHDOG";  // Interrupt WDT also reported as WATCHDOG
        case ESP_RST_BROWNOUT:  return "BROWNOUT";
        case ESP_RST_DEEPSLEEP: return "DEEPSLEEP";
        case ESP_RST_SDIO:      return "SDIO";
        case 11:                return "USB";       // ESP_RST_USB - ESP32-C3 USB peripheral reset
        case 12:                return "JTAG";      // ESP_RST_JTAG - ESP32-C3 JTAG reset
        case ESP_RST_UNKNOWN:   return "UNKNOWN";
        default:                return "UNKNOWN";
    }
}

esp_reset_reason_t get_boot_reason(void)
{
    return s_boot_reason;
}
