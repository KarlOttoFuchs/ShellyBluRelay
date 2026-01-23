idf_component_register(
    SRCS "led_control.c"
    INCLUDE_DIRS "."
    REQUIRES driver relay_control
)
