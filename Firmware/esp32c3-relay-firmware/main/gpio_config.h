/*
 * GPIO Pin Configuration for ESP32-C3 Relay Module
 *
 * Pin assignments per Hardware/ESP32C3-Pin-Mapping.md
 */

#ifndef GPIO_CONFIG_H
#define GPIO_CONFIG_H

// LED Pins (GPIO outputs, active HIGH)
#define PIN_LED_ERROR    0      // Red error LED (D1, R7=1kΩ, ~1.3mA)
#define PIN_LED_STATUS   10     // White status LED (D3, R8=1kΩ, ~1.3mA)

// Relay Pin (GPIO output, active HIGH - drives MOSFET Q1 via R9=100Ω)
#define PIN_RELAY        7      // Relay control (K1: SRD-03VDC-SL-C, 3VDC coil)

// Input Pin (GPIO input, active LOW with pull-up)
#define PIN_BUTTON       9      // User button (S1, R6=10kΩ pull-up, debounce required in software)

// USB & UART Pins (handled by ESP-IDF USB/UART drivers)
#define PIN_USB_DN       18     // USB D- (native USB, J1)
#define PIN_USB_DP       19     // USB D+ (native USB, J1)
#define PIN_UART_RX      20     // UART RX (alternative serial)
#define PIN_UART_TX      21     // UART TX (alternative serial)

#endif // GPIO_CONFIG_H
