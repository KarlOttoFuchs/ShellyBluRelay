/*
 * Firmware Version Tracking - Implementation
 *
 * Story 5.4: Firmware Version & Boot Reason Tracking
 */

#include "firmware_version.h"
#include "esp_log.h"

static const char *TAG = "FW_VERSION";

const char* get_firmware_version(void)
{
    // Return compile-time version string (Story 5.4 AC2)
    return FIRMWARE_VERSION;
}

void get_firmware_version_components(int *major, int *minor, int *patch)
{
    if (major) *major = FIRMWARE_VERSION_MAJOR;
    if (minor) *minor = FIRMWARE_VERSION_MINOR;
    if (patch) *patch = FIRMWARE_VERSION_PATCH;
}
