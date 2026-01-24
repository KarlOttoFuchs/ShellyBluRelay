/*
 * BLE Scanner Component - Implementation
 *
 * Story 2.1: Initialize BLE Stack & Scan for Advertising Packets
 *
 * Uses ESP32-C3 NimBLE stack for passive BLE scanning.
 * Tracks recently seen devices (last 60 seconds) for BLE_SCAN command.
 */

#include "ble_scanner.h"
#include "relay_control.h"
#include "led_control.h"
#include "bthome_parser.h"
#include "nvs_storage.h"
#include "state_machine.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_gap.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>

static const char *TAG = "ble_scanner";

// Device tracking structure
static ble_tracked_device_t tracked_devices[BLE_MAX_TRACKED_DEVICES];
static SemaphoreHandle_t tracked_devices_mutex = NULL;
static bool initialized = false;

// MAC filtering cache (Story 2.4 - performance optimization)
// Caches registered sensor MAC to avoid NVS reads on every packet
static char cached_registered_mac[18] = {0};
static bool mac_cache_valid = false;

// BLE scan parameters (per architecture.md)
static struct ble_gap_disc_params disc_params = {
    .filter_duplicates = 0,                     // Receive every packet for RSSI tracking
    .passive = 1,                               // Passive scan - no scan requests
    .itvl = 0x00A0,                             // 160 * 0.625ms = 100ms interval
    .window = 0x0050,                           // 80 * 0.625ms = 50ms window
    .filter_policy = BLE_HCI_SCAN_FILT_NO_WL,   // No whitelist filtering
    .limited = 0,                               // General discovery, not limited
};

// Forward declarations
static int ble_gap_event(struct ble_gap_event *event, void *arg);
static void ble_on_sync(void);
static void ble_on_reset(int reason);
static void ble_host_task(void *param);
static void ble_scanner_add_device(const char *mac_str, int8_t rssi);
static void format_mac_address(const uint8_t *mac, char *str);

/**
 * Format MAC address as uppercase with colon separators
 * CRITICAL: Per architecture.md, MAC format MUST be "AA:BB:CC:DD:EE:FF"
 *
 * NOTE: BLE addresses are transmitted in little-endian byte order (LSB first),
 * but MAC addresses are conventionally displayed in big-endian order (MSB first).
 * We reverse the byte order here for correct display.
 */
static void format_mac_address(const uint8_t *mac, char *str)
{
    sprintf(str, "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);
}

/**
 * Add or update device in tracking structure
 * Thread-safe: protected by mutex
 */
static void ble_scanner_add_device(const char *mac_str, int8_t rssi)
{
    if (tracked_devices_mutex == NULL) {
        return;
    }

    uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);

    xSemaphoreTake(tracked_devices_mutex, portMAX_DELAY);

    // First, try to find existing device and update
    for (int i = 0; i < BLE_MAX_TRACKED_DEVICES; i++) {
        if (tracked_devices[i].active && strcmp(tracked_devices[i].mac, mac_str) == 0) {
            tracked_devices[i].rssi = rssi;
            tracked_devices[i].last_seen_ms = now_ms;
            xSemaphoreGive(tracked_devices_mutex);
            return;
        }
    }

    // Device not found, add to first available slot
    for (int i = 0; i < BLE_MAX_TRACKED_DEVICES; i++) {
        if (!tracked_devices[i].active) {
            strncpy(tracked_devices[i].mac, mac_str, sizeof(tracked_devices[i].mac) - 1);
            tracked_devices[i].mac[sizeof(tracked_devices[i].mac) - 1] = '\0';
            tracked_devices[i].rssi = rssi;
            tracked_devices[i].last_seen_ms = now_ms;
            tracked_devices[i].active = true;
            xSemaphoreGive(tracked_devices_mutex);
            return;
        }
    }

    // Buffer full - replace oldest entry
    int oldest_idx = 0;
    uint32_t oldest_time = tracked_devices[0].last_seen_ms;
    for (int i = 1; i < BLE_MAX_TRACKED_DEVICES; i++) {
        if (tracked_devices[i].last_seen_ms < oldest_time) {
            oldest_time = tracked_devices[i].last_seen_ms;
            oldest_idx = i;
        }
    }

    strncpy(tracked_devices[oldest_idx].mac, mac_str, sizeof(tracked_devices[oldest_idx].mac) - 1);
    tracked_devices[oldest_idx].mac[sizeof(tracked_devices[oldest_idx].mac) - 1] = '\0';
    tracked_devices[oldest_idx].rssi = rssi;
    tracked_devices[oldest_idx].last_seen_ms = now_ms;
    tracked_devices[oldest_idx].active = true;

    xSemaphoreGive(tracked_devices_mutex);
}

/**
 * BLE GAP event callback
 * Handles discovery events (advertising packets) and discovery complete
 */
static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
    case BLE_GAP_EVENT_DISC:
        {
            // Extract device info
            char mac_str[18];
            format_mac_address(event->disc.addr.val, mac_str);
            int8_t rssi = event->disc.rssi;
            uint8_t adv_len = event->disc.length_data;

            // Log at DEBUG level to avoid spam (AC3)
            ESP_LOGD(TAG, "BLE: %s RSSI:%d len:%d", mac_str, rssi, adv_len);

            // Store in tracking structure (all sensors visible in BLE_SCAN per AC8)
            ble_scanner_add_device(mac_str, rssi);

            // Learning mode bypass (Story 3.2): Process ALL BTHome packets when learning
            bool in_learning_mode = (state_get_current() == STATE_LEARNING);

            if (in_learning_mode) {
                // Learning mode: process all BTHome packets to capture any sensor
                ESP_LOGD(TAG, "Learning mode: processing packet from %s", mac_str);
                bthome_parse_packet(mac_str, event->disc.data, adv_len, rssi);
                break;
            }

            // MAC filtering for BTHome packet processing (Story 2.4, AC7)
            // Use cached MAC to avoid NVS reads on every packet (performance fix)
            if (!mac_cache_valid) {
                // Cache miss - load from NVS (only happens once after boot or refresh)
                esp_err_t ret = nvs_get_registered_sensor_mac(cached_registered_mac, sizeof(cached_registered_mac));
                if (ret == ESP_OK) {
                    mac_cache_valid = true;
                    ESP_LOGI(TAG, "MAC filter cache loaded: %s", cached_registered_mac);
                } else if (ret == ESP_ERR_NVS_NOT_FOUND) {
                    // No sensor registered - clear cache and skip processing
                    cached_registered_mac[0] = '\0';
                    mac_cache_valid = true;  // Cache the "no sensor" state too
                    ESP_LOGD(TAG, "No sensor registered (cached)");
                } else {
                    // NVS error - skip processing, relay stays OFF (safety)
                    ESP_LOGE(TAG, "NVS read failed: %s", esp_err_to_name(ret));
                    break;
                }
            }

            // Check if any sensor is registered
            if (cached_registered_mac[0] == '\0') {
                // No sensor registered - skip all event processing
                ESP_LOGD(TAG, "No sensor registered, packet ignored");
                break;
            }

            // Compare packet MAC with cached registered MAC
            if (strcmp(mac_str, cached_registered_mac) != 0) {
                // MAC does not match - log and ignore
                ESP_LOGD(TAG, "Packet from unregistered sensor: %s (registered: %s)",
                         mac_str, cached_registered_mac);
                break;
            }

            // 3. MAC matches - proceed with BTHome parsing and handler dispatch
            ESP_LOGI(TAG, "Packet from registered sensor: %s", mac_str);
            bthome_parse_packet(mac_str, event->disc.data, adv_len, rssi);
        }
        break;

    case BLE_GAP_EVENT_DISC_COMPLETE:
        // Restart scanning (continuous monitoring per AC4)
        ESP_LOGD(TAG, "BLE scan complete, restarting...");
        ble_gap_disc(BLE_OWN_ADDR_PUBLIC, BLE_HS_FOREVER, &disc_params, ble_gap_event, NULL);
        break;

    default:
        break;
    }
    return 0;
}

/**
 * NimBLE sync callback - called when BLE stack is ready
 */
static void ble_on_sync(void)
{
    ESP_LOGI(TAG, "BLE stack synchronized, starting scan...");

    // Start passive BLE scanning
    int rc = ble_gap_disc(BLE_OWN_ADDR_PUBLIC, BLE_HS_FOREVER, &disc_params, ble_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to start BLE scan: %d", rc);
        relay_force_off();
        led_set_pattern(LED_ERROR, LED_PATTERN_ERROR_TRIPLE);
    } else {
        ESP_LOGI(TAG, "BLE scan started successfully");
    }
}

/**
 * NimBLE reset callback
 */
static void ble_on_reset(int reason)
{
    ESP_LOGW(TAG, "BLE stack reset, reason: %d", reason);
}

/**
 * NimBLE host task
 */
static void ble_host_task(void *param)
{
    ESP_LOGI(TAG, "NimBLE host task started");
    nimble_port_run();
    nimble_port_freertos_deinit();
}

/**
 * Initialize BLE scanner component
 */
esp_err_t ble_scanner_init(void)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "Initializing BLE scanner...");

    // Initialize tracking structure
    memset(tracked_devices, 0, sizeof(tracked_devices));
    tracked_devices_mutex = xSemaphoreCreateMutex();
    if (tracked_devices_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex");
        relay_force_off();
        led_set_pattern(LED_ERROR, LED_PATTERN_ERROR_TRIPLE);
        return ESP_ERR_NO_MEM;
    }

    // Note: NVS is already initialized by nvs_storage_init() in main.c
    // Do NOT call nvs_flash_init() here to avoid double-init and potential data loss

    // Initialize NimBLE controller and host
    ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NimBLE init failed: %s", esp_err_to_name(ret));
        relay_force_off();
        led_set_pattern(LED_ERROR, LED_PATTERN_ERROR_TRIPLE);
        return ret;
    }

    // Configure BLE host callbacks
    ble_hs_cfg.reset_cb = ble_on_reset;
    ble_hs_cfg.sync_cb = ble_on_sync;

    // Start NimBLE host task
    nimble_port_freertos_init(ble_host_task);

    initialized = true;
    ESP_LOGI(TAG, "BLE scanner initialized successfully");

    return ESP_OK;
}

/**
 * Get list of recently seen BLE devices
 * Returns devices seen within the last 60 seconds
 */
esp_err_t ble_scanner_get_devices(ble_tracked_device_t *devices, uint32_t max_devices, uint32_t *count)
{
    if (devices == NULL || count == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (tracked_devices_mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    *count = 0;
    uint32_t now_ms = (uint32_t)(esp_timer_get_time() / 1000);

    xSemaphoreTake(tracked_devices_mutex, portMAX_DELAY);

    // Copy active devices that are within timeout period
    for (int i = 0; i < BLE_MAX_TRACKED_DEVICES && *count < max_devices; i++) {
        if (tracked_devices[i].active) {
            uint32_t age_ms = now_ms - tracked_devices[i].last_seen_ms;
            if (age_ms <= BLE_DEVICE_TIMEOUT_MS) {
                memcpy(&devices[*count], &tracked_devices[i], sizeof(ble_tracked_device_t));
                (*count)++;
            } else {
                // Age out old devices
                tracked_devices[i].active = false;
            }
        }
    }

    xSemaphoreGive(tracked_devices_mutex);

    return ESP_OK;
}

/**
 * Invalidate MAC filter cache
 * Forces reload from NVS on next packet (Story 2.4)
 */
void ble_scanner_invalidate_mac_cache(void)
{
    mac_cache_valid = false;
    ESP_LOGD(TAG, "MAC filter cache invalidated");
}
