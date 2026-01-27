# ESP32-C3 Relay Module Firmware

Production firmware for ESP32-C3 based relay module with BLE sensor monitoring. Supports Shelly BLU sensors via BTHome v2 protocol.

## Features

- **BTHome v2 sensor support** — Button, Motion, and Door/Window sensors
- **30-second learning mode** — button-triggered sensor registration
- **Button-based timer presets** — 30s, 60s, 120s, 300s (no serial required)
- **Persistent configuration** — settings saved to NVS with CRC validation
- **Watchdog protection** — 10-second timeout with automatic recovery
- **Fail-safe relay** — defaults OFF on boot, reset, or any error condition

## Hardware

- **MCU**: ESP32-C3-MINI-1 (RISC-V, 4MB Flash, BLE 5.0)
- **Relay**: SRD-03VDC-SL-C (3VDC coil, 10A contacts)
- **Power**: USB-C 5V or external DC 12-24V (auto-switching)
- **Interface**: Physical button, status/error LEDs, USB serial

## Quick Start (Button Only)

1. **Power on** → Status LED off (unconfigured)
2. **Long press** button (2+ seconds) → Fast blink (learning mode)
3. **Trigger sensor** within 30 seconds → Sensor registered
4. **Ready** → Status LED slow blinks (listening)

### Change Timer Preset

In listening mode (slow blink), use short presses to cycle presets:

| LED Blinks | Duration |
|------------|----------|
| 1 | 30 seconds |
| 2 | 60 seconds |
| 3 | 120 seconds |
| 4 | 300 seconds (default) |

Wait 5 seconds after selecting to save.

## LED Patterns

### Status LED (GPIO10 - White)

| Pattern | State | Meaning |
|---------|-------|---------|
| OFF | UNCONFIGURED | No sensor registered |
| Fast blink (250ms) | LEARNING | 30-second registration window |
| Slow blink (1s) | LISTENING | Ready for sensor triggers |
| Solid ON | ACTIVE | Relay energized, timer running |

### Error LED (GPIO0 - Red)

| Pattern | Meaning |
|---------|---------|
| 1 blink | Sensor battery low (< 20%) |
| 2 blinks | NVS storage corruption |
| 3 blinks | BLE initialization failure |

## GPIO Pin Mapping

| GPIO | Function | Direction | Notes |
|------|----------|-----------|-------|
| 0 | Error LED | Output | Active HIGH, 1kΩ resistor |
| 7 | Relay Control | Output | Active HIGH, MOSFET driver |
| 9 | Button Input | Input | Active LOW, 10kΩ pull-up |
| 10 | Status LED | Output | Active HIGH, 1kΩ resistor |
| 18/19 | USB D-/D+ | I/O | Native USB Serial JTAG |
| 20/21 | UART RX/TX | I/O | Alternative serial |

## Serial Protocol (Advanced)

Connect via USB at **115200 baud** (8N1).

| Command | Description |
|---------|-------------|
| `STATUS` | JSON output: state, sensor, timer, RSSI, events |
| `RELAY ON` / `RELAY OFF` | Manual relay control |
| `SET_TIMER <1-600>` | Set timer duration in seconds |
| `SET_RETRIGGER EXTEND` | New triggers reset timer (default) |
| `SET_RETRIGGER IGNORE` | Ignore triggers while active |
| `REGISTER_SENSOR <MAC> <TYPE>` | Manual sensor registration |
| `BLE_EVENTS` | Show last 10 trigger events |
| `GET_ERRORS` | Show last 10 system errors |
| `CLEAR_SENSOR` | Unregister sensor |
| `HELP` | List all commands |

## Build Instructions

### Prerequisites

- ESP-IDF v5.1.x or v5.2.x LTS
- Environment configured: `source ~/esp/esp-idf/export.sh`

### Build & Flash

```bash
# Configure target (first time only)
idf.py set-target esp32c3

# Build
idf.py build

# Flash
idf.py flash -p /dev/ttyUSB0 -b 460800

# Monitor
idf.py monitor -p /dev/ttyUSB0

# All-in-one
idf.py build flash monitor
```

## Component Structure

```
esp32c3-relay-firmware/
├── main/
│   ├── main.c              # Entry point, initialization sequence
│   ├── gpio_config.h       # Pin definitions
│   └── CMakeLists.txt
├── components/
│   ├── relay_control/      # GPIO7 relay driver, fail-safe OFF
│   ├── button_input/       # GPIO9 debouncing, short/long press
│   ├── led_control/        # Status/error LED patterns
│   ├── serial_protocol/    # UART command parser
│   ├── ble_scanner/        # BLE advertising scanner
│   ├── bthome_parser/      # BTHome v2 packet decoder
│   ├── nvs_storage/        # Persistent config with CRC
│   ├── app_state/          # State machine management
│   ├── system_info/        # Boot reason, error log, version
│   └── timer_presets/      # Button-based timer cycling
├── docs/
│   └── architecture.md     # Detailed technical documentation
└── README.md
```

## Safety Features

**NFR1: Fail-Safe Relay Default**
- Relay MUST default to OFF in all error conditions
- GPIO7 configured FIRST in `app_main()` and set LOW
- Watchdog reset forces relay OFF before system restart

**Initialization Order (Critical)**
1. Relay OFF (GPIO7 LOW) — non-negotiable first step
2. Button, LED, NVS, Serial
3. Load config from NVS (or use defaults)
4. BLE scanner, learning mode, watchdog

## Default Configuration

| Setting | Default | Description |
|---------|---------|-------------|
| Timer | 300 seconds | Relay on-time after trigger |
| Retrigger | EXTEND | New triggers reset countdown |
| State | UNCONFIGURED | Waiting for sensor |

## Documentation

- [Architecture & Protocol Details](docs/architecture.md)
- [Hardware Pin Mapping](../../Hardware/ESP32C3-Pin-Mapping.md)

## License

MIT License
