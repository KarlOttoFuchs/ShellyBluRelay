---
stepsCompleted: [1, 2, 3, 4, 5]
inputDocuments:
  - '/Users/karlfuchs/Documents/Development/Projects/ESP32C3 Relay Module/_bmad-output/planning-artifacts/prd.md'
workflowType: 'architecture'
project_name: 'ESP32C3 Relay Module'
user_name: 'Karl'
date: '2026-01-13'
---

# Architecture Decision Document

_This document builds collaboratively through step-by-step discovery. Sections are appended as we work through each architectural decision together._

## Project Context Analysis

### Requirements Overview

**Functional Requirements (56 Total):**

This is a dual-stack IoT/embedded system with 56 functional requirements organized into clear architectural domains:

- **Hardware Layer (FR1-FR6)**: GPIO relay control, button input, dual LED feedback (white status, red error), USB serial communication, dual power supply (USB-C 5V OR external DC 12-24V with Schottky isolation)

- **BLE Communication (FR7-FR11)**: Passive BLE scanning for BTHome v2 advertising packets from Shelly BLU sensors (Button, Motion, Door/Window), MAC filtering, RSSI monitoring, 24-hour sensor timeout detection

- **Sensor Management (FR12-FR16)**: Trigger-based learning (press button → sensor triggers → auto-register), manual MAC entry fallback, single sensor NVS storage, configuration persistence

- **Relay Control Logic (FR17-FR21)**: Event-triggered relay energization, configurable countdown timer (1-600s, ±1s accuracy), retriggering modes (extend/ignore), persistent timer configuration

- **Diagnostics (FR22-FR26)**: 10-event circular buffer with timestamps, error logging (NVS failures, BLE init, sensor timeouts), comprehensive status reporting with RSSI visibility

- **CLI Interface (FR27-FR32)**: Cross-platform Python tool with auto-port detection, text-based serial protocol (115200 baud), Rich-formatted output (tables, colors, progress), auto-generated help, manual relay testing

- **State Machine & Safety (FR33-FR38)**: Configuration/Listening/Active states, **relay defaults OFF on boot/error/crash** (safety-critical), watchdog recovery, NVS corruption detection, serial crash logs with stack traces

- **Firmware Lifecycle (FR39-FR41)**: Standard esptool flashing, version reporting, boot reason logging

- **Educational Quality (FR42-FR46)**: 4.0/5.0 code readability target, architecture diagrams (state machine, serial protocol, NVS schema), extension guide for new sensor types, inline comments explaining BTHome parsing and state transitions

**Non-Functional Requirements:**

Critical NFRs driving architectural decisions:

- **Performance (NFP1-NFP5)**: 500ms relay response latency, ±1 second timer accuracy over 10 minutes, 10 packets/sec BLE processing, 2-second CLI response, 5-second boot time

- **Reliability (NFR1-NFR5)**: **100% fail-safe relay-OFF on ANY error** (safety-critical), 10-second watchdog recovery, 10,000 power cycle durability, 95% sensor detection at -70 dBm RSSI, 168-hour continuous operation

- **Usability (NFU1-NFU5)**: 80% first-time users succeed in <10 minutes, actionable error messages, comprehensive single-command status, auto-generated help with examples, distinguishable LED patterns

- **Maintainability (NFM1-NFM4)**: 4.0/5.0 firmware readability from 3+ reviewers, complete architecture documentation (state machine, serial protocol, NVS schema), <4 hour sensor extension development time, inline comment coverage for BTHome parsing and state logic

- **Compatibility (NFC1-NFC5)**: Windows/macOS/Linux CLI support, auto-detection of CP2102/CH340 bridges, ESP-IDF 5.1.x/5.2.x compatibility, Shelly BLU Button/Motion/Door sensors, Python 3.8+

**Scale & Complexity:**

- **Primary domain**: IoT/Embedded firmware + Developer CLI tool
- **Complexity level**: Low-Medium (focused MVP with deliberate constraints)
- **Estimated architectural components**: 8 major firmware modules + 1 CLI application
  - Firmware: BTHome parser, BLE scanner, state machine, NVS storage, relay controller, timer manager, serial protocol handler, LED controller
  - CLI: Serial communication layer, command handlers, Rich UI formatter

### Technical Constraints & Dependencies

**Platform Constraints:**
- ESP32-C3 (RISC-V, 160MHz, 400KB SRAM, 4MB flash) - resource-constrained embedded environment
- ESP-IDF 5.x framework (FreeRTOS, NVS library, BLE stack)
- BTHome v2 protocol standard (fixed advertising packet format)
- Shelly BLU sensor compatibility (Button, Motion, Door/Window devices)

**MVP Scoping Constraints:**
- **ONE sensor maximum** (enforced in firmware and CLI) - proves concept before scaling
- Text-based serial protocol (115200 baud, newline-terminated) - debuggable with standard tools
- Manual firmware flashing via esptool (no OTA in MVP)
- No network connectivity (WiFi, MQTT deferred to V2.0+)

**Hardware Design Reference:**
- Complete KiCad 9 schematic at `hardware/ESP32C3-Relay-Module-Rev-A/`
- Dual power with Schottky diode isolation (USB-C 5V OR external DC 12-24V)
- DC relay rated for 12-24V loads (LED lighting primary use case)

### Cross-Cutting Concerns Identified

**Safety & Fail-Safe Behavior:**
- Relay MUST default OFF on boot, firmware crash, watchdog reset, NVS corruption, BLE init failure - affects ALL components
- Watchdog timer must reset MCU within 10 seconds if firmware hangs
- Every state transition must validate relay safety before proceeding

**Serial Protocol Design:**
- Firmware ↔ CLI communication contract defines command format, response structure, error codes
- Text-based for debuggability (PuTTY, screen, minicom compatibility)
- JSON responses for structured data (status queries)
- Extensibility required for V1.2+ features (config export/import, factory reset)

**State Machine Reliability:**
- Clean transitions between Configuration, Listening, Active states
- Error recovery paths must always return relay to OFF state
- NVS state persistence must survive power interruptions

**Persistent Storage Integrity:**
- NVS writes must be durable across 10,000 power cycles (<0.1% corruption)
- Corrupted NVS must be detected and prevent unsafe relay operation
- Factory reset recovery path (V1.3) requires graceful degradation

**Error Handling & Diagnostics:**
- Self-service troubleshooting requirement (Marcus's RSSI debugging, Sofia's validation workflow)
- Actionable error messages in CLI
- Visual LED feedback for non-CLI diagnostics
- Serial crash logs with stack traces for developer debugging (Priya's extension work)

**Educational Transparency:**
- Code clarity affects architecture design (must favor readability over cleverness)
- Extension points must be explicit (new sensor type addition in <4 hours)
- Documentation must be architectural (not just API reference)

## Starter Template Evaluation

### Primary Technology Domain

**Dual-Stack IoT/Embedded System** - This project combines:
1. **Embedded Firmware**: ESP32-C3 (C/ESP-IDF)
2. **Developer CLI Tool**: Python 3.8+ (Typer + Rich)

This is NOT a typical web/mobile application that would benefit from standard starters (Next.js, T3, etc.). The technology stack is already explicitly defined in the PRD based on platform constraints and research findings.

### Starter Options Considered

**ESP-IDF Firmware Project:**
- **Approach**: Standard ESP-IDF project initialization using `idf.py create-project`
- **Rationale**: ESP-IDF provides official project scaffolding with proper CMake structure, component organization, and build system integration
- **No Custom Starter Needed**: BTHome relay controller firmware is domain-specific; generic ESP-IDF structure is appropriate

**Python CLI Tool:**
- **Approach**: Manual Python package setup with `pyproject.toml` (modern Python packaging standard)
- **Rationale**: Typer + Rich are already chosen based on research; cookiecutter templates would add unnecessary abstraction
- **Structure**: Standard Python package layout (`src/` layout or flat layout with `relay_cli/` module)

### Selected Approach: Standard Project Initialization (No Template)

**Rationale for No Starter Template:**

This project has explicit, well-researched technology choices that don't align with generic starters:

1. **Firmware Stack Pre-Determined**: ESP32-C3 platform dictates ESP-IDF framework; no alternatives considered
2. **CLI Stack Pre-Determined**: Research report validated Typer + Rich as best-in-class for professional Python CLIs
3. **Educational Transparency Requirement**: Custom project structure optimized for code clarity (NFM1: 4.0/5.0 readability target) matters more than starter conventions
4. **Dual-Stack Complexity**: No existing starter template handles firmware + CLI tool coordination

**Initialization Approach:**

**Firmware Project Setup:**

```bash
# ESP-IDF project initialization
idf.py create-project esp32c3-relay-firmware

# Project structure will follow ESP-IDF conventions:
# - main/ (application code)
# - components/ (reusable modules: bthome_parser, state_machine, nvs_storage, etc.)
# - CMakeLists.txt (build configuration)
# - sdkconfig (ESP-IDF configuration)
```

**CLI Tool Setup:**

```bash
# Python package initialization (manual setup)
mkdir relay-cli
cd relay-cli

# Create pyproject.toml with:
# - dependencies: typer[all], rich, pyserial
# - entry point: relay-cli command
# - Python 3.8+ requirement

# Package structure:
# - src/relay_cli/ (application code)
#   - __main__.py (CLI entry point)
#   - commands/ (status, register_sensor, set_timer, etc.)
#   - serial_protocol.py (firmware communication)
#   - formatters.py (Rich table/color formatting)
# - tests/ (pytest test suite)
# - README.md
# - pyproject.toml
```

**Architectural Decisions Established by This Approach:**

**Language & Runtime:**
- **Firmware**: C language (C11 standard), ESP-IDF 5.x (compatible with 5.1.x and 5.2.x LTS)
- **CLI**: Python 3.8+ (ensures wide compatibility across modern systems)
- **Build System**: ESP-IDF CMake (firmware), pip/setuptools (CLI)

**Code Organization:**
- **Firmware**: ESP-IDF component-based architecture
  - `main/` for application entry point and state machine
  - `components/bthome_parser/` for BTHome v2 packet decoding (reusable, testable)
  - `components/nvs_storage/` for persistent configuration (isolates NVS complexity)
  - `components/relay_control/` for GPIO and timer management
  - `components/serial_protocol/` for USB serial communication handling
- **CLI**: Standard Python package layout
  - `src/relay_cli/commands/` for command modules (one file per command)
  - `src/relay_cli/serial_protocol.py` for firmware communication contract
  - `src/relay_cli/formatters.py` for Rich UI formatting (separation of concerns)

**Testing Framework:**
- **Firmware**: Hardware validation via serial commands and physical sensor triggers (no unit tests - embedded code requires hardware execution)
- **CLI**: pytest (Python standard, runs locally without hardware)
- **Integration**: End-to-end testing with physical hardware

**Development Experience:**
- **Firmware**: ESP-IDF toolchain (`idf.py build`, `idf.py flash`, `idf.py monitor`)
- **CLI**: Standard Python dev workflow (`pip install -e .`, pytest, black formatter, mypy type checking)
- **Documentation**: Inline comments (BTHome parsing, state transitions) + architecture diagrams (state machine, serial protocol, NVS schema)

**Deployment & Distribution:**
- **Firmware**: Binary releases on GitHub (`.bin` files for esptool flashing)
- **CLI**: PyPI package (`pip install relay-cli`)

**Note:** Project initialization should be the first implementation task:
1. Initialize ESP-IDF firmware project with component structure
2. Initialize Python CLI package with Typer + Rich dependencies
3. Define serial protocol specification (command format, response structure, error codes)
4. Set up CI/CD for both firmware and CLI builds

## Core Architectural Decisions

### Decision Priority Analysis

**Critical Decisions (Block Implementation):**
1. Serial Protocol Specification - Firmware ↔ CLI communication contract
2. State Machine Design - Safety-critical relay control logic
3. NVS Storage Schema - Configuration persistence structure
4. BTHome Parser Architecture - Sensor data extraction and extensibility
5. Error Handling Strategy - Fail-safe behavior patterns
6. Logging & Diagnostics - Event tracking and troubleshooting

**PRD Corrections Identified:**
- **Removed**: 24-hour sensor timeout (FR11, FR21) - flawed assumption for vacation/idle scenarios
- **Added**: Battery-based sensor health monitoring from BTHome packets

### Decision 1: Serial Protocol Specification

**Protocol Format: Text Commands + JSON Responses**

**Command Format:**
```
COMMAND [ARG1] [ARG2] ...\n
```

**Response Format:**
```
OK|{json_data}\n          # Success with structured data
OK|simple_response\n      # Success with simple text
ERROR|code|message\n      # Error with code and description
```

**Example Session:**
```
> STATUS\n
< OK|{"state":"listening","sensor":"AA:BB:CC:DD:EE:FF","rssi":-45,"timer":30,"battery":87}\n

> TEST_RELAY ON\n
< OK|relay_on\n

> SET_TIMER 60\n
< OK|timer_set\n

> INVALID_CMD\n
< ERROR|invalid_command|Unknown command. Type HELP for available commands.\n
```

**Command Set (MVP):**
- `STATUS` - Returns JSON with state, sensor info, RSSI, battery%, events
- `TEST_RELAY [ON|OFF]` - Manual relay control
- `REGISTER_SENSOR [MAC]` - Manual sensor registration (fallback mode)
- `CLEAR_SENSOR` - Unregister sensor
- `SET_TIMER [seconds]` - Configure relay duration (1-600)
- `SET_RETRIGGER [extend|ignore]` - Configure retriggering behavior
- `GET_LOGS` - Return last 10 events
- `PING` - Connectivity test (returns OK|pong)
- `HELP` - List available commands

**Error Codes:**
- `invalid_command` - Unknown command
- `invalid_argument` - Argument out of range/malformed
- `nvs_failure` - NVS corruption detected
- `no_sensor` - Command requires registered sensor
- `sensor_low_battery` - Battery <20% warning

**Rationale:**
- Text commands debuggable with PuTTY, screen, minicom
- JSON responses provide structured data for CLI parsing
- Newline-terminated for simple readline() parsing
- Extensible for V1.2+ features (config export/import, factory reset)

**Physical Layer:**
- Baud rate: 115200 (8N1)
- USB-C with CP2102/CH340 bridge
- No flow control needed

### Decision 2: State Machine Design

**States:**

```
┌─────────────────┐
│  BOOT/RESET     │ (Relay OFF - always)
└────────┬────────┘
         │
         ▼
┌─────────────────┐     Button press (30s window)
│ CONFIGURATION   │◄────────────────────────────────┐
│ (Relay OFF)     │                                 │
│ - LED: Fast     │    No sensor in NVS             │
│   blink         │    OR clear-sensor cmd          │
└────────┬────────┘                                 │
         │                                          │
         │ Sensor registered                        │
         │ (from NVS or pairing)                   │
         ▼                                          │
┌─────────────────┐                                 │
│  LISTENING      │                                 │
│ (Relay OFF)     │                                 │
│ - LED: Slow     │                                 │
│   blink         │    CLEAR_SENSOR command         │
│ - BLE scanning  ├─────────────────────────────────┘
│ - Monitor sensor│
└────────┬────────┘
         │
         │ Valid sensor trigger
         │ (button, motion, door event)
         ▼
┌─────────────────┐
│    ACTIVE       │
│ (Relay ON)      │
│ - LED: Solid    │
│ - Timer running │    Timer expires (always)
│ - Retriggering  ├──────────┐
│   logic active  │          │
└─────────────────┘          │
                             ▼
                       LISTENING (Relay OFF)

ERROR TRANSITIONS (from any state → safe state):
- NVS corruption → CONFIGURATION (Relay OFF)
- BLE init failure → CONFIGURATION (Relay OFF)
- Watchdog timeout → BOOT (Relay OFF)
- Firmware panic → BOOT (Relay OFF)
```

**State Transition Rules:**

1. **BOOT → CONFIGURATION:**
   - No sensor registered in NVS, OR
   - NVS corrupted (CRC check failed)

2. **BOOT → LISTENING:**
   - Valid sensor found in NVS
   - BLE initialized successfully

3. **CONFIGURATION → LISTENING:**
   - Sensor successfully registered (trigger-based OR manual MAC entry)
   - Configuration saved to NVS

4. **LISTENING → ACTIVE:**
   - Registered sensor triggers event
   - Event type matches sensor capability

5. **ACTIVE → LISTENING:**
   - Timer expires (ONLY way to exit ACTIVE)
   - Relay forced OFF

6. **LISTENING → CONFIGURATION:**
   - `CLEAR_SENSOR` command received
   - Sensor config erased from NVS

**Retriggering Modes (within ACTIVE state):**
- **Extend mode**: New trigger → reset timer, stay ACTIVE
- **Ignore mode**: New trigger → ignored, stay ACTIVE until timer expires

**Safety-Critical Rule:**
ALL error conditions → Relay OFF, no exceptions (NFR1 compliance)

**LED Feedback Patterns:**
- **Fast blink** (500ms): CONFIGURATION mode
- **Slow blink** (2s): LISTENING mode
- **Solid ON**: ACTIVE mode (relay energized)
- **Error patterns**: Single blink (battery low), double blink (NVS failure), triple blink (BLE init failure)

**Pairing Window:**
- 30 seconds (hardcoded for MVP)
- Triggered by physical button press

**Rationale:**
- Simple, auditable state machine (educational requirement NFM1)
- Every state explicitly defines relay position
- Error recovery always returns to safe state (relay OFF)
- LED patterns provide non-CLI diagnostics

### Decision 3: NVS Storage Schema

**NVS Namespace:** `relay_config`

**Persisted Data (Infrequent Writes):**

```c
// Key-value pairs stored in NVS flash:
// - "sensor_mac"     : string  (AA:BB:CC:DD:EE:FF format, max 18 bytes)
// - "sensor_type"    : uint8_t (0=none, 1=button, 2=motion, 3=door)
// - "timer_seconds"  : uint16_t (1-600 range)
// - "retrigger_mode" : uint8_t (0=extend, 1=ignore)
// - "config_version" : uint8_t (schema version = 1, for future migrations)
// - "config_crc"     : uint32_t (CRC32 checksum for integrity validation)
```

**RAM-Only Data (NOT Persisted):**

```c
// Volatile data updated frequently, stored in RAM:
// - battery_pct    : uint8_t (0-100%, from BLE packets)
// - last_rssi      : int8_t  (-128 to 0 dBm, signal strength)
// - last_seen_ms   : uint32_t (uptime timestamp)
```

**Factory Defaults (First Boot or NVS Corruption):**
- sensor_mac = "" (empty, no sensor registered)
- sensor_type = 0 (none)
- timer_seconds = 30
- retrigger_mode = 0 (extend)
- config_version = 1
- battery_pct = 255 (unknown/not applicable)

**Write Strategy:**
- Atomic write with immediate commit
- Validate by reading back
- Update CRC after successful write
- On read: validate CRC → if mismatch, log error, use factory defaults, enter CONFIGURATION state

**Write Frequency Analysis:**
- Sensor registration: ~10 writes over device lifetime
- Timer changes: ~20-50 writes (user tweaking)
- Retrigger mode: ~5-10 writes
- **Total: <100 writes lifetime** (well within 10,000 cycle durability NFR3)

**Corruption Detection:**
- CRC32 checksum validation on every read
- Corrupted NVS → factory defaults + CONFIGURATION state entry
- Error logged to event buffer + serial output

**Rationale:**
- Separate volatile (RAM) from persistent (NVS) data
- Battery level changes too frequently for NVS (flash wear prevention)
- CRC integrity checking prevents unsafe relay operation from corrupted config
- Factory reset path (V1.3) built into corruption recovery

### Decision 4: BTHome Parser Architecture

**Architecture: Event-Driven with Handler Registry**

**Component Structure:**

```
components/bthome_parser/
├── bthome_parser.c        # Core packet decoder + handler registry
├── bthome_parser.h        # Public API + extension interface
├── sensor_button.c        # Button event handler (single/double/triple press)
├── sensor_motion.c        # Motion detection handler
├── sensor_door.c          # Door/window open/close handler
└── CMakeLists.txt
```

**Handler Registry Pattern:**

```c
typedef struct {
    uint8_t sensor_type;           // SENSOR_BUTTON, SENSOR_MOTION, etc.
    const char* name;              // "Shelly BLU Button"
    parse_handler_fn parse_event;  // Function pointer to event parser
} sensor_handler_t;

// Extensible registry - add new handlers without modifying core
sensor_handler_t handlers[] = {
    {SENSOR_BUTTON, "Shelly BLU Button", parse_button_event},
    {SENSOR_MOTION, "Shelly BLU Motion", parse_motion_event},
    {SENSOR_DOOR,   "Shelly BLU Door",   parse_door_event},
    // Priya adds: {SENSOR_TEMP_HUM, "Shelly BLU H&T", parse_temp_humidity_event}
};
```

**Parser Flow:**

1. BLE advertising packet received
2. Validate BTHome v2 service UUID (0xFCD2)
3. Extract device MAC, sensor type, battery %, RSSI
4. Look up handler in registry by sensor type
5. Call handler to parse event data
6. Return parsed event to state machine

**Extension Process (for Priya's temperature sensor):**

1. Define `SENSOR_TEMP_HUMIDITY = 4` in header
2. Create `sensor_temp_humidity.c` with parse handler
3. Register handler in `bthome_parser.c` registry
4. **Estimated time: <4 hours** (meets NFM3 requirement)

**BTHome v2 Packet Parsing:**

```c
// Service Data structure:
// [device_info, object_id, value_bytes...]
// - Device info: encryption flag, MAC
// - Object ID: 0x3A (button), 0x21 (motion), 0x1A (door contact)
// - Value: Event data (press count, motion bool, door state)
```

**Rationale:**
- Handler registry provides clear extension points (educational requirement)
- Separate files per sensor type improves readability (NFM1: 4.0/5.0 target)
- Function pointers enable runtime flexibility without switch-case sprawl
- Matches component-based ESP-IDF architecture

### Decision 5: Error Handling Strategy

**Firmware Error Classification:**

**1. Fatal Errors (Relay OFF → System Reset):**
- BLE initialization failure
- Watchdog timeout (firmware hang)
- Unrecoverable NVS corruption

**Handler Pattern:**
```c
if (ble_init() != ESP_OK) {
    ESP_LOGE(TAG, "BLE init failed - FATAL");
    relay_force_off();
    set_error_led_pattern(ERROR_FATAL);
    esp_restart();  // Watchdog ensures recovery
}
```

**2. Recoverable Errors (Relay OFF → Continue Running):**
- BLE packet parsing failure (malformed packet)
- Invalid serial command
- Sensor trigger with no registered sensor
- NVS write failure (retry possible)

**Handler Pattern:**
```c
if (parse_packet(data) != ESP_OK) {
    ESP_LOGW(TAG, "Packet parse failed - ignored");
    relay_force_off();  // Safety first
    log_to_event_buffer(ERROR_PARSE_FAILED);
    // Continue scanning
}
```

**3. Warnings (System Continues, User Notified):**
- Sensor battery <20%
- Weak RSSI (<-70 dBm)
- Config value out of range (use default)

**Handler Pattern:**
```c
if (battery_pct < 20) {
    ESP_LOGW(TAG, "Battery low: %d%%", battery_pct);
    set_warning_led_pattern(WARN_BATTERY_LOW);
    // Continue operation
}
```

**CLI Error Presentation (Rich-formatted):**

```python
try:
    response = serial.send_command("STATUS")
except SerialException:
    console.print("[red]✗ Connection failed[/red]")
    console.print("\nTroubleshooting:")
    console.print("  1. Check USB cable connected")
    console.print("  2. Verify ESP32-C3 has power")
    console.print("  3. Try: relay-cli --port /dev/ttyUSB0 status")
    raise typer.Exit(1)

if response.startswith("ERROR"):
    code, message = parse_error(response)
    console.print(f"[yellow]⚠ {message}[/yellow]")
    if code == "no_sensor":
        console.print("Run: relay-cli register-sensor")
```

**Serial Protocol Error Codes:**

```
ERROR|invalid_command|Unknown command 'FOO'. Type HELP for commands.
ERROR|invalid_argument|Timer 999 out of range (1-600 seconds).
ERROR|nvs_failure|Config corrupted. Using factory defaults.
ERROR|no_sensor|No sensor registered. Run register-sensor first.
ERROR|sensor_low_battery|Battery at 15%. Replace sensor soon.
```

**Actionable Error Messages (NFU2 Requirement):**
- Every error includes "what to do next"
- CLI suggests specific commands to resolve issue
- Status command provides diagnostic context

**Firmware Logging Strategy:**
- ESP-IDF standard macros: `ESP_LOGI`, `ESP_LOGW`, `ESP_LOGE`
- Log levels: INFO (state transitions), WARN (battery/RSSI), ERROR (failures)
- Serial output visible in `idf.py monitor`
- Automatic stack traces on panic (ESP-IDF built-in, FR36)

**Safety-Critical Pattern:**
```c
// ALWAYS force relay OFF before handling any error
relay_force_off();
// Then handle error logging, LED patterns, recovery
```

**Rationale:**
- Tiered error handling matches risk levels
- Every error path guarantees relay-OFF (NFR1 compliance)
- Actionable CLI messages enable self-service troubleshooting (NFU2)
- Standard ESP-IDF logging for educational clarity

### Decision 6: Logging & Diagnostics Architecture

**Event Buffer Design (FR22 - Last 10 Trigger Events):**

```c
typedef struct {
    uint32_t timestamp_ms;  // Uptime in milliseconds (wraps at 49 days, acceptable for MVP)
    uint8_t event_type;     // BUTTON_PRESS, MOTION_DETECTED, DOOR_OPEN, etc.
    int8_t rssi;            // Signal strength at trigger time
    uint8_t battery_pct;    // Battery % at trigger time
} event_record_t;

// Circular buffer in RAM (volatile, lost on reboot - acceptable for diagnostics)
event_record_t event_buffer[10];
uint8_t event_write_index = 0;  // Newest overwrites oldest
```

**Battery Monitoring (Revised from PRD):**

```c
// Track battery level from BTHome packets (NOT persisted to NVS)
uint8_t current_battery_pct = 255;  // 255 = unknown/not yet seen
int8_t current_rssi = 0;
uint32_t last_seen_ms = 0;

// Warning thresholds (checked on every BLE packet):
if (battery_pct < 20) {
    set_warning_led_pattern(WARN_BATTERY_LOW);
}
if (battery_pct < 10) {
    set_error_led_pattern(ERROR_BATTERY_CRITICAL);
}

// NO 24-hour timeout logic (removed from PRD):
// - Sensor may not trigger for weeks (vacation scenario)
// - Battery level is authoritative health indicator
```

**CLI Status Command Output (Rich Table):**

```python
┌──────────────────────────────────────────────┐
│ ESP32C3 Relay Module Status                  │
├──────────────────────────────────────────────┤
│ State:        Listening                      │
│ Relay:        OFF                            │
│ Timer:        30 seconds                     │
│ Retrigger:    Extend                         │
├──────────────────────────────────────────────┤
│ Sensor:       Shelly BLU Motion              │
│ MAC:          AA:BB:CC:DD:EE:FF              │
│ Battery:      87% ✓                          │
│ RSSI:         -52 dBm (Good)                 │
│ Last seen:    12 seconds ago                 │
├──────────────────────────────────────────────┤
│ Recent Events (last 10):                     │
│  1. Motion detected   | -50 dBm | 2s ago     │
│  2. Motion detected   | -48 dBm | 35s ago    │
│  3. Motion detected   | -51 dBm | 1m 12s ago │
│  ...                                         │
└──────────────────────────────────────────────┘
```

**RSSI Interpretation (Marcus's Troubleshooting Workflow):**

```python
# Color-coded RSSI feedback in CLI:
if rssi > -50:
    color = "green"; label = "Excellent"
elif rssi > -60:
    color = "green"; label = "Good"
elif rssi > -70:
    color = "yellow"; label = "Fair - Consider closer placement"
else:
    color = "red"; label = "Weak - Move module closer to sensor"
```

**Firmware Crash Logs (Priya's Debugging Workflow):**

```
ESP-IDF panic handler (automatic):
Guru Meditation Error: Core 0 panic'ed (LoadProhibited)
Backtrace: 0x4008XXXX:0x3FFCXXXX bthome_parser.c:142
           0x4009XXXX:0x3FFCXXXX main.c:87

# Stack trace points to exact line for debugging
# Available via idf.py monitor
```

**Diagnostic Data Available:**
- Current state (CONFIGURATION/LISTENING/ACTIVE)
- Relay position (ON/OFF)
- Sensor info (MAC, type, battery%, RSSI, last seen)
- Configuration (timer, retrigger mode)
- Recent events (10-entry circular buffer)
- Error log (last 10 errors with timestamps)

**Rationale:**
- Uptime-based timestamps acceptable for MVP (RTC not required)
- Battery monitoring replaces flawed 24-hour timeout logic
- RSSI visibility enables Marcus's range debugging
- Rich table format provides comprehensive single-command diagnostics (NFU3)
- Crash logs support Priya's extension development

### Decision Impact Analysis

**PRD Corrections Required:**
1. **Remove FR11**: "System can detect when registered sensor has not been seen for 24 hours"
2. **Remove FR21** (partial): Delete 24-hour timeout, keep battery monitoring
3. **Update NFR4**: Remove 24-hour detection language
4. **Add to Battery Monitoring**: Track battery % from BTHome packets, warn at <20%, error at <10%

**Implementation Sequence (Dependency Order):**

1. **Serial Protocol** (foundation for CLI communication)
   - Define command/response format
   - Implement firmware serial handler
   - Implement CLI protocol layer

2. **NVS Storage** (required by state machine)
   - Schema implementation
   - CRC validation
   - Factory defaults

3. **BTHome Parser** (required by BLE scanning)
   - Core packet decoder
   - Sensor handlers (button, motion, door)
   - Battery extraction

4. **State Machine** (core firmware logic)
   - State transitions
   - Relay control integration
   - Error recovery paths

5. **Error Handling** (cross-cutting)
   - Error classification
   - LED patterns
   - Event logging

6. **CLI Commands** (user interface)
   - Status command (comprehensive output)
   - Configuration commands
   - Diagnostic commands

**Cross-Component Dependencies:**

- **State Machine ↔ NVS Storage**: State machine reads sensor config from NVS on boot
- **State Machine ↔ BTHome Parser**: Parser feeds events to state machine for relay decisions
- **Serial Protocol ↔ State Machine**: Commands modify state, status queries read state
- **Error Handling → All Components**: Every component must follow fail-safe relay-OFF pattern
- **Logging ↔ State Machine**: Event buffer tracks state transitions and trigger events

**Critical Path for MVP:**
1. Hardware validation (Priority #0 - gates all work)
2. Serial protocol + basic state machine (enables CLI testing)
3. NVS + BTHome parser (enables sensor registration)
4. Complete state machine + relay control (enables end-to-end trigger workflow)
5. CLI polish + diagnostics (enables user success metrics)

## Implementation Patterns & Consistency Rules

### Pattern Categories Defined

**Critical Conflict Points Identified:** 9 areas where AI agents could make different implementation choices that would break compatibility

This section defines mandatory patterns that ALL AI agents MUST follow to ensure code consistency and interoperability between firmware and CLI components.

### Naming Patterns

**Firmware Code Naming (C/ESP-IDF Convention):**

```c
// Functions: snake_case with verb_noun pattern
esp_err_t bthome_parse_packet(const uint8_t *data, size_t len);
void relay_force_off(void);
esp_err_t nvs_save_config(const sensor_config_t *config);

// Types: snake_case_t suffix
typedef struct {
    uint8_t sensor_type;
    char mac_address[18];
    uint16_t timer_seconds;
} sensor_config_t;

typedef enum {
    STATE_BOOT,
    STATE_CONFIGURATION,
    STATE_LISTENING,
    STATE_ACTIVE
} system_state_t;

// Constants/Defines: UPPER_SNAKE_CASE with descriptive prefixes
#define GPIO_RELAY_PIN 5
#define GPIO_BUTTON_PIN 9
#define GPIO_LED_STATUS_PIN 8
#define GPIO_LED_ERROR_PIN 10

#define BLE_SCAN_DURATION_MS 10000
#define NVS_NAMESPACE "relay_config"
#define SERIAL_BAUD_RATE 115200

#define TIMER_MIN_SECONDS 1
#define TIMER_MAX_SECONDS 600
#define TIMER_DEFAULT_SECONDS 30

// Component names: lowercase, underscores
// components/bthome_parser/
// components/nvs_storage/
// components/relay_control/
// components/serial_protocol/

// File names: snake_case.c/.h
// bthome_parser.c, bthome_parser.h
// sensor_button.c, sensor_motion.c, sensor_door.c
// state_machine.c, relay_control.c

// ESP-IDF log tags: UPPER_CASE, component-specific
static const char *TAG = "BTHOME_PARSER";
static const char *TAG = "STATE_MACHINE";
static const char *TAG = "RELAY_CTRL";
```

**CLI Code Naming (Python PEP 8):**

```python
# Commands: dash-separated (Typer convention, maps to CLI)
@app.command("status")           # relay-cli status
@app.command("register-sensor")  # relay-cli register-sensor
@app.command("set-timer")        # relay-cli set-timer 60
@app.command("test-relay")       # relay-cli test-relay on

# Functions: snake_case with verb_noun pattern
def send_command(cmd: str, timeout: float = 2.0) -> str:
def parse_status_response(json_str: str) -> dict:
def format_status_table(status_data: dict) -> Table:
def auto_detect_port() -> Optional[str]:

# Classes: PascalCase
class SerialProtocol:
class StatusFormatter:
class CommandError(Exception):

# Constants: UPPER_SNAKE_CASE
DEFAULT_BAUD_RATE = 115200
COMMAND_TIMEOUT = 2.0
MAX_RETRIES = 3

# Files: snake_case.py
# serial_protocol.py
# formatters.py
# commands/status.py
# commands/register_sensor.py
# commands/set_timer.py
```

**Serial Protocol Naming (CRITICAL - Contract Between Firmware & CLI):**

```
# Commands: UPPER_CASE, no underscores in command names (space-separated args)
STATUS\n
PING\n
HELP\n
TEST_RELAY ON\n
TEST_RELAY OFF\n
SET_TIMER 60\n
SET_RETRIGGER extend\n
SET_RETRIGGER ignore\n
REGISTER_SENSOR AA:BB:CC:DD:EE:FF\n
CLEAR_SENSOR\n
GET_LOGS\n

# Responses: OK|data\n or ERROR|code|message\n
OK|pong\n
OK|relay_on\n
OK|timer_set\n
OK|{"state":"listening","sensor":"AA:BB:CC:DD:EE:FF"}\n
ERROR|invalid_command|Unknown command\n
ERROR|invalid_argument|Value out of range\n

# MAC Address Format: AA:BB:CC:DD:EE:FF (uppercase hex, colon-separated)
# State Names in JSON: lowercase (configuration, listening, active)
# Error Codes: snake_case (invalid_command, nvs_failure, no_sensor)
```

**Rationale:**
- Firmware follows ESP-IDF coding standards (educational requirement)
- CLI follows Pythonic conventions (PEP 8)
- Serial protocol uses unambiguous, debuggable format
- MAC addresses uppercase for consistency with BLE spec
- Clear separation prevents cross-language naming confusion

### Structure Patterns

**Firmware Project Organization (ESP-IDF Standard):**

```
firmware/
├── main/
│   ├── main.c                    # Application entry point, state machine
│   ├── state_machine.c/.h        # State transition logic
│   ├── led_control.c/.h          # LED pattern management
│   └── CMakeLists.txt
├── components/
│   ├── bthome_parser/
│   │   ├── bthome_parser.c/.h    # Core parser + registry
│   │   ├── sensor_button.c       # Button event handler
│   │   ├── sensor_motion.c       # Motion event handler
│   │   ├── sensor_door.c         # Door event handler
│   │   └── CMakeLists.txt
│   ├── nvs_storage/
│   │   ├── nvs_storage.c/.h      # NVS read/write/CRC
│   │   └── CMakeLists.txt
│   ├── relay_control/
│   │   ├── relay_control.c/.h    # GPIO + timer management
│   │   └── CMakeLists.txt
│   └── serial_protocol/
│       ├── serial_protocol.c/.h  # Command parser + response builder
│       └── CMakeLists.txt
├── CMakeLists.txt
├── sdkconfig                     # ESP-IDF configuration
└── README.md
```

**CLI Project Organization (Python Package Standard):**

```
relay-cli/
├── src/
│   └── relay_cli/
│       ├── __init__.py
│       ├── __main__.py           # CLI entry point (Typer app)
│       ├── serial_protocol.py    # Serial communication layer
│       ├── formatters.py         # Rich table/color formatting
│       └── commands/
│           ├── __init__.py
│           ├── status.py         # status command
│           ├── register_sensor.py
│           ├── set_timer.py
│           ├── set_retrigger.py
│           ├── test_relay.py
│           └── clear_sensor.py
├── tests/
│   ├── test_serial_protocol.py
│   ├── test_formatters.py
│   └── test_commands/
│       ├── test_status.py
│       └── ...
├── pyproject.toml                # Modern Python packaging
├── README.md
└── .gitignore
```

**Test Organization:**
- **Firmware**: Hardware validation via serial commands and physical sensor triggers (no unit test directory)
- **CLI**: pytest tests in `tests/` mirroring `src/` structure
- **Integration**: End-to-end testing with physical hardware

**Rationale:**
- ESP-IDF component architecture enables reusability
- Firmware validation requires hardware execution - traditional unit tests don't apply
- Python `src/` layout prevents import conflicts
- CLI test structure mirrors source structure for discoverability

### Format Patterns

**Serial Protocol Response Formats (CRITICAL):**

```
Success Responses:
- Simple: OK|text_response\n
  Example: OK|pong\n
          OK|relay_on\n
          OK|timer_set\n

- JSON: OK|{json_object}\n
  Example: OK|{"state":"listening","rssi":-45}\n

Error Responses:
- ERROR|error_code|Human-readable message.\n
  Example: ERROR|invalid_command|Unknown command 'FOO'. Type HELP for available commands.\n
          ERROR|invalid_argument|Timer value 999 out of range (1-600 seconds).\n
          ERROR|nvs_failure|Configuration corrupted. Using factory defaults.\n

Status JSON Response Format:
{
  "state": "listening",              // configuration|listening|active
  "relay": "off",                    // on|off
  "timer": 30,                       // seconds (1-600)
  "retrigger": "extend",             // extend|ignore
  "sensor_mac": "AA:BB:CC:DD:EE:FF", // uppercase, colon-separated
  "sensor_type": "motion",           // button|motion|door
  "battery": 87,                     // 0-100%, 255=unknown
  "rssi": -52,                       // dBm signal strength
  "last_seen_sec": 12,               // seconds ago
  "events": [                        // Last 10 events
    {"type": "motion_detected", "rssi": -50, "battery": 87, "ago_sec": 2},
    {"type": "motion_detected", "rssi": -48, "battery": 87, "ago_sec": 35}
  ]
}
```

**CLI Output Formatting (Rich Tables):**

```python
# Status command: Always use Rich Table
table = Table(title="ESP32C3 Relay Module Status", box=box.ROUNDED)
table.add_column("Property", style="cyan")
table.add_column("Value", style="white")

# RSSI color coding (consistent interpretation):
if rssi > -50:
    rssi_color = "green"
    rssi_label = "Excellent"
elif rssi > -60:
    rssi_color = "green"
    rssi_label = "Good"
elif rssi > -70:
    rssi_color = "yellow"
    rssi_label = "Fair"
else:
    rssi_color = "red"
    rssi_label = "Weak"

# Battery color coding:
if battery >= 50:
    battery_color = "green"
elif battery >= 20:
    battery_color = "yellow"
else:
    battery_color = "red"

# Error messages: Always use [red] for errors, [yellow] for warnings
console.print("[red]✗ Connection failed[/red]")
console.print("[yellow]⚠ Battery low (15%)[/yellow]")
console.print("[green]✓ Sensor registered[/green]")
```

**Data Type Formats:**
- **Timestamps**: uint32_t milliseconds (uptime) in firmware, human-readable ("12s ago") in CLI
- **MAC Addresses**: AA:BB:CC:DD:EE:FF (uppercase, colon-separated) everywhere
- **Booleans**: C: true/false, Python: True/False, JSON: true/false
- **State Names**: lowercase strings (configuration, listening, active)

**Rationale:**
- Pipe-delimited format simple to parse, debug-friendly
- JSON for structured data (status queries)
- Consistent color coding aids rapid troubleshooting

### Communication Patterns

**Error Handling Patterns:**

**Firmware Error Handling (Consistent Across All Components):**

```c
// ALWAYS check return codes with ESP_ERROR_CHECK or explicit if
esp_err_t result = nvs_save_config(&config);
if (result != ESP_OK) {
    ESP_LOGE(TAG, "NVS save failed: %s", esp_err_to_name(result));
    relay_force_off();  // SAFETY FIRST - always force relay OFF on error
    set_error_led_pattern(ERROR_NVS_FAILURE);
    log_to_event_buffer(EVENT_ERROR_NVS_FAILURE);
    return result;
}

// CRITICAL: relay_force_off() MUST be called before any error handling
// Pattern for all error paths:
// 1. Force relay OFF
// 2. Set LED error pattern
// 3. Log to event buffer
// 4. Return error or restart (if fatal)

// Fatal error pattern (BLE init, watchdog timeout):
if (ble_init() != ESP_OK) {
    ESP_LOGE(TAG, "BLE init FATAL");
    relay_force_off();
    set_error_led_pattern(ERROR_FATAL);
    esp_restart();  // System reset, watchdog ensures recovery
}

// Recoverable error pattern (bad packet, invalid command):
if (parse_packet(data) != ESP_OK) {
    ESP_LOGW(TAG, "Packet parse failed - ignoring");
    relay_force_off();  // Safety first, but continue running
    // No restart, continue scanning
}
```

**CLI Error Handling (Typer + Rich):**

```python
# Always catch serial exceptions and provide actionable guidance
try:
    response = serial.send_command("STATUS", timeout=2.0)
except SerialException as e:
    console.print("[red]✗ Connection failed[/red]")
    console.print("\nTroubleshooting:")
    console.print("  1. Check USB cable is connected")
    console.print("  2. Verify ESP32-C3 module has power (LED should blink)")
    console.print("  3. Try specifying port: relay-cli --port /dev/ttyUSB0 status")
    console.print(f"\nError details: {e}")
    raise typer.Exit(1)

# Parse firmware error responses and provide next steps
if response.startswith("ERROR"):
    parts = response.split("|", 2)
    error_code = parts[1] if len(parts) > 1 else "unknown"
    error_msg = parts[2].strip() if len(parts) > 2 else "Unknown error"

    console.print(f"[yellow]⚠ {error_msg}[/yellow]")

    # Actionable guidance based on error code
    if error_code == "no_sensor":
        console.print("\nTo register a sensor:")
        console.print("  relay-cli register-sensor")
    elif error_code == "invalid_argument":
        console.print("\nCheck command syntax:")
        console.print("  relay-cli --help")

    raise typer.Exit(1)
```

**Rationale:**
- Consistent error handling prevents relay staying ON during failures (safety-critical)
- Actionable CLI messages enable self-service troubleshooting (NFU2)
- ESP_LOG macros provide visibility in `idf.py monitor`

### Process Patterns

**Memory Allocation Patterns (Firmware):**

```c
// Prefer stack allocation for small, fixed-size buffers
uint8_t packet_buffer[256];  // BLE packet max size
char mac_str[18];            // MAC address string

// Use heap only for large or variable-sized data
uint8_t *large_buffer = malloc(size);
if (large_buffer == NULL) {
    ESP_LOGE(TAG, "Malloc failed");
    relay_force_off();
    return ESP_ERR_NO_MEM;
}
// Always free heap memory
free(large_buffer);

// Static allocation for global state (state machine, event buffer)
static event_record_t event_buffer[10];
static system_state_t current_state = STATE_BOOT;
```

**FreeRTOS Task Priorities (Firmware):**

```c
// Safety-critical tasks: Higher priority
#define TASK_PRIORITY_RELAY_CONTROL   (tskIDLE_PRIORITY + 4)  // Highest - relay control
#define TASK_PRIORITY_STATE_MACHINE   (tskIDLE_PRIORITY + 3)  // State transitions
#define TASK_PRIORITY_BLE_SCAN        (tskIDLE_PRIORITY + 2)  // BLE scanning
#define TASK_PRIORITY_SERIAL_PROTOCOL (tskIDLE_PRIORITY + 1)  // CLI communication
```

**Logging Patterns:**

**Firmware Logging (ESP-IDF):**

```c
// Use appropriate log levels:
ESP_LOGI(TAG, "State transition: %s -> %s", old_state, new_state);  // Informational
ESP_LOGW(TAG, "Battery low: %d%%", battery_pct);                    // Warning
ESP_LOGE(TAG, "NVS read failed: %s", esp_err_to_name(result));      // Error

// Include context in log messages:
ESP_LOGI(TAG, "Sensor registered: %s (type=%d)", mac, sensor_type);
ESP_LOGW(TAG, "RSSI weak: %d dBm (threshold -70)", rssi);

// Use consistent TAG naming: component name in UPPER_CASE
static const char *TAG = "BTHOME_PARSER";
```

**CLI Logging (Python):**

```python
# Use Rich console for all user-facing output
console.print("[green]✓ Sensor registered successfully[/green]")
console.print(f"[yellow]⚠ Battery at {battery}%[/yellow]")

# Use standard logging for debug output (optional --verbose flag)
import logging
logger = logging.getLogger("relay_cli")
logger.debug(f"Sent command: {cmd}")
logger.debug(f"Received response: {response}")
```

**Testing Patterns:**

**Firmware Validation (Hardware-Based):**

Firmware is validated through direct hardware testing:
- Serial commands (`STATUS`, `TEST_RELAY`, etc.) verify functionality
- Physical sensor triggers validate BLE parsing and event handling
- Hardware observation confirms LED patterns and relay operation

No unit test framework - embedded code requires hardware execution for meaningful validation.

**CLI Testing (pytest):**

```python
# Test file naming: test_<module>.py
# tests/test_serial_protocol.py

def test_parse_status_response():
    json_str = '{"state":"listening","rssi":-45}'
    status = parse_status_response(json_str)

    assert status["state"] == "listening"
    assert status["rssi"] == -45

# Run tests: pytest tests/
```

### Enforcement Guidelines

**ALL AI Agents implementing this project MUST:**

1. **Follow exact naming conventions** specified above for firmware (C), CLI (Python), and serial protocol
2. **Use the serial protocol format EXACTLY** - no deviations (OK|data\n or ERROR|code|message\n)
3. **Call `relay_force_off()` BEFORE any error handling** in firmware (safety-critical requirement)
4. **Use ESP-IDF log macros** (ESP_LOGI/W/E) in firmware, Rich console in CLI - no printf/print
5. **Persist ONLY configuration to NVS** - NEVER persist battery_pct, rssi, last_seen (flash wear)
6. **Use uppercase MAC addresses** with colon separators (AA:BB:CC:DD:EE:FF) everywhere
7. **Follow ESP-IDF component structure** for firmware modules (components/<name>/)
8. **Use Typer dash-separated commands** for CLI (register-sensor, NOT register_sensor)
9. **Return esp_err_t** from all firmware functions that can fail, check with ESP_ERROR_CHECK or explicit if
10. **Provide actionable error messages** in CLI - always tell user what to do next (NFU2 requirement)

**Pattern Enforcement:**

- **Code Review**: Check naming, error handling patterns, serial protocol format
- **Testing**: Unit tests validate component interfaces, integration tests validate serial protocol
- **Documentation**: Architecture diagrams (state machine, serial protocol, NVS schema) in docs/
- **Pattern Violations**: Document in architecture.md updates, discuss in team review

**Updating Patterns:**

If architectural patterns need to change (e.g., V1.2+ features):
1. Update this section in architecture.md with rationale
2. Notify all active development agents
3. Add migration guide if breaking change
4. Update tests to validate new patterns

### Pattern Examples

**Good Example - Firmware Function:**

```c
// ✓ CORRECT: Follows all patterns
esp_err_t nvs_save_sensor_config(const sensor_config_t *config) {
    if (config == NULL) {
        ESP_LOGE(TAG, "Config is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS open failed: %s", esp_err_to_name(err));
        relay_force_off();  // Safety first
        return err;
    }

    err = nvs_set_str(handle, "sensor_mac", config->mac_address);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS set failed: %s", esp_err_to_name(err));
        nvs_close(handle);
        relay_force_off();  // Safety first
        return err;
    }

    err = nvs_commit(handle);
    nvs_close(handle);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Config saved: %s", config->mac_address);
    }

    return err;
}
```

**Good Example - CLI Command:**

```python
# ✓ CORRECT: Follows all patterns
@app.command("register-sensor")
def register_sensor(
    mac: Optional[str] = typer.Argument(None, help="Sensor MAC address (AA:BB:CC:DD:EE:FF)")
):
    """Register a BLE sensor with the relay module."""
    console = Console()

    try:
        protocol = SerialProtocol()

        if mac:
            # Manual registration with MAC
            if not validate_mac_format(mac):
                console.print("[red]✗ Invalid MAC format. Use AA:BB:CC:DD:EE:FF[/red]")
                raise typer.Exit(1)

            response = protocol.send_command(f"REGISTER_SENSOR {mac.upper()}")
        else:
            # Trigger-based pairing
            console.print("Press the physical button on the module to enter pairing mode...")
            console.print("Then trigger your sensor (press button, wave hand, etc.)")
            response = protocol.send_command("REGISTER_SENSOR")

        if response.startswith("OK"):
            console.print("[green]✓ Sensor registered successfully[/green]")
        elif response.startswith("ERROR"):
            parts = response.split("|", 2)
            error_msg = parts[2] if len(parts) > 2 else "Registration failed"
            console.print(f"[red]✗ {error_msg}[/red]")
            raise typer.Exit(1)

    except SerialException as e:
        console.print("[red]✗ Connection failed[/red]")
        console.print("Check USB cable and module power")
        raise typer.Exit(1)
```

**Anti-Patterns (DO NOT DO THIS):**

```c
// ✗ WRONG: Inconsistent naming, no error handling, no safety check
void SaveConfig(SensorConfig* cfg) {  // ✗ CamelCase function name
    NVS_Set("mac", cfg->macAddr);     // ✗ No error check, wrong naming
    // ✗ Missing relay_force_off() on error
    // ✗ Missing ESP_LOG* messages
}

// ✗ WRONG: Using printf instead of ESP_LOG
printf("Error: NVS failed\n");  // ✗ Use ESP_LOGE(TAG, "...")

// ✗ WRONG: Not checking return codes
nvs_set_str(handle, "sensor_mac", mac);  // ✗ Ignored return value
nvs_commit(handle);  // ✗ Ignored return value
```

```python
# ✗ WRONG: Wrong command naming, poor error handling
@app.command("register_sensor")  # ✗ Should be "register-sensor"
def register(mac):  # ✗ Missing type hints
    print("Registering...")  # ✗ Should use console.print with colors
    serial.write(f"REG {mac}")  # ✗ Wrong command format (should be REGISTER_SENSOR)
    # ✗ No error handling
    # ✗ No actionable guidance on failure
```

**Rationale:**
- Good examples demonstrate ALL patterns working together
- Anti-patterns show common mistakes that break consistency
- Examples serve as templates for AI agents implementing features
