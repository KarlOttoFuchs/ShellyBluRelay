# ESP32-C3 Relay Module Firmware

Firmware for ESP32-C3 based relay module with BLE sensor monitoring.

## Hardware

- **MCU**: ESP32-C3-MINI-1 (4MB Flash)
- **Relay**: SRD-03VDC-SL-C (3VDC coil, 10A contacts)
- **Connectivity**: BLE 5.0, USB-C Serial
- **Power**: 12-24VDC or USB-C 5V

## Build Instructions

### Prerequisites

- ESP-IDF v5.1.x LTS or v5.2.x LTS
- ESP-IDF configured in environment (`source ~/esp/esp-idf/export.sh`)

### Build Steps

```bash
# Configure for ESP32-C3 target (first time only)
idf.py set-target esp32c3

# Build firmware
idf.py build

# Flash firmware to ESP32-C3
idf.py flash -p /dev/ttyUSB0 -b 460800

# Monitor serial output
idf.py monitor -p /dev/ttyUSB0

# Combined workflow (build + flash + monitor)
idf.py build flash monitor
```

## GPIO Pin Mapping

| Pin | Function | Direction | Notes |
|-----|----------|-----------|-------|
| GPIO0 | Error LED | Output | Active HIGH, 1kΩ resistor |
| GPIO7 | Relay Control | Output | Active HIGH, MOSFET driver |
| GPIO9 | Button Input | Input | Active LOW, 10kΩ pull-up |
| GPIO10 | Status LED | Output | Active HIGH, 1kΩ resistor |
| GPIO18/19 | USB D-/D+ | I/O | Native USB |
| GPIO20/21 | UART RX/TX | I/O | Alternative serial |

## Safety Features

**NFR1: Fail-Safe Relay Default**
- Relay MUST default to OFF in 100% of error conditions
- GPIO7 is configured FIRST in app_main() and set to LOW
- This is a safety-critical requirement and cannot be compromised

## Component Structure

```
esp32c3-relay-firmware/
├── main/                   # Main application
│   ├── main.c              # Entry point and GPIO initialization
│   ├── gpio_config.h       # GPIO pin definitions
│   └── CMakeLists.txt
├── components/             # ESP-IDF components
│   ├── bthome_parser/      # (Future: Story 2.2)
│   ├── nvs_storage/        # (Future: Story 3.1)
│   ├── relay_control/      # (Future: Story 4A.1)
│   └── serial_protocol/    # (Future: Story 1.5)
├── test/                   # Unit tests
├── CMakeLists.txt          # Root CMake configuration
└── README.md
```

## Serial Protocol

- **Baud Rate**: 115200 (8N1)
- **Interface**: Native USB or UART
- Command format: TBD (Story 1.5)

## Development Status

Story 1.1: ✅ ESP-IDF project initialized, GPIO configuration complete
