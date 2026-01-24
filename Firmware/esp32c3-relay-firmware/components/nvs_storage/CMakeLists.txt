idf_component_register(
    SRCS "nvs_storage.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash relay_control led_control  # Story 3.1: Added relay_control for relay_force_off(), led_control for error LED
)
