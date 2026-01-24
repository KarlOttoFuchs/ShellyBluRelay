idf_component_register(
    SRCS "bthome_parser.c"
         "sensor_button.c"
         "sensor_motion.c"
         "sensor_door.c"
    INCLUDE_DIRS "."
    REQUIRES "led_control"  # For battery threshold LED warnings (Story 2.4)
    PRIV_REQUIRES "esp_timer"  # For esp_timer_get_time()
)
