# ESP32-C3 Relay Module - Pin and Port Mapping

## Hardware Revision
- **Board**: ESP32-C3 Relay Module Rev A
- **MCU**: ESP32-C3-MINI-1-H4 (4MB Flash)
- **Schematic File**: `Hardware/ESP32C3-Relay-Module-Rev-A/ESP32C3-Relay-Module-Rev-A.kicad_sch`

---

## GPIO Pin Assignments

| GPIO Pin | Module Pin | Signal Name | Function | Component | Notes |
|----------|------------|-------------|----------|-----------|-------|
| **GPIO0** | 12 | ERR_LED | Error LED (Red) | D1 | Active High |
| **GPIO2** | 5 | IO2 | General Purpose I/O | - | Strapping pin, pulled high, not used |
| **GPIO7** | 21 | RLY | Relay Control | K1, Q1 | Active High, drives MOSFET |
| **GPIO8** | 22 | IO8 | General Purpose I/O | - | Strapping pin, pulled high, not used |
| **GPIO9** | 23 | BTN | User Button Input | S1 | Active Low (pulled up) |
| **GPIO10** | 16 | USR_LED | User LED (White) | D3 | Active High |
| **GPIO18** | 26 | USB_D- | USB Data- | J1 | USB interface |
| **GPIO19** | 27 | USB_D+ | USB Data+ | J1 | USB interface |
| **GPIO20** | 30 | RXD0 | UART0 RX | - | Serial communication |
| **GPIO21** | 31 | TXD0 | UART0 TX | - | Serial communication |
| **EN** | 8 | EN | Enable/Reset | - | Reset with pull-up (R3 10kΩ) |

---

## Peripheral Components

### LEDs

| Component | Type | Color | Signal | GPIO | Current Limiting Resistor |
|-----------|------|-------|--------|------|---------------------------|
| D1 | Indicator LED | Red | ERR_LED | GPIO0 | R7 (1kΩ) |
| D3 | Indicator LED | White | USR_LED | GPIO10 | R8 (1kΩ) |
| D7 | Power LED | Green | +3V3 | - | R11 (1kΩ) |

### Buttons

| Component | Type | Signal | GPIO | Function | Pull-up |
|-----------|------|--------|------|----------|---------|
| S1 | Tactile Button (160gf) | BTN | GPIO9 | User button / Boot | R6 (10kΩ) to 3.3V |

### Relay

| Component | Type | Signal | GPIO | Specifications |
|-----------|------|--------|------|----------------|
| K1 | Relay | RLY | GPIO7 | SRD-03VDC-SL-C, 3VDC Coil |
| Q1 | MOSFET | Q1_G | GPIO7 | AO3400A, drives relay coil via R9 (100Ω gate resistor) |
| D2 | Flyback Diode | - | - | 1N4148W, protects Q1 |

**Relay Terminals (J2 - Screw Terminal):**
- **R_COM**: Common (pole)
- **R_NO**: Normally Open contact
- **R_NC**: Normally Closed contact

---

## Power Supply

### Power Input

| Terminal | Signal | Voltage Range | Protection |
|----------|--------|---------------|------------|
| J2-1 | +DC_IN | 5V-24V DC | D5 (SMBJ24A TVS), D4 (B5819W Schottky) |
| J2-2 | GND | Ground | - |

### Power Rails

| Rail | Voltage | Regulator | Current Capability | Notes |
|------|---------|-----------|-------------------|-------|
| +3V3 | 3.3V | U1 (AP63203WU) | High current | Buck converter for MCU and peripherals |
| +VBUS | 5V | - | USB powered | From USB connector J1 |
| +DC_IN | 5-24V | - | External | Powers relay and 3.3V regulator |

### Power Components

| Component | Part Number | Function |
|-----------|-------------|----------|
| U1 | AP63203WU | 3.3V Buck Converter |
| L1 | VLS6045EX-6R8M | 6.8µH Inductor for buck converter |
| D6 | B5819W SL | Schottky diode for buck converter |
| C8, C9 | 22µF | Input/output caps for buck converter |

---

## Communication Interfaces

### USB Interface

| Component | Type | Signals | Protection |
|-----------|------|---------|------------|
| J1 | USB Type-C | VBUS, D+, D-, GND, CC1, CC2 | U3 (USBLC6-2SC6 ESD protection) |
| R1, R2 | 5.1kΩ | CC1, CC2 | USB-C configuration (5V default) |

*Note: ESP32-C3 has native USB support on GPIO18/GPIO19*

### UART Interface

| Signal | GPIO | Function |
|--------|------|----------|
| TXD0 | GPIO21 | UART transmit |
| RXD0 | GPIO20 | UART receive |

---

## External I/O Connector

**J2 - 6-Position Screw Terminal (5.08mm pitch):**

| Pin | Signal | Description |
|-----|--------|-------------|
| 1 | +DC_IN | Power input (5-24V DC) |
| 2 | GND | Ground |
| 3 | R_COM | Relay common terminal |
| 4 | R_NC | Relay normally closed contact |
| 5 | R_NO | Relay normally open contact |
| 6 | GND | Ground |

*Additional GPIO may be available on test points or headers - verify PCB layout*

---

## GPIO Configuration Summary for Firmware

```c
// Pin Definitions
#define PIN_LED_ERROR    0   // Red LED (active high)
#define PIN_IO2          2   // General purpose I/O (strapping pin, not used)
#define PIN_RELAY        7   // Relay control (active high)
#define PIN_IO8          8   // General purpose I/O (strapping pin, not used)
#define PIN_BUTTON       9   // User button (active low, with pull-up)
#define PIN_LED_USER     10  // White LED (active high)
#define PIN_USB_DN       18  // USB D- (native USB)
#define PIN_USB_DP       19  // USB D+ (native USB)
#define PIN_UART_RX      20  // UART RX
#define PIN_UART_TX      21  // UART TX

// Active States
// LEDs: HIGH = ON, LOW = OFF
// Button: LOW = PRESSED, HIGH = RELEASED (pulled up)
// Relay: HIGH = ENERGIZED (contacts switched), LOW = DE-ENERGIZED (normal position)
```

---

## Boot Mode Configuration

The ESP32-C3 enters different boot modes based on GPIO state during reset:

| GPIO9 | GPIO8 | GPIO2 | Boot Mode |
|-------|-------|-------|-----------|
| 1 | X | X | SPI Boot (normal) |
| 0 | 0 | X | Download Boot (UART/USB) |

**For this board:**
- GPIO8 and GPIO2 are pulled up by default (normal boot)
- Entering download mode requires pulling GPIO9 low during reset

---

## Important Notes

1. **Relay Switching**: The relay coil is driven through MOSFET Q1 with 100Ω gate resistor (R9). GPIO7 must be set HIGH to energize the relay.

2. **LED Current**: Both indicator LEDs (D1 Red, D3 White) use 1kΩ current limiting resistors (R7, R8). At 3.3V, assuming 2V LED forward voltage, this provides approximately (3.3V - 2V) / 1kΩ = 1.3mA of LED current. Power indicator D7 (Green) also uses 1kΩ resistor (R11).

3. **Button Debouncing**: The tactile button (S1) may require software debouncing in firmware.

4. **Power Considerations**: When the relay is energized, ensure adequate power supply current is available for both the ESP32-C3 module and the relay coil (~30-40mA for relay).

5. **USB Programming**: The ESP32-C3 can be programmed directly via USB using the native USB interface (GPIO18/GPIO19).

6. **Relay Contacts**: Maximum relay contact ratings should be observed (check datasheet for SRD-03VDC-SL-C). Typically 10A @ 250VAC or 10A @ 30VDC.

---

## References

- ESP32-C3 Datasheet: [Espressif Systems](https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf)
- ESP32-C3-MINI-1 Datasheet: [Espressif Systems](https://www.espressif.com/sites/default/files/documentation/esp32-c3-mini-1_datasheet_en.pdf)
- Schematic: `Hardware/ESP32C3-Relay-Module-Rev-A/ESP32C3-Relay-Module-Rev-A.kicad_sch`

---

*Document generated from hardware schematic analysis*
*Last updated: 2026-01-14*
