idf_component_register(
    SRCS "state_machine.c" "learning_mode.c"
    INCLUDE_DIRS "."
    REQUIRES led_control nvs_storage ble_scanner bthome_parser relay_control
)
