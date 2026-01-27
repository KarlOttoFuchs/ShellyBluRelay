---
stepsCompleted: ['step-01-validate-prerequisites', 'step-02-design-epics']
inputDocuments:
  - '/Users/karlfuchs/Documents/Development/Projects/ESP32C3 Relay Module/_bmad-output/planning-artifacts/prd.md'
  - '/Users/karlfuchs/Documents/Development/Projects/ESP32C3 Relay Module/_bmad-output/planning-artifacts/architecture.md'
  - '/Users/karlfuchs/Documents/Development/Projects/ESP32C3 Relay Module/Hardware/ESP32C3-Pin-Mapping.md'
---

# ESP32C3 Relay Module - Epic Breakdown

## Overview

This document provides the complete epic and story breakdown for ESP32C3 Relay Module, decomposing the requirements from the PRD, UX Design if it exists, and Architecture requirements into implementable stories.

## Requirements Inventory

### Functional Requirements (MVP Only - FR1-FR46)

**Hardware Interface & Validation (FR1-FR6):**
- FR1: System can control relay via GPIO output
- FR2: System can detect button press via GPIO input for configuration mode
- FR3: White status LED indicates operating mode (fast blink = config, slow blink = listening, solid = relay energized)
- FR4: Red error LED indicates diagnostic states (single blink = battery low <20%, double blink = NVS failure, triple blink = BLE init failure)
- FR5: System can communicate via USB serial for configuration/status
- FR6: System operates from USB-C 5V OR external DC 12-24V with Schottky diode isolation

**BLE Sensor Communication (FR7-FR11):**
- FR7: System receives/decodes BLE advertising packets without establishing connections
- FR8: System decodes sensor events from Shelly BLU Button/Motion/Door devices
- FR9: System extracts MAC, event type, battery level, RSSI from BLE packets
- FR10: System filters BLE packets to only process registered sensor MAC
- FR11: System monitors sensor battery from BTHome packets (warning <20%, error <10%)

**Sensor Registration & Configuration (FR12-FR16):**
- FR12: System enters 30-second sensor learning mode when button pressed
- FR13: Users can manually register sensor by MAC address fallback
- FR14: System stores single sensor config persistently in NVS
- FR15: Users can clear registered sensor
- FR16: System validates sensor MAC address format

**Relay Control & Timer Logic (FR17-FR21):**
- FR17: System energizes relay when registered sensor triggers
- FR18: System de-energizes relay after configurable timer (1-600 seconds)
- FR19: Users can configure relay-on duration with 1-second granularity
- FR20: System supports "extend" or "ignore" retriggering modes
- FR21: System maintains relay state/timer config across power cycles

**Event Logging & Diagnostics (FR22-FR26):**
- FR22: System buffers last 10 trigger events with timestamps
- FR23: Users can retrieve event log showing trigger history
- FR24: System logs errors (NVS failures, BLE init failures)
- FR25: Users can retrieve error log for troubleshooting
- FR26: Users can query comprehensive system status (state, sensor info, relay, timer, errors)

**CLI Configuration Interface (FR27-FR32):**
- FR27: CLI auto-detects ESP32-C3 serial port with manual selection fallback
- FR28: CLI communicates using text-based serial protocol (115200 baud, JSON responses)
- FR29: CLI displays colorful formatted output (tables, progress, color-coded status)
- FR30: CLI provides comprehensive auto-generated help
- FR31: CLI provides clear, actionable error messages
- FR32: Users can test relay manually (ON/OFF) via CLI

**State Machine & Safety (FR33-FR38):**
- FR33: System transitions between Configuration/Listening/Active states
- FR34: System defaults relay to OFF on boot/error/power loss (fail-safe)
- FR35: System validates configuration changes before applying
- FR36: System recovers from firmware crashes with serial crash logs and stack traces
- FR37: System implements watchdog timer that resets MCU if hung
- FR38: System detects NVS corruption and prevents relay activation with corrupted config

**Firmware Management, Documentation & Extensibility (FR39-FR46):**
- FR39: Users can update firmware via standard ESP-IDF flashing tools
- FR40: System maintains firmware version info accessible via status
- FR41: Firmware logs boot reason for diagnostics
- FR42: Firmware achieves 4.0/5.0 readability rating
- FR43: Architecture docs include state machine diagram, serial protocol spec, NVS schema
- FR44: Extension guide provides clear instructions for adding new sensor types
- FR45: Code includes inline comments for BTHome parsing, state transitions
- FR46: CLI provides auto-generated help with examples

**Note:** Post-MVP features (FR47-FR56: multi-sensor, config export/import, lux gates, factory reset, OTA updates) are explicitly excluded from this epic breakdown.

### Non-Functional Requirements

**Performance (NFP1-NFP5):**
- NFP1: Relay energizes within 500ms of BLE event detection
- NFP2: Timer accurate within ±1 second over 10-minute duration
- NFP3: Process BLE packets at 10 packets/second minimum
- NFP4: CLI status command returns within 2 seconds
- NFP5: Boot completes within 5 seconds

**Reliability (NFR1-NFR5):**
- NFR1: Relay MUST default OFF in 100% of error conditions (safety-critical)
- NFR2: Watchdog resets MCU within 10 seconds if hung, relay OFF within 1 second
- NFR3: Config survives 10,000 power cycles with <0.1% corruption
- NFR4: System detects sensor events with >95% success at RSSI ≥ -70 dBm
- NFR5: Operates continuously for 168 hours without crash/memory leak

**Usability (NFU1-NFU5):**
- NFU1: 80% of first-time users succeed within 10 minutes
- NFU2: 100% of error messages include actionable guidance
- NFU3: Status command answers: relay state, sensor registered, in range, timer, errors
- NFU4: Every CLI command has help text with description, constraints, examples
- NFU5: Users distinguish LED patterns without documentation (>90% accuracy)

**Maintainability (NFM1-NFM4):**
- NFM1: Firmware achieves ≥4.0/5.0 readability from 3+ developers
- NFM2: Docs include state machine diagram, serial protocol spec, NVS schema
- NFM3: Add new sensor type in ≤4 hours using extension guide
- NFM4: Non-obvious code has explanatory comments (BTHome parsing, state transitions, NVS, watchdog)

**Compatibility (NFC1-NFC5):**
- NFC1: CLI runs on Windows 10+, macOS 11+, Linux Ubuntu 20.04+
- NFC2: CLI auto-detects CP2102/CH340 bridges, manual fallback for multiple ports
- NFC3: Firmware compiles on ESP-IDF 5.1.x and 5.2.x LTS
- NFC4: Parses BTHome v2 from Shelly BLU Button/Motion/Door devices
- NFC5: CLI runs on Python 3.8+

### Additional Requirements

**From Architecture Document:**
- Architecture specifies standard ESP-IDF project initialization (no custom starter template)
- Serial protocol MUST use exact format: `COMMAND [ARG]\n` and `OK|data\n` or `ERROR|code|message\n`
- NVS schema: sensor_mac, sensor_type, timer_seconds, retrigger_mode, config_version, config_crc
- BTHome parser uses handler registry pattern for extensibility
- Component structure: bthome_parser, nvs_storage, relay_control, serial_protocol as separate ESP-IDF components
- Python CLI uses Typer + Rich frameworks per research findings
- Firmware MUST call `relay_force_off()` BEFORE any error handling (safety-critical pattern)
- All firmware functions return `esp_err_t` and check with ESP_ERROR_CHECK
- MAC addresses MUST be uppercase with colon separators (AA:BB:CC:DD:EE:FF)

**From Hardware Pin Mapping Document:**
- GPIO0: Error LED (Red) - Active High
- GPIO7: Relay Control - Active High, drives MOSFET Q1 with 100Ω gate resistor
- GPIO9: User Button - Active Low with 10kΩ pull-up
- GPIO10: User LED (White) - Active High
- GPIO18/19: USB D-/D+ (native USB)
- GPIO20/21: UART RX/TX (serial communication)
- Relay: SRD-03VDC-SL-C, 3VDC Coil, driven via AO3400A MOSFET
- Power: Dual input with Schottky diodes - USB-C 5V OR external DC 5-24V (AP63203WU buck converter to 3.3V)
- Current consumption: ~15-30mA idle, +50-80mA relay active, ~100-150mA peak
- LED current limiting: 1kΩ resistors (R7 error LED, R8 user LED) = ~1.3mA per LED
- Button debouncing required in software
- Boot mode: GPIO9 LOW during reset = Download mode (USB/UART programming)

### FR Coverage Map

**Epic 1: Hardware Validation & Foundational Setup**
- FR1: System can control relay via GPIO output
- FR2: System can detect button press via GPIO input for configuration mode
- FR3: White status LED indicates operating mode
- FR4: Red error LED indicates diagnostic states
- FR5: System can communicate via USB serial for configuration/status
- FR6: System operates from USB-C 5V OR external DC 12-24V with Schottky diode isolation

**Note:** FR39, FR40, FR41 (firmware version & boot tracking) moved to Epic 5 (Diagnostics) for better fit with STATUS command implementation.

**Epic 2: BLE Sensor Communication & Parsing**
- FR7: System receives/decodes BLE advertising packets without establishing connections
- FR8: System decodes sensor events from Shelly BLU Button/Motion/Door devices
- FR9: System extracts MAC, event type, battery level, RSSI from BLE packets
- FR10: System filters BLE packets to only process registered sensor MAC
- FR11: System monitors sensor battery from BTHome packets (warning <20%, error <10%)

**Epic 3: Sensor Registration & Configuration Management**
- FR12: System enters 30-second sensor learning mode when button pressed
- FR13: Users can manually register sensor by MAC address fallback
- FR14: System stores single sensor config persistently in NVS
- FR15: Users can clear registered sensor
- FR16: System validates sensor MAC address format

**Epic 4A: Basic Relay Control & Timer**
- FR17: System energizes relay when registered sensor triggers
- FR18: System de-energizes relay after configurable timer (1-600 seconds)
- FR19: Users can configure relay-on duration with 1-second granularity
- FR20: System supports "extend" or "ignore" retriggering modes
- FR21: System maintains relay state/timer config across power cycles

**Epic 4B: State Machine & Safety Systems**
- FR33: System transitions between Configuration/Listening/Active states
- FR34: System defaults relay to OFF on boot/error/power loss (fail-safe)
- FR35: System validates configuration changes before applying
- FR36: System recovers from firmware crashes with serial crash logs and stack traces
- FR37: System implements watchdog timer that resets MCU if hung
- FR38: System detects NVS corruption and prevents relay activation with corrupted config

**Epic 5: Event Logging & Diagnostics**
- FR22: System buffers last 10 trigger events with timestamps
- FR23: Users can retrieve event log showing trigger history
- FR24: System logs errors (NVS failures, BLE init failures)
- FR25: Users can retrieve error log for troubleshooting
- FR26: Users can query comprehensive system status (state, sensor info, relay, timer, errors)
- FR39: Users can update firmware via standard ESP-IDF flashing tools
- FR40: System maintains firmware version info accessible via status
- FR41: Firmware logs boot reason for diagnostics

**Epic 6: Python CLI Tool - Core Commands**
- FR27: CLI auto-detects ESP32-C3 serial port with manual selection fallback
- FR28: CLI communicates using text-based serial protocol (115200 baud, JSON responses)
- FR29: CLI displays colorful formatted output (tables, progress, color-coded status)
- FR30: CLI provides comprehensive auto-generated help
- FR31: CLI provides clear, actionable error messages
- FR32: Users can test relay manually (ON/OFF) via CLI

**Epic 7: Documentation & Educational Quality**
- FR42: Firmware achieves 4.0/5.0 readability rating
- FR43: Architecture docs include state machine diagram, serial protocol spec, NVS schema
- FR44: Extension guide provides clear instructions for adding new sensor types
- FR45: Code includes inline comments for BTHome parsing, state transitions
- FR46: CLI provides auto-generated help with examples

## Epic List

### Epic 1: Hardware Validation & Foundational Setup

**Goal:** Developers get a reusable hardware test suite that validates all interfaces work on newly assembled boards.

Developers will be able to verify relay control, button input detection, LED feedback (white status, red error), USB serial communication, and dual power supply operation via a comprehensive test suite executable on every newly assembled board.

**FRs covered:** FR1, FR2, FR3, FR4, FR5, FR6

**Architecture requirements:** ESP-IDF project structure, GPIO pin mapping (per Hardware/ESP32C3-Pin-Mapping.md), serial protocol foundation, hardware test commands

**Note:** FR39-FR41 (firmware version & boot tracking) moved to Epic 5 for better alignment with STATUS command implementation.

### Epic 2: BLE Sensor Communication & Parsing

**Goal:** System can reliably detect and decode events from Shelly BLU sensors.

ESP32-C3 receives BLE advertising packets from Shelly BLU devices, decodes Button (single/double/triple press), Motion, and Door events, extracts MAC address, event type, battery level, and RSSI, and filters packets by registered sensor MAC.

**FRs covered:** FR7, FR8, FR9, FR10, FR11

**Architecture requirements:** BTHome v2 parser with handler registry pattern, sensor_button.c, sensor_motion.c, sensor_door.c components

### Epic 3: Sensor Registration & Configuration Management

**Goal:** Users can register a sensor and persist configuration across power cycles.

Users can press the physical button to enter 30-second learning mode, auto-register sensor by triggering it, manually register sensor by MAC address as fallback, clear registered sensor, and configuration survives power cycles via NVS persistence.

**FRs covered:** FR12, FR13, FR14, FR15, FR16

**Architecture requirements:** NVS storage component with CRC validation, sensor config schema (sensor_mac, sensor_type, timer_seconds, retrigger_mode, config_version, config_crc)

### Epic 4A: Basic Relay Control & Timer

**Goal:** System activates relay when sensor triggers, with configurable countdown timer.

Registered sensor triggers relay activation, relay de-energizes automatically after timer expires (1-600 seconds), users can configure timer duration and retriggering mode (extend timer vs. ignore new triggers), and timer configuration persists across power cycles.

**FRs covered:** FR17, FR18, FR19, FR20, FR21

**Architecture requirements:** relay_control component, timer management, retriggering logic

**Validation:** End-to-end test (sensor → relay ON → timer expires → relay OFF), latency <500ms (NFP1), timer accuracy ±1s (NFP2)

### Epic 4B: State Machine & Safety Systems

**Goal:** System guarantees fail-safe relay behavior with comprehensive safety mechanisms.

System transitions between Configuration/Listening/Active states, relay defaults to OFF on boot/error/power loss (fail-safe behavior), validates configuration changes, recovers from firmware crashes with serial logs, implements watchdog timer for hung detection, and detects NVS corruption to prevent unsafe relay operation.

**FRs covered:** FR33, FR34, FR35, FR36, FR37, FR38

**Architecture requirements:** State machine implementation, watchdog timer, fail-safe relay-OFF pattern, NVS corruption detection, crash recovery

**Validation:** Fault injection tests (10+ scenarios: NVS corruption, firmware crash, watchdog timeout, power loss) - relay MUST be OFF in 100% of cases (NFR1)

### Epic 5: Event Logging & Diagnostics

**Goal:** Users can troubleshoot system behavior and monitor sensor health.

Users can view last 10 trigger events with timestamps and RSSI, retrieve error log (NVS failures, BLE init failures), monitor sensor battery level (warnings at <20%, errors at <10%), and query comprehensive system status (state, sensor info, relay, timer, errors).

**FRs covered:** FR22, FR23, FR24, FR25, FR26

**Architecture requirements:** Event buffer (circular buffer in RAM), battery monitoring from BTHome packets, comprehensive status reporting

### Epic 6: Python CLI Tool - Core Commands

**Goal:** Users can configure and control the relay module via professional CLI interface.

Users can auto-detect ESP32-C3 serial port (with manual selection fallback), test relay manually (ON/OFF), register sensor via CLI (trigger-based or manual MAC entry), set timer duration and retriggering mode, clear sensor configuration, view comprehensive status with color-coded output (tables, RSSI interpretation, battery warnings), retrieve event logs and error logs, and get auto-generated help for all commands.

**FRs covered:** FR27, FR28, FR29, FR30, FR31, FR32

**Architecture requirements:** Python package with Typer + Rich frameworks, serial_protocol.py, formatters.py, command modules (status, register_sensor, set_timer, test_relay, clear_sensor, logs)

### Epic 7: Documentation & Educational Quality

**Goal:** Developers can understand, extend, and maintain the firmware codebase.

Developers can review architecture documentation (state machine diagram, serial protocol spec, NVS schema), follow extension guide to add new sensor types, understand BTHome parsing and state transitions through inline comments, use CLI help system for command examples and constraints, with firmware meeting 4.0/5.0 readability rating.

**FRs covered:** FR42, FR43, FR44, FR45, FR46

**Architecture requirements:** Architecture diagrams, extension guide, inline comments for BTHome parsing and state transitions, comprehensive CLI help system

## Epic 1: Hardware Validation & Foundational Setup

Developers get a reusable hardware test suite that validates all interfaces work on newly assembled boards.

### Story 1.1: Initialize ESP-IDF Project & GPIO Configuration

As a firmware developer,
I want to initialize an ESP-IDF project with proper GPIO pin configuration,
So that I have a validated foundation for all hardware interfaces.

**Acceptance Criteria:**

**Given** a fresh ESP32-C3 development environment
**When** I run `idf.py create-project esp32c3-relay-firmware`
**Then** the project is created with standard ESP-IDF structure (main/, components/, CMakeLists.txt, sdkconfig)

**And** GPIO pins are configured per Hardware/ESP32C3-Pin-Mapping.md:
- GPIO0: Error LED output (active high)
- GPIO7: Relay control output (active high, drives MOSFET)
- GPIO9: Button input (active low with internal pull-up enabled)
- GPIO10: Status LED output (active high)
- GPIO18/19: USB D-/D+ (native USB, configured by ESP-IDF)
- GPIO20/21: UART RX/TX (configured by ESP-IDF for serial)

**And** project compiles successfully with `idf.py build`

**And** firmware flashes to ESP32-C3 with `idf.py flash`

**And** boot log visible via `idf.py monitor` shows successful initialization

### Story 1.2: Implement Relay Control (GPIO7)

As a firmware developer,
I want to control the relay via GPIO7,
So that I can validate relay hardware functionality.

**Acceptance Criteria:**

**Given** firmware is flashed to the ESP32-C3
**When** GPIO7 is set HIGH
**Then** relay energizes (audible click, MOSFET Q1 conducts, relay coil SRD-03VDC-SL-C energized)

**And** relay contacts switch (R_COM connects to R_NO, disconnects from R_NC)

**When** GPIO7 is set LOW
**Then** relay de-energizes (audible click, MOSFET Q1 off, relay returns to normal position)

**And** relay contacts return to rest position (R_COM connects to R_NC, disconnects from R_NO)

**And** firmware implements `relay_set_state(bool on)` function that:
- Returns `esp_err_t` status code
- Checks return value with ESP_ERROR_CHECK
- Logs state change with ESP_LOGI

**And** on boot, relay defaults to OFF state (GPIO7 LOW) per safety requirement NFR1

### Story 1.3: Implement Button Input Detection (GPIO9)

As a firmware developer,
I want to detect button presses on GPIO9,
So that I can validate button hardware and enable user input.

**Acceptance Criteria:**

**Given** firmware is running
**When** physical button S1 is NOT pressed
**Then** GPIO9 reads HIGH (pulled up by R6 10kΩ resistor)

**When** physical button S1 is pressed
**Then** GPIO9 reads LOW (button connects GPIO9 to GND)

**And** firmware implements `button_read_state()` function that:
- Returns `esp_err_t` status code
- Returns button state via output parameter (true = pressed, false = released)
- Includes software debouncing (20ms delay between reads)
- Logs button state changes with ESP_LOGI

**And** button state changes are detectable within 50ms (debounce + read latency)

### Story 1.4: Implement LED Control (Status & Error LEDs)

As a firmware developer,
I want to control both status and error LEDs,
So that I can validate LED hardware and provide visual feedback.

**Acceptance Criteria:**

**Given** firmware is running
**When** status LED (GPIO10) is set ON
**Then** white LED D3 illuminates (active high, ~1.3mA current via R8 1kΩ resistor)

**When** status LED is set OFF
**Then** white LED D3 turns off

**When** error LED (GPIO0) is set ON
**Then** red LED D1 illuminates (active high, ~1.3mA current via R7 1kΩ resistor)

**When** error LED is set OFF
**Then** red LED D1 turns off

**And** firmware implements `led_set_pattern()` function that supports:
- Solid ON
- Solid OFF
- Fast blink (500ms on/off for configuration mode per FR3)
- Slow blink (2s on/off for listening mode per FR3)
- Error patterns (single/double/triple blink per FR4)

**And** LED patterns run in FreeRTOS task without blocking main loop

**And** all LED functions return `esp_err_t` and use ESP_LOG* macros

### Story 1.5: Serial Protocol Foundation & Hardware Test Suite

As a hardware developer,
I want a comprehensive test suite that validates all GPIO interfaces via serial commands,
So that I can quickly verify newly assembled boards work correctly.

**Acceptance Criteria:**

**Given** firmware is flashed to a newly assembled ESP32-C3 board
**When** I connect via USB serial at 115200 baud (8N1)
**Then** firmware responds to the following test commands:

**Serial connectivity validated by HW_TEST command** (Story 1.6)
- If HW_TEST responds, serial communication is functional
- No separate PING command needed
- HW_TEST validates both serial and hardware in single command

**Command: `TEST_RELAY ON`**
- Firmware responds: `OK|relay_on\n`
- Relay energizes (audible click)
- Validates FR1 (relay control)

**Command: `TEST_RELAY OFF`**
- Firmware responds: `OK|relay_off\n`
- Relay de-energizes (audible click)

**Command: `TEST_LED STATUS ON`**
- Firmware responds: `OK|led_on\n`
- White status LED illuminates
- Validates FR3 (status LED)

**Command: `TEST_LED STATUS OFF`**
- Firmware responds: `OK|led_off\n`
- White status LED turns off

**Command: `TEST_LED STATUS BLINK`**
- Firmware responds: `OK|led_blinking\n`
- White LED blinks at configured pattern

**Command: `TEST_LED ERROR ON`**
- Firmware responds: `OK|led_on\n`
- Red error LED illuminates
- Validates FR4 (error LED)

**Command: `TEST_LED ERROR OFF`**
- Firmware responds: `OK|led_off\n`
- Red error LED turns off

**Command: `TEST_BUTTON`**
- Firmware responds: `OK|button_state|released\n` when button not pressed
- Firmware responds: `OK|button_state|pressed\n` when button is pressed
- Validates FR2 (button input)

**Command: `HELP`**
- Firmware responds with list of all available test commands
- Each command includes usage example

**And** serial protocol uses exact format per architecture:
- Command format: `COMMAND [ARG1] [ARG2]\n`
- Success response: `OK|data\n`
- Error response: `ERROR|code|message\n`

**And** test suite is documented in `docs/hardware-validation.md` with step-by-step board bringup procedure

**And** optional Python automation script `tools/hardware_test.py` provided for automated validation

**And** all test commands work with manual testing via PuTTY, screen, or minicom

**And** firmware validates FR6 (dual power supply) by successfully running from:
- USB-C 5V power only
- External DC 12-24V power only
- Both power sources connected (Schottky diode isolation prevents conflict)

### Story 1.6: Comprehensive Hardware Validation Command (HW_TEST)

As a hardware technician or field engineer,
I want a single command that validates all hardware components in one test sequence,
So that I can quickly verify a newly assembled or deployed board without running multiple separate commands.

**Context:** After cleanup of test commands (commit e20d464), individual TEST_LED, TEST_BUTTON commands were removed as development scaffolding. Story 1.6 provides a manual HW_TEST validation command where technician triggers test via button press and observes hardware operation visually/audibly.

**Acceptance Criteria:**

**Given** firmware is running on ESP32-C3 hardware
**When** user sends `HW_TEST\n` command
**Then** firmware responds: `OK|hw_test_waiting|Press the button to start hardware validation`
**And** firmware waits for button press (max 30 seconds)

**When** technician presses button
**Then** firmware executes manual validation sequence:
1. Flash white status LED (500ms) - technician observes
2. Flash red error LED (500ms) - technician observes
3. Pulse relay ON→OFF (500ms) - technician hears audible click

**And** firmware responds: `OK|hw_test_complete`

**And** test sequence completes in approximately 2 seconds after button press

**And** technician validates hardware by observation (no automated pass/fail)

**And** all components return to safe state (LEDs OFF, relay OFF)

**And** timeout occurs if button not pressed within 30 seconds: `ERROR|hw_test_timeout|Button not pressed within 30 seconds`

**And** replaces PING command (HW_TEST validates both serial and hardware)

**And** HELP command includes: `HW_TEST - Run comprehensive hardware validation test`

**Dependencies:** Stories 1.2 (relay), 1.3 (button), 1.4 (LEDs)

**Related:** Replaces TEST_LED, TEST_BUTTON, TEST_RELAY and PING commands

## Epic 2: BLE Sensor Communication & Parsing

System can reliably detect and decode events from Shelly BLU sensors.

### Story 2.1: Initialize BLE Stack & Scan for Advertising Packets

As a firmware developer,
I want to initialize the ESP32-C3 BLE stack and scan for advertising packets,
So that I can receive raw BLE data from nearby sensors without establishing connections.

**Acceptance Criteria:**

**Given** firmware with GPIO foundation from Epic 1
**When** firmware boots
**Then** BLE controller and host stack initialize successfully (ESP-IDF Bluedroid or NimBLE)

**And** BLE initialization errors logged with ESP_LOGE and error LED shows triple blink (per FR4)

**When** BLE stack is initialized
**Then** firmware starts passive BLE scanning (no connections, advertising packets only)

**And** scan parameters configured:
- Scan type: Passive (no scan requests)
- Scan interval: 100ms
- Scan window: 50ms
- Filter duplicates: Disabled (receive every packet for RSSI tracking)

**When** BLE advertising packets are received
**Then** firmware logs each packet with:
- MAC address (uppercase with colon separators per architecture: AA:BB:CC:DD:EE:FF)
- RSSI value
- Advertising data length
- Timestamp (system uptime in milliseconds)

**And** firmware processes packets at minimum 10 packets/second (NFP3)

**And** scan continues indefinitely without stopping (continuous monitoring)

**And** serial command `BLE_SCAN` returns list of recently seen devices (last 60 seconds) with MAC and RSSI

**And** validates FR7 (receive/decode BLE advertising packets without connections)

### Story 2.2: Implement BTHome v2 Parser with Handler Registry

As a firmware developer,
I want a BTHome v2 parser with extensible handler registry,
So that I can decode sensor data and easily add support for new sensor types.

**Acceptance Criteria:**

**Given** BLE scanning is operational from Story 2.1
**When** advertising packet is received
**Then** parser checks for BTHome v2 service UUID 0xFCD2 in service data

**And** packets without 0xFCD2 UUID are ignored (not BTHome packets)

**When** BTHome v2 packet is detected
**Then** parser extracts packet header:
- Device Info byte per BTHome v2 spec:
  - Bit 0: Encryption flag (0 = unencrypted, 1 = encrypted)
  - Bit 1: Reserved
  - Bit 2: Trigger-based device flag (0 = interval-based, 1 = event-triggered)
  - Bits 3-4: Reserved
  - Bits 5-7: BTHome Version (010 = v2)
- Object ID-Length-Value (OLV) triplets that follow

**And** parser validates BTHome version (bits 5-7 must equal 010 for v2)

**And** parser validates encryption flag:
- If encryption bit = 1, log warning "Encrypted packets not supported" and skip packet
- If encryption bit = 0, proceed with parsing

**And** parser extracts trigger flag (bit 2) to indicate event-triggered advertising (button press, motion)

**When** parsing OLV triplets
**Then** parser iterates through triplets:
- Object ID (1 byte) identifies data type
- Length (implicit, depends on Object ID per BTHome v2 spec)
- Value (variable length)

**And** parser implements handler registry pattern:
- `bthome_register_handler(object_id, handler_function)` to register handlers
- Handlers called with: `handler(mac_address, object_id, value, value_len, rssi)`
- Unknown Object IDs logged at debug level but do not cause errors

**And** parser extracts battery level when Object ID 0x01 (Battery %) is present:
- Value = unsigned 8-bit integer (0-100%)
- Stored for battery monitoring (FR11)

**And** parser provides API:
```c
esp_err_t bthome_parse_packet(const uint8_t *mac, const uint8_t *adv_data, size_t adv_len, int8_t rssi);
esp_err_t bthome_register_handler(uint8_t object_id, bthome_handler_t handler);
```

**And** all parser functions return `esp_err_t` and use ESP_ERROR_CHECK pattern

**And** parser includes inline comments explaining BTHome v2 format per FR45

**And** validates FR9 (extract MAC, battery, RSSI) and architecture requirement for handler registry

### Story 2.3: Decode Shelly BLU Button Events

As a user,
I want the system to decode button press events from Shelly BLU Button sensor,
So that button presses can trigger relay activation.

**Acceptance Criteria:**

**Given** BTHome parser with handler registry from Story 2.2
**When** firmware initializes
**Then** Button handler is registered for Object ID 0x3A (Button event)

**When** Shelly BLU Button advertising packet is received with Object ID 0x3A
**Then** handler decodes button event value:
- 0x01 = Single press
- 0x02 = Double press
- 0x03 = Triple press
- 0x04 = Long press
- Other values logged as "Unknown button event"

**And** decoded event logged with:
- MAC address (uppercase with colons)
- Event type ("single_press", "double_press", "triple_press", "long_press")
- Battery level (if Object ID 0x01 present in same packet)
- RSSI value
- Timestamp

**And** trigger flag (bit 2 of device info byte) is set for button press events

**And** serial command `BLE_EVENTS` returns last 10 decoded button events with timestamps

**And** firmware tested with captured BLE packet from real Shelly BLU Button device:
- Test vector provided in unit test with known MAC, event type, battery, RSSI
- Parser correctly extracts all fields

**And** validates FR8 (decode sensor events from Shelly BLU Button)

### Story 2.4: Decode Motion & Door Events + Implement MAC Filtering

As a user,
I want the system to decode Motion and Door sensor events and filter by registered sensor MAC,
So that only my registered sensor triggers relay activation.

**Acceptance Criteria:**

**Given** Button handler from Story 2.3 is operational
**When** firmware initializes
**Then** Motion handler registered for Object ID 0x21 (Motion event)

**And** Door handler registered for Object ID 0x2D (Opening event)

**When** Shelly BLU Motion advertising packet received with Object ID 0x21
**Then** handler decodes motion event value:
- 0x01 = Motion detected
- 0x00 = Motion timeout (no motion for configured period)

**And** decoded motion event logged with MAC, event type, battery, RSSI, timestamp

**When** Shelly BLU Door/Window advertising packet received with Object ID 0x2D
**Then** handler decodes door event value:
- 0x01 = Open
- 0x00 = Closed

**And** decoded door event logged with MAC, event type, battery, RSSI, timestamp

**When** MAC address filtering is implemented
**Then** firmware checks if sensor MAC matches registered sensor MAC (from NVS config):
- If MAC matches: Process event and trigger relay logic
- If MAC does NOT match: Log at debug level "Packet from unregistered sensor" and ignore

**And** unregistered sensors visible in `BLE_SCAN` output but do NOT trigger events

**When** battery level is extracted (Object ID 0x01)
**Then** firmware monitors battery thresholds per FR11:
- Battery < 20%: Log warning "Sensor battery low" and set error LED single blink (per FR4)
- Battery < 10%: Log error "Sensor battery critical"
- Battery >= 20%: No warnings

**And** battery status included in STATUS command output

**And** firmware tested with captured packets from:
- Shelly BLU Motion (motion detected + timeout)
- Shelly BLU Door/Window (open + closed)

**And** validates FR8 (Motion/Door decoding), FR10 (MAC filtering), FR11 (battery monitoring)

## Epic 3: Sensor Registration & Configuration Management

Users can register a sensor and persist configuration across power cycles.

### Story 3.1: Implement NVS Storage Component with CRC Validation

As a firmware developer,
I want a robust NVS storage component with CRC validation,
So that sensor configuration survives power cycles and corrupted config is detected.

**Acceptance Criteria:**

**Given** ESP32-C3 firmware with NVS partition defined in partition table
**When** firmware boots
**Then** NVS partition initializes successfully with namespace "relay_config"

**And** NVS initialization errors logged with ESP_LOGE and error LED shows double blink (per FR4)

**When** NVS storage component is implemented
**Then** sensor config schema includes all required fields per architecture:
- `sensor_mac` (string, 17 bytes, format: AA:BB:CC:DD:EE:FF)
- `sensor_type` (uint8_t, enum: 0=BUTTON, 1=MOTION, 2=DOOR)
- `timer_seconds` (uint16_t, range: 1-600 seconds)
- `retrigger_mode` (uint8_t, enum: 0=EXTEND, 1=IGNORE)
- `config_version` (uint8_t, current version = 1)
- `config_crc` (uint32_t, CRC32 checksum of all fields)

**And** NVS component provides API:
```c
esp_err_t nvs_save_config(const sensor_config_t *config);
esp_err_t nvs_load_config(sensor_config_t *config);
esp_err_t nvs_clear_config(void);
esp_err_t nvs_validate_config(const sensor_config_t *config);
```

**When** configuration is saved with `nvs_save_config()`
**Then** CRC32 checksum is calculated over all config fields (excluding config_crc itself)

**And** checksum is stored in `config_crc` field

**And** all fields written to NVS atomically (commit only if all writes succeed)

**When** configuration is loaded with `nvs_load_config()`
**Then** all fields are read from NVS

**And** CRC32 checksum is recalculated and compared with stored `config_crc`

**And** if CRC mismatch detected:
- Log error "NVS corruption detected, CRC mismatch"
- Return `ESP_ERR_INVALID_CRC` error code
- Set error LED double blink pattern (per FR4)
- **CRITICAL:** Call `relay_force_off()` BEFORE error handling (safety-critical per architecture)
- Prevent relay activation until valid config restored

**And** if CRC valid, return `ESP_OK` and populate config struct

**When** configuration survives power cycles (NFR3)
**Then** firmware tested with 100 consecutive power cycles (subset of 10,000 requirement)

**And** config reads successfully after each boot with valid CRC

**And** validates FR14 (persistent storage), FR38 (NVS corruption detection), NFR3 (config durability)

### Story 3.2: Implement 30-Second Learning Mode

As a user,
I want to press the physical button to enter 30-second learning mode,
So that I can easily register a sensor by triggering it.

**Acceptance Criteria:**

**Given** firmware with button detection from Epic 1 and BLE scanning from Epic 2
**When** physical button (GPIO9) is pressed and held for 2 seconds
**Then** firmware enters learning mode

**And** status LED changes to fast blink pattern (500ms on/off per FR3)

**And** serial log shows "Entering learning mode for 30 seconds"

**When** learning mode is active
**Then** 30-second countdown timer starts

**And** firmware monitors BLE advertising packets for BTHome v2 sensors

**And** serial command `STATUS` shows:
```
State: LEARNING
Time remaining: XX seconds
```

**When** BTHome v2 sensor event is received during learning mode (Button press, Motion, or Door event)
**Then** firmware captures first sensor's information:
- MAC address (uppercase with colons)
- Sensor type (detected from Object ID: 0x3A=BUTTON, 0x21=MOTION, 0x2D=DOOR)
- Battery level (if present)
- RSSI value

**And** learning mode exits immediately (no need to wait 30 seconds)

**And** sensor config is saved to NVS via `nvs_save_config()`:
- sensor_mac = captured MAC
- sensor_type = detected type
- timer_seconds = default 10 seconds
- retrigger_mode = default EXTEND
- config_version = 1
- config_crc = calculated CRC32

**And** status LED changes to slow blink pattern (2s on/off, listening mode per FR3)

**And** serial log shows "Sensor registered: [MAC] ([TYPE])"

**When** 30-second timeout expires without sensor trigger
**Then** learning mode exits

**And** status LED returns to previous pattern

**And** serial log shows "Learning mode timeout, no sensor detected"

**And** validates FR12 (30-second learning mode) and FR3 (status LED patterns)

### Story 3.3: Manual Sensor Registration via Serial Command

As a user,
I want to manually register a sensor by MAC address via serial command,
So that I have a fallback if auto-learning fails or sensor is out of range.

**Acceptance Criteria:**

**Given** firmware with NVS storage from Story 3.1
**When** serial command `REGISTER_SENSOR AA:BB:CC:DD:EE:FF BUTTON` is received
**Then** firmware validates MAC address format per FR16:
- Exactly 17 characters
- Uppercase A-F hex digits
- Colon separators at positions 2, 5, 8, 11, 14
- Format: `AA:BB:CC:DD:EE:FF`

**And** if MAC format invalid, respond:
```
ERROR|INVALID_MAC|MAC must be uppercase with colons (AA:BB:CC:DD:EE:FF)
```

**When** MAC address format is valid
**Then** firmware validates sensor type argument:
- Valid types: BUTTON, MOTION, DOOR
- Case-insensitive matching

**And** if sensor type invalid, respond:
```
ERROR|INVALID_TYPE|Sensor type must be BUTTON, MOTION, or DOOR
```

**When** both MAC and sensor type are valid
**Then** firmware saves sensor config to NVS:
- sensor_mac = provided MAC (uppercase with colons per architecture)
- sensor_type = parsed type (0=BUTTON, 1=MOTION, 2=DOOR)
- timer_seconds = default 10 seconds
- retrigger_mode = default EXTEND
- config_version = 1
- config_crc = calculated CRC32

**And** firmware responds:
```
OK|registered|AA:BB:CC:DD:EE:FF|BUTTON
```

**And** status LED changes to slow blink pattern (listening mode)

**When** sensor is already registered
**Then** existing config is overwritten with new sensor

**And** firmware responds with confirmation of new sensor

**And** validates FR13 (manual MAC registration), FR16 (MAC validation), architecture requirement for MAC format

### Story 3.4: Clear Sensor Configuration & Persistence Validation

As a user,
I want to clear registered sensor configuration,
So that I can reset the device or register a different sensor.

**Acceptance Criteria:**

**Given** firmware with registered sensor from Story 3.2 or 3.3
**When** serial command `CLEAR_SENSOR` is received
**Then** firmware clears sensor config from NVS via `nvs_clear_config()`

**And** all NVS keys erased (sensor_mac, sensor_type, timer_seconds, retrigger_mode, config_version, config_crc)

**And** firmware responds:
```
OK|cleared
```

**And** status LED pattern changes to indicate unconfigured state

**And** subsequent `nvs_load_config()` calls return `ESP_ERR_NVS_NOT_FOUND`

**When** firmware is in unconfigured state (no sensor registered)
**Then** serial command `STATUS` shows:
```
State: UNCONFIGURED
Sensor: None registered
```

**And** BLE packets are scanned but do NOT trigger relay (no registered MAC to match)

**When** configuration persistence is validated across power cycles
**Then** test procedure:
1. Register sensor via `REGISTER_SENSOR` command
2. Verify `STATUS` shows registered sensor
3. Power cycle ESP32-C3 (full power off/on)
4. After boot, verify `STATUS` still shows registered sensor
5. Verify sensor_mac, sensor_type, timer_seconds, retrigger_mode match pre-reboot values
6. Verify config CRC is valid (no corruption)

**And** persistence test passes for 10 consecutive power cycles

**When** NVS corruption is detected during boot
**Then** firmware logs error "Config corrupted, relay disabled"

**And** error LED shows double blink pattern (NVS failure per FR4)

**And** relay remains OFF (fail-safe behavior per NFR1)

**And** serial command `CLEAR_SENSOR` allows recovery by clearing corrupted config

**And** validates FR15 (clear sensor), FR14 (persistence across power cycles), NFR3 (config durability)

## Epic 4A: Basic Relay Control & Timer

System activates relay when sensor triggers, with configurable countdown timer.

### Story 4A.1: Relay Activation on Registered Sensor Event

As a user,
I want the relay to energize when my registered sensor triggers,
So that I can automate devices in response to sensor events.

**Acceptance Criteria:**

**Given** firmware with registered sensor from Epic 3 and BLE parsing from Epic 2
**When** BLE packet is received from registered sensor MAC (exact match)
**Then** firmware triggers relay activation within 500ms of packet receipt (NFP1)

**And** relay energizes via `relay_set_state(true)` (GPIO7 HIGH)

**And** relay contacts switch (R_COM to R_NO, audible click)

**And** status LED changes to solid ON pattern (per FR3: solid = relay energized)

**And** serial log shows:
```
Sensor triggered: [MAC] ([EVENT_TYPE])
Relay activated
```

**When** BLE packet is received from unregistered sensor (MAC does NOT match)
**Then** relay does NOT energize

**And** packet logged at debug level "Packet from unregistered sensor"

**When** relay is activated
**Then** serial command `STATUS` shows:
```
State: ACTIVE
Relay: ON
Timer remaining: XX seconds
Sensor: [MAC] ([TYPE])
Last trigger: [TIMESTAMP]
```

**And** latency test validates NFP1:
- Measure time from BLE packet callback to relay GPIO HIGH
- Latency MUST be ≤ 500ms in 100% of test runs

**And** validates FR17 (relay energizes on registered sensor trigger), NFP1 (500ms latency)

### Story 4A.2: Automatic Timer De-energize with Configurable Duration

As a user,
I want the relay to automatically turn off after a configurable timer,
So that I don't have to manually turn it off or worry about leaving devices on indefinitely.

**Acceptance Criteria:**

**Given** relay is energized from Story 4A.1
**When** timer countdown starts
**Then** firmware uses FreeRTOS timer with 1-second tick resolution

**And** timer counts down from configured `timer_seconds` value (loaded from NVS)

**And** default timer duration = 10 seconds (if not configured)

**When** timer is counting down
**Then** serial command `STATUS` shows remaining seconds:
```
Relay: ON
Timer remaining: 7 seconds
```

**And** remaining time updates every second

**When** timer expires (countdown reaches 0)
**Then** relay de-energizes via `relay_set_state(false)` (GPIO7 LOW)

**And** relay contacts return to rest position (R_COM to R_NC, audible click)

**And** status LED changes to slow blink pattern (listening mode per FR3)

**And** serial log shows:
```
Timer expired
Relay deactivated
```

**And** serial command `STATUS` shows:
```
State: LISTENING
Relay: OFF
Timer remaining: 0 seconds
```

**When** timer accuracy is validated (NFP2)
**Then** test procedure for 10-minute (600 second) timer:
1. Set timer to 600 seconds via `SET_TIMER 600`
2. Trigger relay activation
3. Measure actual elapsed time from relay ON to relay OFF using external timer
4. Calculate error: |actual_time - 600s|
5. Error MUST be ≤ 1 second

**And** timer accuracy test passes for durations: 1s, 10s, 60s, 300s, 600s

**And** validates FR18 (auto de-energize after timer), FR19 (configurable duration), NFP2 (±1s accuracy)

### Story 4A.3: Configure Timer Duration via Serial Command

As a user,
I want to set a custom relay-on duration via serial command,
So that I can adjust the timer to match my specific use case.

**Acceptance Criteria:**

**Given** firmware with NVS storage from Epic 3
**When** serial command `SET_TIMER 60` is received
**Then** firmware validates timer value:
- Range: 1-600 seconds (per FR19)
- Integer value required

**And** if timer value out of range, respond:
```
ERROR|INVALID_RANGE|Timer must be 1-600 seconds
```

**And** if timer value not an integer, respond:
```
ERROR|INVALID_FORMAT|Timer must be an integer
```

**When** timer value is valid (1-600 seconds)
**Then** firmware updates NVS config:
- Load existing config via `nvs_load_config()`
- Update `timer_seconds` field = new value
- Recalculate CRC32 checksum
- Save config via `nvs_save_config()`

**And** firmware responds:
```
OK|timer_set|60
```

**And** serial log shows "Timer set to 60 seconds"

**When** timer config is saved to NVS
**Then** value persists across power cycles:
1. Set timer to 120 seconds via `SET_TIMER 120`
2. Verify `STATUS` shows "Timer duration: 120 seconds"
3. Power cycle ESP32-C3
4. After boot, verify `STATUS` still shows "Timer duration: 120 seconds"
5. Trigger sensor and verify relay stays ON for 120 seconds

**And** timer granularity is 1 second (FR19):
- Users can set any integer value 1-600
- Examples: 1, 5, 30, 65, 247, 600 all valid

**And** validates FR19 (configurable duration with 1-second granularity), FR21 (timer config persistence)

### Story 4A.4: Implement Retriggering Modes (Extend vs Ignore)

As a user,
I want to configure whether new sensor triggers extend the timer or are ignored,
So that I can choose the behavior that fits my automation needs.

**Acceptance Criteria:**

**Given** firmware with timer functionality from Story 4A.2
**When** serial command `SET_RETRIGGER EXTEND` is received
**Then** firmware validates retrigger mode:
- Valid modes: EXTEND, IGNORE
- Case-insensitive matching

**And** if mode invalid, respond:
```
ERROR|INVALID_MODE|Retrigger mode must be EXTEND or IGNORE
```

**When** retrigger mode is valid
**Then** firmware updates NVS config:
- Load existing config via `nvs_load_config()`
- Update `retrigger_mode` field (0=EXTEND, 1=IGNORE)
- Recalculate CRC32 checksum
- Save config via `nvs_save_config()`

**And** firmware responds:
```
OK|retrigger_set|EXTEND
```

**And** serial log shows "Retrigger mode set to EXTEND"

**When** retrigger mode = EXTEND and relay is already ON
**Then** new sensor trigger resets timer countdown:
1. Relay energizes with 10-second timer
2. After 6 seconds (4 seconds remaining), sensor triggers again
3. Timer resets to 10 seconds (full duration)
4. Relay stays ON for full 10 seconds from second trigger

**And** serial log shows:
```
Sensor triggered: [MAC] ([EVENT_TYPE])
Timer extended (reset to 10 seconds)
```

**When** retrigger mode = IGNORE and relay is already ON
**Then** new sensor triggers are ignored:
1. Relay energizes with 10-second timer
2. After 6 seconds (4 seconds remaining), sensor triggers again
3. Timer continues counting down (NOT reset)
4. Relay turns OFF 4 seconds later (original countdown completes)

**And** serial log shows:
```
Sensor triggered: [MAC] ([EVENT_TYPE])
Retrigger ignored (relay already active)
```

**When** retrigger config is saved to NVS
**Then** value persists across power cycles:
1. Set mode to IGNORE via `SET_RETRIGGER IGNORE`
2. Verify `STATUS` shows "Retrigger mode: IGNORE"
3. Power cycle ESP32-C3
4. After boot, verify `STATUS` still shows "Retrigger mode: IGNORE"
5. Test retrigger behavior matches IGNORE mode

**And** validates FR20 (EXTEND and IGNORE retriggering modes), FR21 (retrigger config persistence)

## Epic 4B: State Machine & Safety Systems

System guarantees fail-safe relay behavior with comprehensive safety mechanisms.

### Story 4B.1: Implement State Machine (Configuration/Listening/Active)

As a firmware developer,
I want a formal state machine implementation that transitions between Configuration, Listening, and Active states,
So that system behavior is predictable and state transitions are well-defined.

**Acceptance Criteria:**

**Given** firmware with all previous epic functionality
**When** state machine is implemented
**Then** firmware defines three primary states:
- **UNCONFIGURED**: No sensor registered, waiting for configuration
- **LEARNING**: 30-second learning mode active (variant of Configuration)
- **LISTENING**: Sensor registered, monitoring BLE packets
- **ACTIVE**: Relay energized, timer running

**And** state machine includes inline comments explaining state transitions per FR45

**When** firmware boots with no sensor registered
**Then** state machine enters UNCONFIGURED state

**And** status LED indicates unconfigured state

**When** user presses button for 2 seconds
**Then** state transitions: UNCONFIGURED → LEARNING

**And** status LED fast blink (500ms, per FR3)

**When** sensor is registered (learning mode or REGISTER_SENSOR command)
**Then** state transitions: LEARNING → LISTENING or UNCONFIGURED → LISTENING

**And** status LED slow blink (2s, per FR3)

**When** registered sensor triggers while in LISTENING state
**Then** state transitions: LISTENING → ACTIVE

**And** status LED solid ON (per FR3)

**When** timer expires while in ACTIVE state
**Then** state transitions: ACTIVE → LISTENING

**And** status LED slow blink (per FR3)

**And** validates FR33 (state machine transitions) and FR3 (LED patterns per state)

### Story 4B.2: Fail-Safe Relay Default OFF on Boot/Error/Power Loss

As a user,
I want the relay to default to OFF in all error conditions and power loss scenarios,
So that connected devices fail safely and don't remain energized during faults.

**Acceptance Criteria:**

**Given** firmware with relay control from Epic 1
**When** firmware boots (cold boot or reset)
**Then** relay MUST be OFF before any other initialization (GPIO7 = LOW)

**And** `relay_force_off()` called in first 10 lines of `app_main()`

**And** relay stays OFF until valid config loaded and system enters LISTENING state

**When** NVS corruption detected during boot (CRC mismatch)
**Then** `relay_force_off()` called BEFORE error handling (per architecture safety pattern)

**And** relay remains OFF

**And** error LED double blink (NVS failure per FR4)

**And** serial log shows "Config corrupted, relay disabled for safety"

**When** BLE initialization fails during boot
**Then** `relay_force_off()` called BEFORE error handling

**And** relay remains OFF

**And** error LED triple blink (BLE init failure per FR4)

**When** power loss occurs while relay is ON
**Then** relay de-energizes immediately (no power to GPIO7)

**And** on next boot, relay defaults to OFF per boot behavior

**And** validates FR34 (fail-safe default OFF), NFR1 (relay OFF in 100% of error conditions)

### Story 4B.3: Configuration Validation Before Applying

As a user,
I want configuration changes validated before being applied,
So that invalid settings don't cause system malfunction.

**Acceptance Criteria:**

**Given** firmware with NVS storage from Epic 3
**When** `SET_TIMER` command received
**Then** firmware validates timer value BEFORE saving to NVS:
- Range check: 1-600 seconds
- Type check: Integer only
- If invalid, return ERROR response WITHOUT modifying NVS

**When** `SET_RETRIGGER` command received
**Then** firmware validates retrigger mode BEFORE saving to NVS:
- Valid values: EXTEND or IGNORE only
- If invalid, return ERROR response WITHOUT modifying NVS

**When** `REGISTER_SENSOR` command received
**Then** firmware validates sensor config BEFORE saving to NVS:
- MAC format: Exactly 17 characters, uppercase hex, colons at correct positions
- Sensor type: BUTTON, MOTION, or DOOR only
- If invalid, return ERROR response WITHOUT modifying NVS

**When** NVS config is loaded on boot
**Then** firmware validates CRC32 checksum:
- Calculate CRC over all config fields (excluding config_crc)
- Compare with stored config_crc
- If mismatch, call `relay_force_off()` and reject config

**And** firmware provides `nvs_validate_config()` API that returns:
- `ESP_OK` if config valid
- `ESP_ERR_INVALID_ARG` if field values out of range
- `ESP_ERR_INVALID_CRC` if CRC mismatch

**And** validates FR35 (validate configuration changes before applying)

### Story 4B.4: Firmware Crash Recovery with Serial Logs & Stack Traces

As a developer,
I want firmware crashes to be logged with stack traces over serial,
So that I can debug issues and ensure crash recovery works correctly.

**Acceptance Criteria:**

**Given** firmware with serial protocol from Epic 1
**When** firmware crashes (division by zero, null pointer, assert failure)
**Then** ESP-IDF panic handler executes automatically

**And** crash log output to serial includes:
- Crash reason (exception type, address)
- Register dump (PC, SP, A0-A15)
- Stack backtrace with function names and line numbers
- Task that crashed (name, priority, stack watermark)

**And** relay forced OFF by hardware reset (GPIO7 returns to default LOW state)

**When** firmware reboots after crash
**Then** ESP-IDF logs boot reason to serial:
```
rst:0xc (SW_CPU_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
```

**And** firmware calls `esp_reset_reason()` to determine if previous boot was crash

**And** if crash detected, serial log shows:
```
WARNING: Previous boot was due to crash
Check serial logs for crash details
```

**And** crash history available via `esp_core_dump` integration (optional)

**When** crash recovery is tested
**Then** test scenarios:
1. Force division-by-zero exception
2. Force null pointer dereference
3. Force stack overflow
4. All scenarios: relay OFF after reboot, crash logged to serial

**And** validates FR36 (crash recovery with serial crash logs and stack traces)

### Story 4B.5: Watchdog Timer Implementation

As a user,
I want a watchdog timer that resets the MCU if firmware hangs,
So that the system recovers automatically from software faults.

**Acceptance Criteria:**

**Given** firmware main loop with BLE scanning, serial processing, timer management
**When** watchdog timer is initialized
**Then** ESP-IDF Task Watchdog Timer (TWDT) configured:
- Timeout: 10 seconds (per NFR2)
- Monitors main task and critical FreeRTOS tasks
- Panic action: Reset MCU

**And** watchdog initialized early in `app_main()` before main loop

**When** firmware is running normally
**Then** main task calls `esp_task_wdt_reset()` at least once every 10 seconds

**And** watchdog does NOT trigger (no reset)

**When** firmware hangs (infinite loop, deadlock, blocking call)
**Then** main task fails to reset watchdog within 10 seconds

**And** watchdog triggers panic

**And** firmware resets within 10 seconds of hang (NFR2)

**And** relay de-energizes within 1 second of hang via hardware reset (GPIO7 → LOW)

**When** watchdog reset is tested
**Then** test procedure:
1. Inject infinite loop in main task: `while(1) { vTaskDelay(1000); /* no watchdog reset */ }`
2. Monitor serial output for watchdog timeout message
3. Verify firmware resets within 10 seconds
4. Verify relay turns OFF (measure GPIO7 with oscilloscope if needed)
5. Confirm relay OFF within 1 second per NFR2

**And** validates FR37 (watchdog timer), NFR2 (watchdog resets within 10s, relay OFF within 1s)

### Story 4B.6: Comprehensive Safety Validation with Fault Injection

As a quality engineer,
I want comprehensive fault injection tests that validate fail-safe relay behavior,
So that I can certify the system meets NFR1 (relay OFF in 100% of error conditions).

**Acceptance Criteria:**

**Given** firmware with all safety mechanisms from Stories 4B.1-4B.5
**When** fault injection test suite is executed
**Then** test suite validates relay OFF in these scenarios:

**Test 1: NVS Corruption on Boot**
- Corrupt NVS config (flip random bits in config_crc field)
- Reboot ESP32-C3
- Verify relay stays OFF
- Verify error LED double blink

**Test 2: BLE Init Failure**
- Mock `esp_ble_gap_register_callback()` to return error
- Reboot ESP32-C3
- Verify relay stays OFF
- Verify error LED triple blink

**Test 3: Watchdog Timeout (Firmware Hang)**
- Inject infinite loop in main task
- Wait 10 seconds for watchdog trigger
- Verify relay turns OFF within 1 second
- Verify firmware resets

**Test 4: Power Loss During Relay ON**
- Trigger sensor to energize relay
- Cut power (VCC disconnect) while relay active
- Restore power
- Verify relay OFF on boot

**Test 5: Division by Zero Crash**
- Inject division by zero in timer callback
- Verify relay OFF after crash reset
- Verify crash logged to serial

**Test 6: Null Pointer Dereference**
- Inject null pointer access in BLE callback
- Verify relay OFF after crash reset

**Test 7: Stack Overflow**
- Inject recursive function call to overflow stack
- Verify relay OFF after crash reset

**Test 8: Invalid Timer Config (Negative Value)**
- Attempt `SET_TIMER -10`
- Verify command rejected with ERROR response
- Verify relay behavior unchanged

**Test 9: Invalid MAC Address**
- Attempt `REGISTER_SENSOR INVALID_MAC BUTTON`
- Verify command rejected with ERROR response
- Verify relay behavior unchanged

**Test 10: Config Validation Failure**
- Manually corrupt NVS timer_seconds to 9999
- Reboot ESP32-C3
- Verify relay stays OFF due to validation failure

**When** all fault injection tests complete
**Then** relay MUST be OFF in 10/10 scenarios (100% per NFR1)

**And** test report generated showing:
- Scenario description
- Expected result: Relay OFF
- Actual result: [PASS/FAIL]
- Serial log excerpt

**And** validates NFR1 (relay OFF in 100% of error conditions), FR34, FR36, FR37, FR38

## Epic 5: Event Logging & Diagnostics

Users can troubleshoot system behavior and monitor sensor health.

### Story 5.1: Implement Event Log Buffer (Last 10 Trigger Events)

As a user,
I want to view the last 10 sensor trigger events with timestamps,
So that I can understand relay activation history and troubleshoot unexpected behavior.

**Acceptance Criteria:**

**Given** firmware with relay control from Epic 4A
**When** event log buffer is implemented
**Then** firmware maintains circular buffer in RAM:
- Capacity: 10 events
- Fields per event: timestamp (ms), sensor_mac, event_type, battery_level, rssi

**When** registered sensor triggers relay
**Then** event logged to circular buffer with:
- Timestamp: `esp_timer_get_time() / 1000` (milliseconds since boot)
- sensor_mac: MAC address from BLE packet
- event_type: Button press type, Motion, or Door state
- battery_level: Battery percentage (0-100%)
- rssi: Signal strength in dBm

**When** buffer reaches capacity (10 events)
**Then** oldest event is overwritten (FIFO behavior)

**When** serial command `GET_EVENTS` is received
**Then** firmware responds with last 10 events (newest first):
```
OK|events|10
1. [timestamp_ms] MAC=[AA:BB:CC:DD:EE:FF] EVENT=[single_press] BATT=[85%] RSSI=[-65dBm]
2. [timestamp_ms] MAC=[AA:BB:CC:DD:EE:FF] EVENT=[single_press] BATT=[84%] RSSI=[-67dBm]
...
```

**And** validates FR22 (buffer last 10 trigger events), FR23 (retrieve event log)

### Story 5.2: Implement Error Log & Diagnostics

As a developer,
I want to retrieve error logs showing NVS failures and BLE init failures,
So that I can troubleshoot system issues in the field.

**Acceptance Criteria:**

**Given** firmware with error handling from previous epics
**When** error log buffer is implemented
**Then** firmware maintains error log in RAM:
- Capacity: 10 most recent errors
- Fields: timestamp, error_code, error_message

**When** NVS failure occurs (CRC mismatch, read/write error)
**Then** error logged with:
- timestamp (ms since boot)
- error_code: "NVS_CRC_FAIL" or "NVS_READ_FAIL"
- error_message: Human-readable description

**When** BLE initialization fails
**Then** error logged with:
- timestamp
- error_code: "BLE_INIT_FAIL"
- error_message: ESP-IDF error description

**When** serial command `GET_ERRORS` is received
**Then** firmware responds with last 10 errors (newest first):
```
OK|errors|3
1. [timestamp_ms] CODE=[NVS_CRC_FAIL] MSG=[Config corrupted, CRC mismatch]
2. [timestamp_ms] CODE=[BLE_INIT_FAIL] MSG=[esp_ble_gap_register_callback failed]
3. [timestamp_ms] CODE=[NVS_READ_FAIL] MSG=[Failed to read sensor_mac from NVS]
```

**And** validates FR24 (log errors), FR25 (retrieve error log)

### Story 5.3: Comprehensive STATUS Command Implementation

As a user,
I want to query comprehensive system status via STATUS command,
So that I can see all critical information in one place.

**Acceptance Criteria:**

**Given** firmware with all previous epic functionality
**When** serial command `STATUS` is received
**Then** firmware responds within 2 seconds (NFP4) with JSON-formatted status:
```json
{
  "state": "LISTENING",
  "relay": "OFF",
  "timer_remaining_sec": 0,
  "sensor": {
    "registered": true,
    "mac": "AA:BB:CC:DD:EE:FF",
    "type": "BUTTON",
    "battery": 85,
    "last_seen_sec_ago": 12,
    "rssi": -65,
    "in_range": true
  },
  "config": {
    "timer_duration_sec": 10,
    "retrigger_mode": "EXTEND"
  },
  "firmware": {
    "version": "1.0.0",
    "boot_reason": "POWER_ON"
  },
  "errors": {
    "count": 0,
    "last_error": null
  }
}
```

**When** sensor has NOT been seen recently (>60 seconds)
**Then** `sensor.in_range` = false

**When** errors exist in error log
**Then** `errors.count` shows total errors, `errors.last_error` shows most recent

**And** validates FR26 (comprehensive system status), FR39-FR41 (firmware version, boot reason), NFU3 (status answers all key questions), NFP4 (response within 2 seconds)

### Story 5.4: Firmware Version & Boot Reason Tracking

As a developer,
I want firmware version and boot reason accessible via STATUS command,
So that I can track deployed firmware versions and diagnose unexpected resets.

**Acceptance Criteria:**

**Given** firmware with STATUS command from Story 5.3
**When** firmware is compiled
**Then** version string defined in firmware:
- Format: "MAJOR.MINOR.PATCH" (e.g., "1.0.0")
- Defined in main/CMakeLists.txt or version.h header
- Accessible via `get_firmware_version()` function

**When** firmware boots
**Then** firmware calls `esp_reset_reason()` to determine boot reason:
- ESP_RST_POWERON: Power-on reset
- ESP_RST_SW: Software reset
- ESP_RST_PANIC: Exception/panic reset
- ESP_RST_WDT: Watchdog timer reset
- Other reasons per ESP-IDF documentation

**And** boot reason logged to serial on startup

**When** STATUS command is received
**Then** firmware version and boot reason included in response (per Story 5.3 JSON format)

**And** validates FR39 (firmware update via standard ESP-IDF tools - `idf.py flash`), FR40 (firmware version accessible), FR41 (boot reason logged)

## Epic 6: Python CLI Tool - Core Commands [DEFERRED TO V1.1]

> **⚠️ DEFERRED:** This epic has been postponed to V1.1 per Sprint Change Proposal (2026-01-27).
>
> **Rationale:** The firmware serial protocol (Epic 5) provides complete command access via any serial terminal. The CLI adds convenience (auto-port detection, Rich formatting, auto-generated help) but is not essential for MVP value delivery.
>
> **User Access:** Until CLI is available, users can access all commands via serial terminal (PuTTY, screen, minicom) at 115200 baud. See serial protocol documentation for command reference.

Users can configure and control the relay module via professional CLI interface.

### Story 6.1: CLI Project Structure with Typer + Rich Frameworks

As a CLI developer,
I want a well-structured Python CLI project using Typer and Rich,
So that I have a professional foundation for all CLI commands.

**Acceptance Criteria:**

**Given** Python 3.8+ development environment
**When** CLI project is initialized
**Then** project structure follows Python best practices:
```
cli/
├── pyproject.toml (or setup.py)
├── README.md
├── relay_cli/
│   ├── __init__.py
│   ├── main.py (Typer app entry point)
│   ├── serial_protocol.py (serial communication)
│   ├── formatters.py (Rich table/console formatting)
│   └── commands/
│       ├── __init__.py
│       ├── status.py
│       ├── register_sensor.py
│       ├── set_timer.py
│       ├── test_relay.py
│       └── logs.py
```

**And** dependencies specified in pyproject.toml:
- typer[all]
- rich
- pyserial

**And** CLI installable via `pip install -e .` for development

**And** validates architecture requirement (Python CLI with Typer + Rich), NFC5 (Python 3.8+)

### Story 6.2: Serial Port Auto-Detection with Manual Fallback

As a user,
I want the CLI to auto-detect my ESP32-C3's serial port,
So that I don't have to manually specify the port every time.

**Acceptance Criteria:**

**Given** ESP32-C3 connected via USB
**When** CLI command is executed without `--port` argument
**Then** CLI scans for serial ports using `serial.tools.list_ports`

**And** filters for ESP32-C3 compatible devices:
- CP2102 USB-to-UART bridge (VID:PID detection)
- CH340 USB-to-UART bridge
- Native USB CDC on ESP32-C3

**When** single matching port found
**Then** CLI auto-selects that port and displays:
```
Auto-detected port: /dev/ttyUSB0
```

**When** multiple matching ports found
**Then** CLI displays list and prompts user to select:
```
Multiple ports found:
1. /dev/ttyUSB0 (CP2102)
2. /dev/ttyUSB1 (CH340)
Select port (1-2):
```

**When** no matching ports found
**Then** CLI displays error with actionable guidance:
```
ERROR: No ESP32-C3 found. Please check:
- USB cable is connected
- Device is powered on
- Drivers are installed (CP2102 or CH340)
Use --port /dev/ttyUSB0 to specify manually
```

**And** validates FR27 (auto-detect with manual fallback), NFU2 (error messages with actionable guidance)

### Story 6.3: Implement Core CLI Commands (status, register-sensor, set-timer, test-relay, clear-sensor, logs)

As a user,
I want comprehensive CLI commands for all configuration and control operations,
So that I can manage the relay module from my computer.

**Acceptance Criteria:**

**Given** CLI with serial communication from Story 6.2
**When** CLI commands are implemented using Typer
**Then** all commands follow this pattern:
```python
@app.command()
def command_name(
    port: str = typer.Option(None, help="Serial port"),
    # ... other options
):
    """Command description."""
    # Implementation
```

**Command: `relay-cli status`**
- Sends `STATUS` command to firmware
- Parses JSON response
- Displays formatted table using Rich:
```
┏━━━━━━━━━━━━━━━━━━┳━━━━━━━━━━━━━━━━━━━━━━━━┓
┃ Field            ┃ Value                  ┃
┡━━━━━━━━━━━━━━━━━━╇━━━━━━━━━━━━━━━━━━━━━━━━┩
│ State            │ LISTENING              │
│ Relay            │ OFF                    │
│ Sensor MAC       │ AA:BB:CC:DD:EE:FF      │
│ Sensor Type      │ BUTTON                 │
│ Battery          │ 85% ✓                  │
│ RSSI             │ -65 dBm (Excellent)    │
│ Timer Duration   │ 10 seconds             │
│ Retrigger Mode   │ EXTEND                 │
│ Firmware Version │ 1.0.0                  │
└──────────────────┴────────────────────────┘
```

**Command: `relay-cli register-sensor AA:BB:CC:DD:EE:FF BUTTON`**
- Sends `REGISTER_SENSOR` command
- Validates MAC format client-side before sending
- Displays success/error with Rich formatting

**Command: `relay-cli set-timer 60`**
- Sends `SET_TIMER 60` command
- Validates range (1-600) client-side
- Confirms timer updated

**Command: `relay-cli set-retrigger EXTEND`**
- Sends `SET_RETRIGGER EXTEND` command
- Validates mode (EXTEND/IGNORE)

**Command: `relay-cli test-relay on`**
- Sends `TEST_RELAY ON` command
- Displays relay state change

**Command: `relay-cli clear-sensor`**
- Sends `CLEAR_SENSOR` command
- Confirms sensor cleared

**Command: `relay-cli logs events`**
- Sends `GET_EVENTS` command
- Displays formatted event table

**Command: `relay-cli logs errors`**
- Sends `GET_ERRORS` command
- Displays formatted error table

**And** validates FR27-FR32, FR29 (colorful formatted output), FR30 (comprehensive help via Typer auto-generation)

## Epic 7: Documentation & Educational Quality

Developers can understand, extend, and maintain the firmware codebase.

### Story 7.1: Create Architecture Documentation with Diagrams

As a developer,
I want comprehensive architecture documentation with diagrams,
So that I can understand system design at a glance.

**Acceptance Criteria:**

**Given** completed firmware from all previous epics
**When** architecture documentation is created
**Then** `docs/architecture.md` includes:

**State Machine Diagram:**
- Visual representation of UNCONFIGURED/LEARNING/LISTENING/ACTIVE states
- Transition conditions between states
- LED patterns for each state
- Created using Mermaid, PlantUML, or Excalidraw

**Serial Protocol Specification:**
- Command format: `COMMAND [ARG1] [ARG2]\n`
- Response formats: `OK|data\n` and `ERROR|code|message\n`
- Complete command reference table with examples

**NVS Schema Documentation:**
- Table showing all NVS keys, types, ranges, defaults
- CRC32 calculation explanation
- Example config hex dump

**Component Architecture:**
- Diagram showing ESP-IDF components:
  - bthome_parser
  - nvs_storage
  - relay_control
  - serial_protocol
- Dependencies between components

**And** validates FR43 (architecture docs with diagrams), NFM2 (docs include state machine, serial protocol, NVS schema)

### Story 7.2: Write Extension Guide for Adding New Sensor Types

As a developer,
I want a clear guide for adding new sensor types,
So that I can extend the system to support additional BTHome sensors.

**Acceptance Criteria:**

**Given** completed firmware with BTHome parser from Epic 2
**When** extension guide is created
**Then** `docs/adding-sensors.md` includes step-by-step instructions:

**Step 1: Identify BTHome Object ID**
- Reference BTHome v2 specification
- Find Object ID for new sensor type (e.g., 0x05 for Illuminance)

**Step 2: Create Sensor Handler**
- Code template for new handler function
- Example: `sensor_illuminance.c` implementation

**Step 3: Register Handler in BTHome Parser**
- Where to add `bthome_register_handler()` call
- Example registration code

**Step 4: Update NVS Schema (if needed)**
- When to add new sensor_type enum value
- How to maintain backwards compatibility

**Step 5: Add CLI Support**
- Update `REGISTER_SENSOR` command validation
- Update STATUS command display logic

**Complete Working Example:**
- Full code for adding Illuminance sensor support
- Unit test example
- Expected time to complete: ≤4 hours per NFM3

**And** validates FR44 (extension guide), NFM3 (add new sensor type in ≤4 hours)

### Story 7.3: Add Inline Comments for BTHome Parsing & State Transitions

As a developer,
I want inline comments explaining non-obvious code,
So that I can understand complex logic without extensive debugging.

**Acceptance Criteria:**

**Given** firmware codebase from all previous epics
**When** code review for comments is performed
**Then** BTHome parsing code includes comments:
- Packet structure explanation (Device Info byte, OLV triplets)
- Bit manipulation for encryption/trigger flags
- Object ID lookup table
- Handler registry implementation

**Example BTHome Parser Comments:**
```c
// BTHome v2 Device Info byte (first byte after UUID):
// Bit 0: Encryption flag (0=unencrypted, 1=encrypted)
// Bit 1: Trigger flag (0=normal, 1=button/motion trigger)
// Bits 2-7: Reserved (must be 0)
uint8_t device_info = adv_data[0];
bool encrypted = (device_info & 0x01);  // Check bit 0
bool trigger = (device_info & 0x02);    // Check bit 1
```

**State Transition Comments:**
```c
// State transition: LISTENING → ACTIVE
// Trigger: Registered sensor BLE packet received
// Actions: Energize relay, start timer, LED solid ON
if (state == STATE_LISTENING && mac_matches_registered()) {
    relay_set_state(true);  // Energize relay
    timer_start(config.timer_seconds);  // Start countdown
    led_set_pattern(LED_SOLID_ON);  // Visual feedback
    state = STATE_ACTIVE;
}
```

**NVS Code Comments:**
- CRC32 calculation explanation
- Atomic write pattern
- Corruption detection logic

**Watchdog Comments:**
- TWDT initialization parameters
- Reset frequency requirement
- Panic behavior on timeout

**And** validates FR45 (inline comments for BTHome parsing, state transitions), NFM4 (non-obvious code has explanatory comments)

### Story 7.4: Implement CLI Auto-Generated Help System [NOT APPLICABLE - V1.1]

> **⚠️ NOT APPLICABLE:** This story depends on Epic 6 (Python CLI Tool) which has been deferred to V1.1.
>
> This story will be implemented when Epic 6 is developed in V1.1.

~~As a user,
I want comprehensive auto-generated help for all CLI commands,
So that I can learn how to use the CLI without reading separate documentation.~~

**Deferred to:** V1.1 (when Epic 6 CLI is implemented)

**Acceptance Criteria:**

**Given** CLI with all commands from Epic 6
**When** Typer framework is used correctly
**Then** CLI provides auto-generated help at multiple levels:

**Top-level help: `relay-cli --help`**
```
Usage: relay-cli [OPTIONS] COMMAND [ARGS]...

ESP32C3 Relay Module CLI

Commands:
  status          Query comprehensive system status
  register-sensor Register sensor by MAC address
  set-timer       Configure relay-on duration (1-600 seconds)
  set-retrigger   Set retrigger mode (EXTEND or IGNORE)
  test-relay      Manually control relay (ON/OFF)
  clear-sensor    Clear registered sensor
  logs            View event logs or error logs
```

**Command-specific help: `relay-cli set-timer --help`**
```
Usage: relay-cli set-timer [OPTIONS] SECONDS

Configure relay-on duration (1-600 seconds)

Arguments:
  SECONDS  Timer duration in seconds (1-600)  [required]

Options:
  --port TEXT  Serial port (auto-detected if not specified)
  --help       Show this message and exit

Examples:
  relay-cli set-timer 30        # Set 30-second timer
  relay-cli set-timer 600       # Set maximum 10-minute timer
```

**Each command includes:**
- Description of what it does
- Argument/option details with types and constraints
- Usage examples (minimum 2 per command)
- Clear error messages per NFU2

**And** validates FR46 (CLI auto-generated help with examples), FR30 (comprehensive help), NFU4 (every command has help with description, constraints, examples)

### Story 7.5: Firmware Readability Review & Refinement

As a developer,
I want clean, readable firmware code,
So that the codebase meets the 4.0/5.0 readability target.

**Acceptance Criteria:**

**Given** completed firmware from all previous epics
**When** readability review is performed by 3+ developers
**Then** firmware achieves ≥4.0/5.0 average readability rating

**Readability Checklist:**
- ✓ Consistent naming conventions (snake_case for C, clear variable names)
- ✓ Functions ≤50 lines (complex functions broken into helpers)
- ✓ Clear function names (e.g., `relay_energize_with_timer()` vs `do_stuff()`)
- ✓ Comments explain "why", code explains "what"
- ✓ No magic numbers (use #define or const for all constants)
- ✓ Error handling consistent (`esp_err_t` return values, ESP_ERROR_CHECK)
- ✓ Indentation and formatting consistent (4 spaces, no tabs)

**Example Refactoring:**
```c
// BEFORE (poor readability):
void f(int x) {
    if(x>0){gpio_set_level(7,1);vTaskDelay(x*1000);gpio_set_level(7,0);}
}

// AFTER (high readability):
void relay_energize_with_timer(uint16_t duration_seconds) {
    relay_set_state(true);  // Energize relay
    vTaskDelay(pdMS_TO_TICKS(duration_seconds * 1000));  // Wait for timer
    relay_set_state(false);  // De-energize relay
}
```

**Validation Process:**
1. 3+ developers independently rate code sections on 1-5 scale
2. Calculate average rating
3. If <4.0, refactor low-scoring sections and re-review
4. Iterate until ≥4.0 achieved

**And** validates FR42 (firmware achieves 4.0/5.0 readability), NFM1 (≥4.0/5.0 from 3+ developers)
