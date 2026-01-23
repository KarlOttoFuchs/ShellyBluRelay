idf_component_register(
    SRCS "ble_scanner.c"
    INCLUDE_DIRS "."
    REQUIRES bt relay_control led_control nvs_flash bthome_parser
)
