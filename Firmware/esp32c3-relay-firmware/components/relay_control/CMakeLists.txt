idf_component_register(
    SRCS "relay_control.c" "relay_timer.c"
    INCLUDE_DIRS "."
    REQUIRES driver app_state led_control nvs_storage
)
