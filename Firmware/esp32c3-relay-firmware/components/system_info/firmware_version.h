/*
 * Firmware Version Tracking - API
 *
 * Story 5.4: Firmware Version & Boot Reason Tracking
 */

#ifndef FIRMWARE_VERSION_H
#define FIRMWARE_VERSION_H

// Firmware version string (Story 5.4 AC1)
// Format: "MAJOR.MINOR.PATCH"
#define FIRMWARE_VERSION_MAJOR 1
#define FIRMWARE_VERSION_MINOR 0
#define FIRMWARE_VERSION_PATCH 0

// Macro to generate version string at compile time
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define FIRMWARE_VERSION \
    TOSTRING(FIRMWARE_VERSION_MAJOR) "." \
    TOSTRING(FIRMWARE_VERSION_MINOR) "." \
    TOSTRING(FIRMWARE_VERSION_PATCH)

/**
 * Get firmware version string
 *
 * Returns the firmware version in "MAJOR.MINOR.PATCH" format per Story 5.4 AC1.
 *
 * @return Static string with firmware version (do not free)
 */
const char* get_firmware_version(void);

/**
 * Get firmware version components
 *
 * @param major  Pointer to store major version number
 * @param minor  Pointer to store minor version number
 * @param patch  Pointer to store patch version number
 */
void get_firmware_version_components(int *major, int *minor, int *patch);

#endif // FIRMWARE_VERSION_H
