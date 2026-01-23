idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES relay_control button_input led_control serial_protocol ble_scanner bthome_parser)
