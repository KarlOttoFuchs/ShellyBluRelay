idf_component_register(
    SRCS
        "test_main.c"
        "../../components/led_control/test/test_led_control.c"
        "../../components/serial_protocol/test/test_serial_protocol.c"
        # Add other test files here as you add them:
        # "../../components/button_input/test/test_button_input.c"
        # "../../components/relay_control/test/test_relay_control.c"
        # "../../components/ble_scanner/test/test_ble_scanner.c"
    INCLUDE_DIRS
        "."
        "../../components/led_control/test"
        "../../components/serial_protocol/test"
        # Add other test include directories as needed:
        # "../../components/button_input/test"
        # "../../components/relay_control/test"
        # "../../components/ble_scanner/test"
    REQUIRES unity led_control button_input relay_control serial_protocol ble_scanner driver
)
