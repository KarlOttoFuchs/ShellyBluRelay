idf_component_register(
    SRCS "timer_presets.c"
    INCLUDE_DIRS "."
    REQUIRES button_input led_control nvs_storage app_state
)
