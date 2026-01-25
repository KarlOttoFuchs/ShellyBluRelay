idf_component_register(
    SRCS "boot_reason.c" "error_log.c" "firmware_version.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES esp_system esp_timer
)
