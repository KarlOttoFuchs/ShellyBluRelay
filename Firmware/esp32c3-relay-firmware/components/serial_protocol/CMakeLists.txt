idf_component_register(
    SRCS "serial_protocol.c"
    INCLUDE_DIRS "."
    REQUIRES relay_control led_control button_input ble_scanner esp_driver_usb_serial_jtag vfs
)
