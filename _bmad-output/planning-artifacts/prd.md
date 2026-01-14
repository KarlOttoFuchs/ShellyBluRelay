---
stepsCompleted: ['step-01-init', 'step-02-discovery', 'step-03-success', 'step-04-journeys', 'step-05-domain', 'step-06-innovation', 'step-07-project-type', 'step-08-scoping', 'step-09-functional', 'step-10-nonfunctional', 'step-11-polish', 'step-e-01-discovery', 'step-e-02-review', 'step-e-03-edit']
inputDocuments:
  - '/Users/karlfuchs/Documents/Development/Projects/ESP32C3 Relay Module/docs/brief.md'
  - '/Users/karlfuchs/Documents/Development/Projects/ESP32C3 Relay Module/docs/brainstorming-session-results.md'
  - '/Users/karlfuchs/Documents/Development/Projects/ESP32C3 Relay Module/docs/RESEARCH-REPORT-Python-CLI-Frameworks.md'
briefCount: 1
researchCount: 1
brainstormingCount: 1
projectDocsCount: 0
workflowType: 'prd'
workflow: 'edit'
classification:
  projectType: 'iot_embedded'
  secondaryType: 'cli_tool'
  domain: 'home_automation_iot'
  complexity: 'low_medium'
  projectContext: 'greenfield'
lastEdited: '2026-01-14'
editHistory:
  - date: '2026-01-14'
    changes: 'Fixed FR11 - replaced 24-hour sensor timeout with battery-based health monitoring per Architecture correction. Updated FR4, MVP list, error codes, and technical sections for consistency.'
---

# Product Requirements Document - ESP32C3 Relay Module

**Author:** Karl
**Date:** 2026-01-13

## Executive Summary

The ESP32C3 Relay Module is a safety-first automation controller that connects Shelly BLU Bluetooth sensors to relay-switched loads, eliminating network-dependency friction in home automation installations. **Target users** include DIY enthusiasts seeking rapid prototyping, professional installers requiring bulk deployment efficiency, and embedded systems learners studying production-quality firmware. **Core value proposition**: USB-only configuration via Python CLI enables 2-minute first success for beginners and <6 minutes per module bulk deployment for professionals, while trigger-based sensor learning eliminates manual MAC address entry. The system prioritizes fail-safe operation with explicit relay-off guarantees on any error condition.

This greenfield IoT/embedded project delivers both firmware (ESP32-C3, BLE, BTHome v2 protocol) and companion CLI tool (Python, cross-platform serial communication). Hardware validation, persistent storage, and extensible architecture enable this to serve as a reference implementation for embedded learners while meeting professional installer deployment needs.

## Success Criteria

### User Success

**DIY Home Automation Enthusiast:**
- **"Got it working, yeah!" moment**: User sees relay click and LED respond within 2 minutes of USB connection (80% success rate with first-time users)
- **Self-sufficiency**: User troubleshoots sensor-not-triggering or wrong-timeout issues using `status` command without consulting external help (target: <10% need support)
- **Confidence to extend**: User feels capable of adding custom automation logic or new sensor types after reviewing firmware (measured by community fork activity)
- **Complete setup flow**: Full sensor registration, timeout configuration, and successful test completed in <10 minutes (90th percentile)

**Professional Installer/Electrician:**
- **Margin protection**: Configure 10 modules in <1 hour total using config export/import workflow (target: <6 min/module after first "golden" config)
- **Zero callbacks**: Client verification via `status` command shows exactly what's configured, preventing post-installation confusion
- **No network dependency**: Complete installation in construction sites without WiFi access
- **Predictability**: Every module behaves identically after config import - no surprises on job sites

**Embedded Systems Learner:**
- **Showcase quality**: Firmware code clarity rated 4.0/5.0 by developers reviewing codebase (survey of 5+ developers)
- **Learning outcomes**: Student can explain BTHome packet parsing and state machine design after code review
- **Portfolio value**: Project serves as reference in job applications or student portfolios (anecdotal evidence from community)
- **Extension success**: Intermediate developers successfully add support for new sensor type (temperature/humidity) within one afternoon

### Business Success

**Adoption & Community:**
- **GitHub stars**: >50 stars within 6 months (indicates community interest)
- **Documentation reach**: >500 page views/month (indicates search discovery and learning resource value)
- **Community contributions**: 2+ sensor types added by community within 12 months (indicates extensibility success)
- **Educational citations**: Firmware referenced in embedded systems courses or tutorials (qualitative validation)

**Reliability & Safety:**
- **Zero safety incidents**: Zero stuck-on relay incidents across 1,000+ cumulative operating hours (community testing reports)
- **Error rate**: <1% overall error rate in production use
- **Hardware validation**: All interfaces (LEDs, relay, button, BLE, USB) validated before architectural work begins

**User Experience Quality:**
- **CLI usability**: System Usability Scale (SUS) score >75 (above average) from 10+ users
- **Time to first success**: Median <2 minutes, 90th percentile <5 minutes (screen recording + timer measurement)
- **Self-service troubleshooting**: >80% of users resolve common errors using status/logs without external help

### Technical Success

**Firmware Quality:**
- **Timer accuracy**: Relay countdown timer accurate within ±1 second over 10-minute duration
- **Relay reliability**: <0.1% relay failure rate, zero stuck-on conditions
- **State machine reliability**: Clean state transitions, relay defaults OFF on all error conditions
- **BTHome parsing**: Correctly parses button single/double/triple press, motion detection, door open/close, lux values from real Shelly BLU devices

**CLI Quality:**
- **Auto-detection**: Serial port auto-detected on common platforms (Windows, macOS, Linux)
- **Error messaging**: Clear, actionable error messages for connection failures and configuration issues
- **Output formatting**: Colorful, professional output using Typer + Rich (tables, progress bars, status indicators)
- **Help system**: Comprehensive, auto-generated help from function docstrings

**Development Velocity:**
- **CLI development time**: 20-28 hours for complete CLI implementation (prototype to polish)
- **Hardware validation**: Priority #0 completed successfully before any architectural work begins
- **Code clarity**: 3+ developers rate firmware readability >4/5 (ease of understanding survey)

### Measurable Outcomes

**MVP Success Criteria (All Must Pass):**
1. ✅ Hardware validation complete - all interfaces confirmed working
2. ✅ End-to-end working - ONE BLU sensor triggers relay with configured timeout
3. ✅ 2-minute first success achieved - new user registers sensor and tests relay in <2 minutes
4. ✅ Self-diagnosable - status command enables troubleshooting without external help
5. ✅ Safety validated - zero stuck-on relay incidents across 100+ test cycles
6. ✅ Code clarity confirmed - 3+ developers rate firmware readability >4/5
7. ✅ Foundation for extension - architecture supports multi-sensor V1.1 without major refactoring

## Product Scope

### MVP - Minimum Viable Product

**Core Constraint:** ONE sensor maximum - enforced in firmware and CLI

**Must-Have Features:**
1. **Hardware Validation Test Suite** - Validate all interfaces (LEDs, relay, button, BLE, USB) before architectural work
2. **BTHome v2 Parser** - Parse Shelly BLU devices (Button, Motion, Door/Window) for device type, MAC, events
3. **Persistent Storage (NVS)** - Single sensor configuration + relay timer settings survive power cycles
4. **Trigger-Based Sensor Learning** - Press button → trigger sensor → auto-register with smart defaults
5. **Manual MAC Entry Fallback** - `register-sensor --mac AA:BB:CC:DD:EE:FF` when trigger-based fails
6. **Basic State Machine** - Configuration, Listening, Active states with relay-defaults-OFF safety
7. **Relay Control with Timer** - Countdown timer (1-600 seconds), relay switches OFF on expiration
8. **Configurable Retriggering** - "extend timer" vs. "ignore" modes for motion sensor use cases
9. **Event Buffer** - Last 10 trigger events with timestamps (uptime-based acceptable for MVP)
10. **Python CLI (Core Commands)** - status, test-relay, register-sensor, set-timer, set-retrigger, logs, clear-sensor
11. **Comprehensive Status Command** - Current state, sensor info, event history, error log, config summary
12. **Error LED Patterns** - Visual diagnostics (steady, slow blink, fast blink, double blink codes)
13. **Sensor Battery Health Monitoring** - Warning LED + status message when battery <20%, error at <10%

**MVP Success = Shipping Complete List Above**

**Deliberately Excluded from MVP:**
- Multi-sensor support (proves concept with one, V1.1 adds scalability)
- Config file export/import (installer workflow optimization - V1.2)
- Advanced trigger logic (lux gates, compound conditions - V1.3+)
- Alternative configuration methods (AT commands, BLE app - V2.0+)
- Network connectivity of any kind (WiFi, MQTT - V2.0+)

### Growth Features (Post-MVP)

**V1.1 - Multi-Sensor Support (2-3 weeks after MVP):**
- Support 2-5 registered sensors triggering same relay
- Enhanced CLI: `list-sensors`, `add-sensor`, `remove-sensor`
- Per-sensor trigger configuration
- Sensor registry management in NVS

**V1.2 - Configuration Management (1-2 months after MVP):**
- Config file export to YAML (`config export config.yaml`)
- Config file import from YAML (`config import config.yaml`)
- Scan-and-select registration (alternative to trigger-based)
- Configuration validation and error checking on import
- **This unlocks installer bulk deployment workflow**

**V1.3 - Advanced Automation (2-3 months after MVP):**
- Lux gate support (only trigger if below threshold - daylight-aware)
- AT-style command support for direct serial terminal access
- Factory reset command
- Enhanced error classification and logging

### Vision (Future)

**V2.0 - Connectivity & Polish (6+ months):**
- Mobile BLE app for wireless configuration (no USB required)
- Home Assistant MQTT integration
- OTA firmware updates over BLE
- Optional web interface (WiFi mode)

**Hardware Evolution:**
- PWM-capable relay module for LED dimming (requires hardware redesign)
- Multi-relay boards (2-4 relays per ESP32-C3)
- Solar-powered outdoor variant with battery management
- Weatherproof enclosure options

**Ecosystem Integration:**
- Pre-built integrations with popular home automation platforms
- Support for additional BTHome sensors beyond Shelly BLU
- Custom sensor profiles for community-contributed device types
- Video tutorial series, workshop curriculum, "Build Your Own" guidebook

## User Journeys

### Journey 1: Marcus - The Garage Lighting Problem

**Opening Scene:**

Marcus, 34, software engineer, just moved into a house with a detached garage. Every night when he takes out the trash, he fumbles with his phone flashlight while trying to unlock the garage door in the dark. His wife jokes that he's going to trip and break his neck. He's looked at commercial motion-sensor solutions - $200+ for a "smart" system that requires WiFi setup, a hub, and a monthly subscription. That's ridiculous for a garage light.

He finds a Reddit post about the ESP32C3 BLE Relay Module. Open source. No cloud. No WiFi. Just works.

**Rising Action:**

Marcus downloads the PCB files from GitHub and orders a fully assembled module from JLCPCB for $12. He also orders a Shelly BLU Motion sensor ($25). Total cost: $37.

The package arrives. He opens the box - one small PCB with ESP32-C3, relay, and screw terminals. Clean. Professional.

He plugs it into his laptop via USB-C.

He runs `pip install relay-cli` and then `relay-cli status`.

The CLI auto-detects the serial port. Green text: "✓ Connected to ESP32C3 Relay Module"

He sees a simple table showing "Relay: OFF | Sensors: None registered"

**First "Got it working!" moment (90 seconds in):**

He runs `relay-cli test-relay --on`. The relay clicks. The LED on the module lights up. He grins. It's alive.

He runs `relay-cli test-relay --off`. Click. LED off.

"Okay, now the real test."

He runs `relay-cli register-sensor`. The CLI says "Press the button on the module, then trigger your sensor within 30 seconds."

He presses the button on the relay module. The LED starts blinking.

He waves his hand in front of the Shelly BLU Motion sensor.

CLI: "✓ Sensor registered: Shelly BLU Motion (AA:BB:CC:DD:EE:FF) | Default trigger: Motion detected | Timer: 30 seconds"

**Climax:**

Marcus walks 10 feet away from the motion sensor. He walks back toward it.

The relay clicks ON. The LED lights up.

He counts: "...28, 29, 30..."

Click. Relay OFF. LED dark.

"YES."

**Resolution:**

Marcus mounts the relay module in the garage, wires it to his 12V LED strip, positions the motion sensor by the door. He tweaks the timer to 60 seconds using `relay-cli set-timer 60`.

That night, he walks to the garage to take out trash. Motion sensor detects him. Lights come on. He unlocks the door, grabs the trash bin, walks out. Sixty seconds later, lights turn off automatically.

Cost: $37. Setup time: 8 minutes. Setup complexity: ran 3 CLI commands. No WiFi. No hub. No subscription. No fumbling with his phone.

His wife notices: "Hey, the garage lights work now. Nice."

Marcus posts his build on r/homeautomation with photos and the JLCPCB order link. "Seriously, if you can run a Python script, you can do this. Total cost $37."

**Edge Case - Port Detection Failure:**

Marcus's laptop has an Arduino also plugged in. `relay-cli status` shows:

"Multiple serial ports detected:
1. /dev/ttyUSB0 - CP2102 USB to UART Bridge
2. /dev/ttyUSB1 - CH340 USB to Serial

Which port is your ESP32C3 module? (1/2)"

Marcus selects 1. CLI connects successfully. He continues setup.

**Edge Case - Sensor Not Triggering:**

Marcus registers the sensor but walks away too far - the BLE range is only ~10 meters. He comes back and waves at the sensor. Nothing happens.

He runs `relay-cli status`. The output shows:

"Sensor: Shelly BLU Motion (AA:BB:CC:DD:EE:FF)
Last seen: 45 seconds ago
RSSI: -78 dBm (weak signal)"

Marcus realizes he's too far. He moves the module 5 feet closer. Runs `relay-cli status` again:

"Sensor: Shelly BLU Motion (AA:BB:CC:DD:EE:FF)
Last seen: 2 seconds ago  
RSSI: -45 dBm (good signal)"

He waves at the sensor. Relay clicks. Problem solved.

---

### Journey 2: Sofia - The Office Renovation Job

**Opening Scene:**

Sofia, 42, runs a small electrical contracting business. She just won a bid to install motion-activated LED lighting in 12 offices across a new co-working space. The client wants simple, reliable, no monthly fees. Sofia quoted them based on installing basic motion sensors with relay modules - $180 per office including labor.

She's used commercial WiFi-based systems before. They're a nightmare on job sites - you need the building's WiFi credentials, half the time the network isn't even live yet during construction, and configuration takes 15 minutes per unit through a janky web interface.

She finds the ESP32C3 BLE Relay Module. Open source. USB configuration. Config file export/import (V1.2). No network needed.

**Rising Action:**

Sofia orders 12 assembled modules from JLCPCB ($144 total) and 12 Shelly BLU Motion sensors ($300). She brings her Windows laptop with PuTTY and the relay-cli installed.

Job site, 8 AM. The building WiFi isn't live yet - doesn't matter.

She mounts the first module, wires it to the LED ceiling panel, positions the motion sensor. Plugs USB-C cable into her laptop.

`relay-cli status` - Connected.

`relay-cli register-sensor` - Press button, wave at sensor. Registered.

`relay-cli set-timer 90` - 90 seconds for office use.

`relay-cli set-retrigger extend` - Motion resets timer.

`relay-cli test-relay --on` - Lights come on. Perfect.

She runs `relay-cli config export office-config.yaml` (V1.2 feature, post-MVP).

**Climax:**

Now she's at module #2. Instead of repeating setup:

`relay-cli config import office-config.yaml`

CLI: "Configuration imported. Relay timer: 90s | Retrigger mode: extend"

`relay-cli register-sensor` - Press button, wave at sensor for this specific office. Done.

Time: 4 minutes.

Modules 3-12: same process. Press button, trigger sensor, move on.

Total installation time: 52 minutes for all 12 offices.

**Resolution:**

Client walks through at 3 PM for inspection. Sofia shows them: "Walk into any office, lights turn on. Stand still for 90 seconds, lights turn off. Walk around, timer resets."

Client: "Perfect. How do I change the timer if we need to?"

Sofia: "I'll leave you the USB cable and a one-page instruction sheet. Run this command with whatever timeout you want."

Client is thrilled. No app. No cloud account. No WiFi setup. Just works.

Sofia made her margin - quoted 15 minutes per office, finished in 4.3 minutes average. The extra hour she saved goes straight to profit.

She recommends the module to three other contractors in her network.

**Edge Case - Config Import Validation Error:**

Module #7 - Sofia runs `relay-cli config import office-config.yaml`

CLI: "❌ Error: Configuration file format invalid. Expected schema version 1.0, found 0.9"

Sofia checks - she accidentally grabbed an old config file from a different project.

She runs `relay-cli config import office-config-v12.yaml` with the correct file.

CLI: "✓ Configuration imported successfully"

The validation saved her from misconfiguring the module.

**Edge Case - NVS Corruption Recovery:**

Module #10 - Sofia tries to register the sensor but gets:

CLI: "❌ Error: Failed to save configuration. NVS partition corrupted."

She runs `relay-cli factory-reset` (V1.3 feature, but critical for recovery).

CLI: "✓ Factory reset complete. All configuration cleared."

She reimports the config, registers the sensor. Module #10 works. Total delay: 3 minutes.

---

### Journey 3: Priya - The First IoT Project

**Opening Scene:**

Priya, 22, computer science student, just finished her embedded systems course. She understands the theory - BLE advertising, state machines, GPIO control - but she's never built anything real from scratch.

Her professor assigned a final project: "Build something with an ESP32. Demonstrate wireless communication, persistent storage, and user interaction."

She wants to build something impressive for her portfolio. Not just blink an LED. Something she can actually use and show in job interviews.

She finds the ESP32C3 BLE Relay Module on GitHub. Open firmware. Well-documented. BTHome protocol implementation as reference. Perfect learning opportunity.

**Rising Action:**

Priya clones the GitHub repo. She reads through the architecture documentation:
- `bthome_parser.c` - "Oh, this is how you decode BLE advertising packets!"
- `state_machine.c` - "Clean state transitions, exactly like the textbook said."
- `nvs_storage.c` - "So that's how ESP-IDF handles persistent data."

She orders an assembled module ($12) and a Shelly BLU Button ($20) to test with.

Module arrives. She flashes the firmware using `esptool` (she's done this in class). Plugs it in.

`relay-cli status` works. She sees her firmware running.

**Climax:**

Priya wants to extend it - add support for temperature/humidity sensors (Shelly BLU H&T).

She reads the extension guide in the documentation. It tells her exactly where to add code:
1. Update BTHome parser to recognize H&T advertising format
2. Add temperature threshold configuration
3. Extend status command to show current temperature

She spends an afternoon coding. She adds temperature parsing to `bthome_parser.c`. She tests it:

`relay-cli status` now shows: "Temperature: 22.3°C | Humidity: 45%"

She sets a threshold: "Trigger relay if temp >25°C" (could control a fan).

It works.

**Resolution:**

Priya's final project presentation:

"I built an extensible BLE relay controller. The base firmware handles motion, button, and door sensors. I extended it to support temperature sensors with threshold-based triggering. Here's the architecture diagram. Here's the state machine. Here's my code changes."

She demos it live - triggers the relay with temperature change.

Professor: "Did you write the whole firmware?"

Priya: "I studied the reference implementation and extended it. I can explain every component - BTHome parsing, NVS storage, the state machine, timer management."

She gets an A.

Two months later, in a job interview: "Tell me about a technical project you're proud of."

Priya pulls up her GitHub fork with her temperature extension. "This taught me BLE protocol parsing, ESP-IDF patterns, and state machine design. The base project showed me production-quality embedded code structure."

She gets the job.

**Edge Case - Firmware Bug Causes Crash Loop:**

Priya's first attempt at adding temperature support has a bug - she didn't bounds-check the temperature value. When the sensor reports -40°C (out of range), the firmware crashes and reboots.

She connects via USB serial, sees the crash logs:

```
Guru Meditation Error: Core 0 panic'ed (LoadProhibited)
...
Backtrace: 0x4008XXXX:0x3FFCXXXX |0x4009XXXX:0x3FFCXXXX bthome_parser.c:142
```

The stack trace points her to line 142 in her modified `bthome_parser.c`. She adds bounds checking:

```c
if (temp_raw < -4000 || temp_raw > 8000) {
    ESP_LOGW(TAG, "Temperature out of range: %d", temp_raw);
    return ESP_ERR_INVALID_ARG;
}
```

She reflashes. The firmware handles the error gracefully. Module stays online.

This debugging experience teaches her defensive programming - exactly what embedded systems require.

---

### Journey Requirements Summary

**Requirements Revealed by Marcus's Journey (DIY Enthusiast):**
- CLI auto-detection of serial port with fallback to manual selection when multiple ports detected
- Immediate hardware validation (`test-relay` command)
- Trigger-based sensor registration (press button → trigger sensor → registered)
- Clear visual feedback (relay click, LED, colorful CLI output)
- Simple timer configuration (`set-timer` command)
- RSSI visibility in status command for troubleshooting BLE range issues
- Complete setup in <10 minutes
- Beautiful CLI output using Typer + Rich
- Comprehensive help system

**Requirements Revealed by Sofia's Journey (Installer):**
- Config file export/import with schema validation (V1.2 feature - critical for bulk deployment)
- Config validation prevents misconfiguration from incompatible files
- Cross-platform CLI support (Windows with PuTTY mentioned)
- No network dependency for configuration
- Predictable behavior across modules
- Quick verification workflow (`status` command)
- Retrigger mode configuration (`set-retrigger extend`)
- Factory reset command for NVS corruption recovery (V1.3 feature, but critical)
- Client-facing documentation (one-page instruction sheet)
- <6 minute configuration per module after first

**Requirements Revealed by Priya's Journey (Learner):**
- Well-documented firmware architecture with inline comments
- Clear code structure following ESP-IDF patterns
- Extension guide documentation ("where to add code")
- Reference implementation for BTHome parsing
- Example patterns for ESP-IDF (NVS, state machines, GPIO)
- Firmware flashability using standard tools (esptool)
- Architecture diagrams and state machine documentation
- Serial crash logs with stack traces for debugging
- Bounds checking examples and defensive programming patterns
- Code clarity target: 4.0/5.0 developer rating

**Cross-Journey Capabilities Required:**
- **Firmware**: BTHome v2 parser with bounds checking, state machine with error recovery, NVS storage with corruption detection, relay control with safety defaults, timer management
- **CLI**: status (with RSSI), test-relay, register-sensor, set-timer, set-retrigger, logs, config export/import (V1.2), factory-reset (V1.3), manual port selection
- **Serial Protocol**: Text-based command/response format, error codes, status reporting, configuration persistence
- **Hardware**: LED feedback patterns, button interface, relay control with watchdog, USB serial with crash logging
- **Documentation**: User guide, architecture docs with diagrams, extension guide, API reference, troubleshooting guide
- **Testing**: Hardware validation suite, CLI testing, end-to-end workflows, error recovery scenarios, NVS corruption handling

**Critical Insights from Edge Cases:**
1. **Serial Protocol Definition** (Winston's insight) - Need formal spec for command format, response codes, error handling
2. **Error Recovery Workflows** (Murat's insight) - Multiple ports, config validation, NVS corruption, firmware crashes all need explicit handling
3. **V1.2 Config Export** (Sally's insight) - Critical for Sofia's workflow, but correctly scoped to V1.2 (not MVP bloat)

## IoT/Embedded & CLI Tool Specific Requirements

### Project-Type Overview

This is a dual-stack system combining IoT/Embedded hardware with a developer-friendly CLI tool:

- **Embedded Stack**: ESP32-C3 (RISC-V) firmware written in C using ESP-IDF 5.x, implementing BTHome v2 BLE protocol parsing, state machine logic, NVS persistent storage, and relay control with safety-first design
- **CLI Stack**: Python 3.8+ command-line tool using Typer + Rich frameworks for professional, colorful terminal output with auto-generated help and serial communication via pyserial
- **Integration Model**: USB serial text-based protocol (115200 baud) connecting Python CLI to ESP32-C3 firmware
- **Deployment Model**: JLCPCB-manufactured assembled PCBs + pip-installable Python package

### Technical Architecture Considerations

#### Hardware Requirements

**Hardware Design Reference:**
Complete schematic and PCB layout available in KiCad 9 format at `hardware/ESP32C3-Relay-Module-Rev-A/` (includes `.kicad_sch`, `.kicad_pcb`, and project files).

**ESP32-C3 Platform:**
- **MCU**: ESP32-C3 (RISC-V 32-bit, 160MHz, 400KB SRAM, 4MB flash)
- **Connectivity**: BLE 5.0 for passive listening to BTHome v2 advertising packets
- **Interfaces**: USB-C (serial + power), GPIO for relay control, status LEDs, physical button
- **Relay Module**: DC relay rated for 12-24V loads (LED lighting primary use case)
- **Power Architecture**: Dual input with Schottky diodes - USB-C 5V OR external DC 12-24V (both can be connected simultaneously, Schottky diodes prevent backfeeding)

**Operating Modes:**
- **Initial Configuration**: USB-C powered, laptop/desktop serial connection for CLI-based setup
- **Production Operation**: External DC power (12-24V) OR USB-C power (either works, not both required)
- **Field Maintenance**: USB-C connection for reconfiguration, status checks, firmware updates

**Current Consumption:**
- **Idle (BLE listening)**: ~15-30 mA (ESP32-C3 in active mode, BLE scanning)
- **Relay Activated**: +50-80 mA (relay coil energized)
- **Peak (relay switching)**: ~100-150 mA transient

#### BLE Connectivity Protocol

**BTHome v2 Standard:**
- **Role**: Passive listener (ESP32-C3 scans for BLE advertising packets, does NOT connect to sensors)
- **Supported Devices**: Shelly BLU Button, Motion, Door/Window sensors (extensible to H&T, DoorWindow Gen2)
- **Packet Format**: BTHome v2 encrypted or unencrypted advertising packets (unauthenticated broadcasts acceptable for MVP)
- **Parsing**: Extract device MAC, event type (button press, motion detected, door open/close), battery level, RSSI

**Security Model:**
- **MVP Approach**: Listen to unauthenticated BTHome broadcasts (Shelly BLU sensors broadcast openly by default)
- **Threat Model**: Physical proximity required (BLE range ~10m indoors), MAC address filtering prevents accidental cross-triggering
- **Future Enhancement (V2.0+)**: Optional encrypted BTHome support with bindkey authentication

**Connection Strategy:**
- **No Pairing Required**: ESP32-C3 never initiates connections, only listens to advertising packets
- **RSSI Monitoring**: Track signal strength to detect sensor presence/absence and range issues
- **Scan Parameters**: Continuous scanning with 100ms window, 100ms interval (balance responsiveness vs. power)

#### Serial Protocol Specification

**Physical Layer:**
- **Baud Rate**: 115200 (8N1 - 8 data bits, no parity, 1 stop bit)
- **Hardware**: USB-C connector with CP2102 or CH340 USB-to-UART bridge
- **Flow Control**: None required (command/response is low bandwidth)

**Protocol Format (Text-Based for MVP):**

```
Command Format: COMMAND [ARG1] [ARG2] ...\n
Response Format: OK|data\n or ERROR|message\n

Example Session:
> STATUS\n
< OK|{"state":"listening","sensor":"AA:BB:CC:DD:EE:FF","rssi":-45,"timer":30,"events":5}\n

> TEST_RELAY ON\n
< OK|relay_on\n

> REGISTER_SENSOR AA:BB:CC:DD:EE:FF\n
< OK|sensor_registered\n

> SET_TIMER 60\n
< OK|timer_set\n
```

**Command Set (MVP):**
- `STATUS` → Returns JSON with current state, sensor info, RSSI, event count, errors
- `TEST_RELAY [ON|OFF]` → Manual relay control for testing
- `REGISTER_SENSOR [MAC]` → Register sensor by MAC address (optional, used when trigger-based fails)
- `CLEAR_SENSOR` → Unregister sensor
- `SET_TIMER [seconds]` → Configure relay-on duration (1-600 seconds)
- `SET_RETRIGGER [extend|ignore]` → Configure retriggering behavior
- `GET_LOGS` → Return last 10 events with timestamps
- `PING` → Connectivity test (returns `OK|pong`)

**Error Codes:**
- `ERROR|invalid_command` - Unknown command
- `ERROR|invalid_argument` - Argument out of range or malformed
- `ERROR|nvs_failure` - Persistent storage error
- `ERROR|no_sensor` - Command requires registered sensor but none found
- `ERROR|sensor_battery_low` - Sensor battery critically low (<10%)

**Design Rationale:**
- Text-based (not binary) for easy debugging with serial monitors like PuTTY, screen, minicom
- JSON responses for structured data (easier Python parsing)
- Newline-terminated commands for simple readline() parsing
- Extensible format allows adding commands in future versions without protocol breaks

#### Power Profile & Management

**Operating Scenarios:**

1. **Development/Configuration Mode**:
   - USB-C powered (5V from laptop)
   - External DC NOT connected
   - Power consumption: 15-30 mA idle, 100-150 mA peak

2. **Production Deployment (Dual Power)**:
   - USB-C AND external DC both connected simultaneously
   - Schottky diodes ensure proper power source selection (higher voltage source wins)
   - ESP32-C3 operates from whichever source is connected
   - **Design Note**: Allows USB reconfiguration without disconnecting production power

3. **Production Deployment (DC Only)**:
   - External DC 12-24V input only
   - USB-C not connected
   - Typical installation scenario after initial setup complete

**Schottky Diode Configuration:**
- Prevents backfeeding between USB-C (5V) and external DC (12-24V) power rails
- ESP32-C3 input stage regulates either source down to 3.3V
- No user configuration required - automatic power source selection

**Battery Health Monitoring:**
- Monitor sensor battery level from BTHome packet data
- Warn user when battery level drops below 20% (warning state)
- Error state when battery level drops below 10% (critical)
- Trigger error LED pattern (single blink) and status command warning/error messages

#### Security Model

**Threat Analysis:**

1. **Physical Access Threats**:
   - **Attacker plugs USB into module**: Serial protocol allows full configuration access → Mitigated by physical security (module installed in secured location)
   - **Relay stuck ON due to firmware bug**: Safety hazard for lighting/loads → Mitigated by watchdog timer, fail-safe defaults (relay OFF on boot/error)

2. **Wireless Threats**:
   - **Attacker spoofs BLE packets to trigger relay**: Possible but requires physical proximity (BLE range ~10m) → Mitigated by MAC filtering (only registered sensor triggers relay)
   - **Attacker floods BLE packets (DoS)**: Could drain battery/interfere → Accepted risk for MVP (low-stakes application, physical proximity required)

3. **Configuration Integrity**:
   - **NVS corruption causes incorrect relay behavior**: Could leave relay ON → Mitigated by NVS checksum validation, factory reset command (V1.3)

**Security Principles:**
- **Fail-Safe Defaults**: Relay defaults to OFF on boot, error, power loss, configuration failure
- **MAC Filtering**: Only packets from registered sensor MAC trigger relay (prevents accidental cross-triggering from neighbor's sensors)
- **No Remote Access**: No WiFi, no MQTT, no cloud connectivity in MVP (eliminates remote attack surface)
- **Open Design**: Firmware source code available for security review (educational transparency)

**Future Security Enhancements (V2.0+):**
- BTHome encrypted broadcasts with bindkey authentication
- OTA firmware updates with signature verification
- Optional BLE secure connections for configuration (alternative to USB serial)

#### Firmware Update Strategy

**MVP Approach (Manual Flashing):**
- **Method**: Standard ESP-IDF `esptool` flashing via USB-C
- **User Workflow**:
  1. Download `.bin` firmware file from GitHub releases
  2. Install esptool: `pip install esptool`
  3. Flash: `esptool.py --port /dev/ttyUSB0 write_flash 0x0 firmware.bin`
- **Target Users**: DIY enthusiasts (Marcus) and learners (Priya) comfortable with command-line tools
- **Rationale**: Keeps MVP scope focused, esptool is industry-standard and well-documented

**V1.2 Enhancement (CLI Wrapper):**
- Add `relay-cli flash firmware.bin` command that wraps esptool
- Auto-detects serial port, enters bootloader mode automatically
- Friendlier for installers (Sofia) who want one-command updates
- Still uses esptool under the hood (no custom bootloader required)

**V2.0+ Vision (OTA Updates):**
- Over-The-Air firmware updates via BLE or WiFi (if WiFi mode added)
- Signed firmware images with ESP32 Secure Boot integration
- Requires bootloader partition redesign and signature verification

**Recommended Approach for MVP:**
- Keep manual esptool flashing (proven, simple, no custom code)
- Document clear flashing instructions with screenshots
- Defer CLI wrapper to V1.2 based on user feedback (if Sofia's workflow shows demand)

#### CLI Tool Architecture

**Framework Choice:**
- **Primary**: Typer (modern CLI framework with automatic help generation from type hints)
- **UI Layer**: Rich (beautiful terminal output - tables, progress bars, colors, syntax highlighting)
- **Serial Communication**: pyserial (cross-platform serial port handling)
- **Rationale**: Research report (RESEARCH-REPORT-Python-CLI-Frameworks.md) validated Typer + Rich as best-in-class for professional CLI tools

**Command Structure:**

```
relay-cli status                           # Show current state, sensor info, RSSI, events
relay-cli test-relay --on|--off            # Manual relay control
relay-cli register-sensor [--mac MAC]     # Trigger-based or manual MAC registration
relay-cli clear-sensor                     # Unregister sensor
relay-cli set-timer <seconds>              # Configure relay-on duration
relay-cli set-retrigger <extend|ignore>    # Configure retriggering behavior
relay-cli logs                             # Show last 10 trigger events
relay-cli config export <file.yaml>        # V1.2: Export configuration
relay-cli config import <file.yaml>        # V1.2: Import configuration
relay-cli factory-reset                    # V1.3: Clear all NVS data
relay-cli flash <firmware.bin>             # V1.2: Wrapper for esptool flashing
```

**Output Formatting:**
- **Status Command**: Rich table showing state, sensor MAC, RSSI, timer, event count, errors
- **Colors**: Green for success, yellow for warnings (weak RSSI), red for errors
- **Progress Indicators**: Spinner during "waiting for sensor trigger" registration
- **Help Text**: Auto-generated from Typer docstrings with examples

**Serial Port Auto-Detection:**
- Scan available serial ports using `serial.tools.list_ports`
- Filter for ESP32-C3 USB-to-UART bridge IDs (CP2102, CH340)
- If single match: auto-connect
- If multiple matches: prompt user to select (fallback for Marcus's edge case with Arduino plugged in)
- If no matches: error with instructions to check USB connection

**Shell Completion:**
- Generate shell completion scripts for Bash, Zsh, Fish
- Enable tab-completion for commands and arguments
- Installation: `relay-cli --install-completion`

#### Configuration Schema (V1.2 Feature)

**Config File Format (YAML):**

```yaml
# relay-config.yaml
schema_version: 1.0
relay_timer: 90        # seconds, range 1-600
retrigger_mode: extend # extend | ignore
# Sensor MAC is NOT exported - each module must register its own sensor
# Rationale: MAC is device-specific (Sofia registers different sensor per office)
```

**Export Workflow:**
```bash
relay-cli config export office-config.yaml
# Saves timer + retrigger settings, excludes sensor MAC
```

**Import Workflow:**
```bash
relay-cli config import office-config.yaml
# Validates schema version, applies settings
# User still runs `register-sensor` to register this module's specific sensor
```

**Validation:**
- Schema version check (prevents loading incompatible configs)
- Range validation (timer 1-600 seconds)
- Enum validation (retrigger_mode must be 'extend' or 'ignore')
- Error messages guide user to fix invalid config files

**Rationale for Excluding Sensor MAC:**
- Each module is deployed with a different physical sensor (Sofia's 12 offices = 12 different Shelly BLU sensors)
- Config export/import is for settings replication, not sensor cloning
- Forces explicit per-device sensor registration (prevents accidental misconfiguration)

### Implementation Considerations

**Development Stack:**
- **Firmware**: ESP-IDF 5.x (stable LTS), C language, FreeRTOS, NVS storage library
- **CLI**: Python 3.8+ (widely available), Typer 0.9+, Rich 13+, pyserial 3.5+
- **Build Tools**: ESP-IDF build system (CMake), pip for Python packaging
- **Version Control**: Git with semantic versioning (firmware and CLI versioned together)

**Development Priorities:**
1. **Priority #0**: Hardware validation test suite (validate all interfaces before architecture work)
2. **Priority #1**: Core firmware - BTHome parser, state machine, NVS storage, relay control
3. **Priority #2**: Serial protocol implementation and testing
4. **Priority #3**: MVP CLI commands - status, test-relay, register-sensor, set-timer, logs
5. **Priority #4**: CLI polish - Rich formatting, auto-detection, error handling, help text

**Testing Strategy:**
- **Hardware Validation**: Test every interface (LEDs, relay, button, BLE scanning, USB serial) with simple test firmware before architecture implementation
- **Unit Testing**: BTHome parser unit tests with real packet captures from Shelly BLU devices
- **Integration Testing**: End-to-end workflows matching user journeys (Marcus, Sofia, Priya scenarios)
- **Error Recovery Testing**: NVS corruption, firmware crashes, multiple port detection, config validation failures
- **Load Testing**: 24-hour continuous operation, 1000+ trigger cycles, sensor battery health monitoring

**Code Quality Targets:**
- **Firmware Readability**: 4.0/5.0 rating from 3+ developers (supports Priya's learning journey)
- **Inline Comments**: Explain non-obvious design decisions, BTHome packet format, state transitions
- **Architecture Documentation**: State machine diagram, serial protocol spec, NVS schema, extension guide
- **CLI Help Text**: Every command has clear description, examples, argument constraints

**Risk Mitigations:**
- **Relay Stuck ON**: Watchdog timer resets ESP32-C3 if firmware hangs, relay defaults OFF on boot
- **NVS Corruption**: Factory reset command (V1.3) clears corrupted data, validates writes with checksums
- **BLE Range Issues**: RSSI monitoring and status command visibility helps users diagnose placement problems (Marcus's edge case)
- **Firmware Crashes**: Serial crash logs with stack traces help developers (Priya) debug issues

## Project Scoping & Phased Development

### MVP Strategy & Philosophy

**MVP Approach: Problem-Solving MVP with Educational Transparency**

This MVP prioritizes delivering immediate user value (Marcus's "got it working, yeah!" moment) while maintaining code quality for learning (Priya's portfolio needs). The strategy balances three competing priorities:

1. **Speed to Value**: Marcus gets garage lighting working in <10 minutes
2. **Professional Margin**: Sofia can deploy efficiently (though bulk workflow deferred to V1.2)
3. **Learning Quality**: Priya gets production-quality reference code

**Key Strategic Decisions:**

- **ONE Sensor Constraint**: Proves the concept works reliably before adding complexity. Prevents scope creep while validating core value proposition.
- **USB-First Configuration**: Eliminates WiFi setup complexity, enables job site deployment without network dependency
- **Text-Based Serial Protocol**: Debuggable with standard tools (PuTTY, screen), supports educational transparency
- **Manual Firmware Flashing**: Leverages industry-standard esptool, keeps MVP scope focused

**Resource Requirements:**
- **Team Size**: 1 developer (dual-stack: embedded C + Python CLI)
- **Skills Needed**: ESP-IDF/FreeRTOS, BLE protocols (BTHome v2), Python CLI development (Typer/Rich), serial communication
- **Development Time**: 4-6 weeks (assuming hardware validation passes Priority #0)
- **External Dependencies**: JLCPCB for assembled PCBs, Shelly BLU sensors for testing

### MVP Feature Set (Phase 1)

**Core User Journeys Supported:**

1. **Marcus (Primary)**: DIY enthusiast gets garage motion-activated lighting working in <10 minutes
   - Full journey supported: hardware validation → sensor registration → timer config → deployment

2. **Priya (Secondary)**: Student studies firmware, extends with temperature sensor support
   - Full journey supported: code review → extension guide → custom sensor implementation

3. **Sofia (Partial)**: Installer deploys modules but WITHOUT bulk config workflow
   - Supported: Per-module USB configuration, reliable deployment
   - **NOT Supported in MVP**: Config export/import (deferred to V1.2)
   - **Rationale**: Sofia can still deploy profitably at 10 min/module; bulk optimization comes after core value proven

**Must-Have Capabilities:**

**Firmware (ESP32-C3):**
1. Hardware Validation Test Suite (Priority #0 - gates all other work)
2. BTHome v2 Parser (Button, Motion, Door/Window sensors)
3. Persistent Storage (NVS) - single sensor config + timer settings
4. Trigger-Based Sensor Learning (press button → trigger sensor → auto-register)
5. Manual MAC Entry Fallback (`REGISTER_SENSOR [MAC]` command)
6. Basic State Machine (Configuration, Listening, Active states)
7. Relay Control with Timer (1-600 seconds, countdown with relay-off on expiration)
8. Configurable Retriggering (extend timer vs. ignore modes)
9. Event Buffer (last 10 trigger events with timestamps)
10. Error LED Patterns (visual diagnostics: steady, slow blink, fast blink, double blink)
11. Sensor Battery Health Monitoring (warning <20%, error <10%)
12. Watchdog Timer + Fail-Safe Defaults (relay OFF on boot/error)

**CLI Tool (Python):**
1. Core Commands: `status`, `test-relay`, `register-sensor`, `set-timer`, `set-retrigger`, `logs`, `clear-sensor`
2. Serial Port Auto-Detection (with manual selection fallback for multiple ports)
3. Rich Table Output (state, sensor MAC, RSSI, timer, event count, errors)
4. Colorful Status Indicators (green success, yellow warnings, red errors)
5. Comprehensive Help System (auto-generated from Typer docstrings)
6. Error Messages with Actionable Guidance

**Serial Protocol:**
1. Text-Based Command/Response Format (115200 baud, newline-terminated)
2. JSON Status Responses (structured data for easy parsing)
3. Error Code System (`invalid_command`, `invalid_argument`, `nvs_failure`, etc.)
4. PING Command (connectivity testing)

**Documentation:**
1. Hardware Validation Guide (test procedure for all interfaces)
2. Quick Start Guide (Marcus's 10-minute setup flow)
3. Architecture Documentation (state machine diagram, serial protocol spec)
4. Extension Guide (Priya's temperature sensor example)
5. Firmware Flashing Instructions (esptool workflow with screenshots)

**MVP Success = Shipping Complete List Above**

**Deliberately Excluded from MVP:**
- Multi-sensor support → V1.1 (proves concept with one sensor first)
- Config file export/import → V1.2 (installer workflow optimization)
- Factory reset command → V1.3 (critical for recovery but not launch blocker)
- CLI firmware flash wrapper → V1.2 (esptool works, wrapper adds convenience)
- Advanced trigger logic (lux gates, compound conditions) → V1.3+
- Alternative configuration methods (AT commands, BLE app) → V2.0+
- Network connectivity (WiFi, MQTT) → V2.0+

### Post-MVP Features

**Phase 2 (V1.1 - Multi-Sensor, 2-3 weeks after MVP):**

**Goal**: Scale from one sensor to 2-5 sensors per module

**Features:**
- Support 2-5 registered sensors triggering same relay
- Enhanced CLI: `list-sensors`, `add-sensor`, `remove-sensor`
- Per-sensor trigger configuration
- Sensor registry management in NVS
- Multi-sensor event correlation in logs

**User Value Unlocked:**
- Marcus can trigger garage lights from multiple doors (front door + side door motion sensors)
- Enables more complex automation scenarios without additional hardware

**Phase 2 (V1.2 - Configuration Management, 3-4 weeks after V1.1):**

**Goal**: Unlock Sofia's bulk deployment workflow

**Features:**
- Config file export to YAML (`config export config.yaml`)
- Config file import from YAML (`config import config.yaml`)
- Scan-and-select sensor registration (alternative to trigger-based learning)
- Configuration validation and error checking on import
- CLI firmware flash wrapper (`relay-cli flash firmware.bin`)

**User Value Unlocked:**
- Sofia's workflow drops from 10 min/module to <6 min/module (margin protection)
- One "golden" config replicated across 12+ installations
- Friendlier firmware updates for less technical users

**Rationale for V1.2 Timing:**
- Config export requires MVP to stabilize first (what settings are worth exporting?)
- Sensor MAC intentionally excluded from config (per-device registration prevents misconfiguration)
- CLI flash wrapper builds on proven esptool foundation

**Phase 3 (V1.3 - Advanced Automation, 4-6 weeks after V1.2):**

**Goal**: Enhance reliability and add advanced trigger logic

**Features:**
- Lux gate support (only trigger if below threshold - daylight-aware automation)
- AT-style command support (direct serial terminal access, no CLI required)
- Factory reset command (recovery from NVS corruption)
- Enhanced error classification and logging
- Firmware crash recovery improvements

**User Value Unlocked:**
- Marcus's garage lights only turn on at night (energy savings)
- Field technicians can troubleshoot with serial terminals (no Python installation required)
- Corrupted modules recoverable without reflashing firmware

### Expansion Vision (V2.0+, 6+ months)

**Connectivity & Polish:**
- Mobile BLE app for wireless configuration (no USB required)
- Home Assistant MQTT integration (ecosystem connectivity)
- OTA firmware updates over BLE (frictionless updates)
- Optional web interface (WiFi mode for advanced users)

**Hardware Evolution:**
- PWM-capable relay module for LED dimming (requires hardware redesign)
- Multi-relay boards (2-4 relays per ESP32-C3)
- Solar-powered outdoor variant with battery management
- Weatherproof enclosure options

**Ecosystem Integration:**
- Pre-built integrations with popular home automation platforms
- Support for additional BTHome sensors beyond Shelly BLU
- Custom sensor profiles for community-contributed device types
- Video tutorial series, workshop curriculum, "Build Your Own" guidebook

### Risk Mitigation Strategy

**Technical Risks:**

1. **Risk: Hardware interfaces don't work as expected (BLE, relay, LEDs, button, USB)**
   - **Mitigation**: Priority #0 Hardware Validation Test Suite gates all architectural work
   - **Validation**: Simple test firmware validates every interface before state machine implementation
   - **Contingency**: If hardware validation fails, halt development and redesign PCB

2. **Risk: BTHome v2 parsing fails with real Shelly BLU devices**
   - **Mitigation**: Unit tests with real packet captures from Shelly BLU Button, Motion, Door/Window
   - **Validation**: Test with physical sensors before firmware integration
   - **Contingency**: Simplified packet parsing for MVP (single sensor type), defer multi-type support

3. **Risk: NVS storage corruption causes relay stuck ON (safety hazard)**
   - **Mitigation**: Watchdog timer + fail-safe defaults (relay OFF on boot/error)
   - **Validation**: Corruption testing with deliberate NVS damage
   - **Contingency**: Factory reset command (V1.3) for field recovery

4. **Risk: Serial protocol proves inadequate for CLI needs**
   - **Mitigation**: Text-based format debuggable with standard tools, JSON responses extensible
   - **Validation**: Prototype protocol with Python mock before firmware implementation
   - **Contingency**: Protocol versioning allows breaking changes between major versions

**Market Risks:**

1. **Risk: Users can't achieve <10 minute setup (complexity too high)**
   - **Mitigation**: Trigger-based sensor learning (press button → wave sensor → done)
   - **Validation**: Usability testing with 5+ first-time users (screen recording + timer)
   - **Contingency**: Enhanced status command guidance, video tutorial, simplified CLI prompts

2. **Risk: Sofia's workflow not profitable without config export (margin protection fails)**
   - **Mitigation**: V1.2 roadmap clearly communicates config export timing (3-4 months post-MVP)
   - **Validation**: Early feedback from installer beta testers on V1.0 deployment times
   - **Contingency**: Prioritize V1.2 config export if installer adoption stalls

3. **Risk: Code quality insufficient for Priya's learning (portfolio value fails)**
   - **Mitigation**: 4.0/5.0 readability target, architecture docs, extension guide, inline comments
   - **Validation**: Code review by 3+ developers before MVP launch
   - **Contingency**: Post-launch refactoring sprints to improve code clarity based on feedback

**Resource Risks:**

1. **Risk: Development takes longer than 4-6 weeks (scope creep, technical blockers)**
   - **Mitigation**: ONE sensor constraint enforced in MVP, config export deferred to V1.2
   - **Validation**: Weekly sprint reviews, kill features that slip timeline
   - **Contingency**: Ship MVP without event buffer or battery death detection if needed (add in V1.1)

2. **Risk: Solo developer bottleneck (firmware + CLI + docs)**
   - **Mitigation**: Leverage proven frameworks (ESP-IDF, Typer/Rich), text-based protocol (simple)
   - **Validation**: Spike work on BTHome parser and CLI auto-detection before committing to timeline
   - **Contingency**: Community contributions for non-critical features (shell completion, extra sensor types)

3. **Risk: JLCPCB manufacturing issues delay hardware availability**
   - **Mitigation**: Order dev batch early (10 units for testing while finalizing firmware)
   - **Validation**: Hardware validation on dev batch before ordering production run
   - **Contingency**: Manual breadboard prototypes for development if PCBs delayed (ugly but functional)

## Functional Requirements

### Hardware Interface & Validation (FR1-FR6)

- FR1: System can control relay (energize/de-energize) via GPIO output
- FR2: System can detect button press via GPIO input for triggering configuration mode
- FR3: White status LED can indicate operating mode (fast blink = configuration mode, slow blink = listening mode, solid ON = relay energized)
- FR4: Red error LED can indicate diagnostic states (single blink = sensor battery low <20%, double blink = NVS failure, triple blink = BLE initialization failure)
- FR5: System can communicate via USB serial connection for configuration and status reporting
- FR6: System can operate from either USB-C 5V power OR external DC 12-24V power, with Schottky diode isolation preventing backfeeding when both sources connected

### BLE Sensor Communication (FR7-FR11)

- FR7: System can receive and decode BLE advertising packets from supported sensors without establishing connections
- FR8: System can decode sensor events from Shelly BLU Button (single/double/triple press), Motion (motion detected), and Door/Window (open/close) devices
- FR9: System can extract device MAC address, event type, battery level, and RSSI from BLE advertising packets
- FR10: System can filter BLE packets to only process events from registered sensor MAC address
- FR11: System can monitor sensor battery level from BTHome packets and alert user when battery is low (warning at <20%, error at <10%)

### Sensor Registration & Configuration (FR12-FR16)

- FR12: System can enter 30-second sensor learning mode when physical button pressed, auto-registering first sensor that triggers during window
- FR13: Users can manually register sensor by MAC address when trigger-based learning fails
- FR14: System can store single sensor configuration persistently in NVS (survives power cycles)
- FR15: Users can clear registered sensor configuration
- FR16: System can validate sensor MAC address format before registration

### Relay Control & Timer Logic (FR17-FR21)

- FR17: System can energize relay when registered sensor triggers event
- FR18: System can de-energize relay automatically after configurable timer expires (1-600 seconds)
- FR19: Users can configure relay-on duration (timer setting) with 1-second granularity
- FR20: System can support two retriggering modes: "extend" (reset timer on new trigger) or "ignore" (ignore triggers while relay active)
- FR21: System can maintain relay state and timer configuration across power cycles

### Event Logging & Diagnostics (FR22-FR26)

- FR22: System can buffer last 10 trigger events with timestamps (uptime-based acceptable for MVP)
- FR23: Users can retrieve event log showing trigger history
- FR24: System can log errors (NVS failures, BLE initialization failures, sensor timeouts)
- FR25: Users can retrieve error log for troubleshooting
- FR26: Users can query comprehensive system status including current state, sensor info (MAC, last seen, RSSI), relay state, timer value, event count, error conditions

### CLI Configuration Interface (FR27-FR32)

- FR27: CLI can auto-detect ESP32-C3 serial port on common platforms (Windows, macOS, Linux) with manual selection fallback when multiple ports detected
- FR28: CLI can communicate with firmware using text-based serial protocol (115200 baud, newline-terminated commands, JSON responses, defined error codes)
- FR29: CLI can display colorful, formatted output using tables, progress indicators, and color-coded status (green success, yellow warnings, red errors)
- FR30: CLI can provide comprehensive help text auto-generated from command documentation
- FR31: CLI can provide clear, actionable error messages for connection failures and configuration issues
- FR32: Users can test relay manually (turn ON/OFF) via CLI command

### State Machine & Safety (FR33-FR38)

- FR33: System can transition between Configuration, Listening, and Active states based on sensor registration, trigger events, and timer expiration
- FR34: System can default relay to OFF state on boot, error conditions, or power loss (fail-safe)
- FR35: System can validate configuration changes (timer range 1-600 seconds, retrigger mode enum validation) before applying
- FR36: System can recover from firmware crashes with serial crash logs including stack traces
- FR37: System can implement watchdog timer that resets MCU if firmware hangs, with relay defaulting to OFF on reset
- FR38: System can detect NVS corruption and prevent relay activation with corrupted configuration

### Firmware Management & Updates (FR39-FR41)

- FR39: Users can update firmware via standard ESP-IDF flashing tools over USB serial
- FR40: System can maintain firmware version information accessible via status command
- FR41: Firmware can log boot reason (power-on, watchdog reset, crash) for diagnostics

### Documentation & Extensibility (FR42-FR46)

- FR42: Firmware codebase can achieve 4.0/5.0 readability rating from independent developers
- FR43: Architecture documentation can include state machine diagram, serial protocol specification, and NVS schema
- FR44: Extension guide can provide clear instructions for adding support for new sensor types
- FR45: Code can include inline comments explaining non-obvious design decisions, BTHome packet format details, and state transition logic
- FR46: CLI can provide auto-generated help text with examples and argument constraints for every command

### Future Capabilities (Post-MVP, FR47-FR56)

- FR47: System can support 2-5 registered sensors triggering same relay (V1.1)
- FR48: Users can export configuration to YAML file (V1.2)
- FR49: Users can import configuration from YAML file with schema validation (V1.2)
- FR50: Users can scan and select sensors from list of nearby BLE devices (V1.2)
- FR51: CLI can wrap esptool functionality for simplified firmware flashing (V1.2)
- FR52: System can support lux gate triggering (only activate relay below light threshold) (V1.3)
- FR53: Users can send AT-style commands directly via serial terminal without CLI (V1.3)
- FR54: Users can execute factory reset to clear all NVS data and return to defaults (V1.3)
- FR55: System can support OTA firmware updates over BLE (V2.0+)
- FR56: Users can configure system via mobile BLE app without USB connection (V2.0+)

## Non-Functional Requirements

### Performance

**NFP1 - Relay Response Latency:**
- Relay must energize within 500ms of BLE event detection (measured from BLE packet reception to GPIO output)
- **Rationale**: Immediate feedback required for user confidence; delays perceived as system failure
- **Measurement**: Oscilloscope timing from BLE packet arrival to relay click

**NFP2 - Timer Accuracy:**
- Relay countdown timer must be accurate within ±1 second over 10-minute duration
- **Rationale**: Success criteria specifies timer accuracy for predictable automation behavior
- **Measurement**: Compare actual relay-off time against configured timer value across 100+ test cycles

**NFP3 - BLE Packet Processing Rate:**
- System must process BLE advertising packets at scan rate of 10 packets/second minimum
- **Rationale**: Prevents missed triggers during normal BLE traffic
- **Measurement**: Monitor BLE packet queue depth during continuous scanning

**NFP4 - CLI Command Response Time:**
- CLI status command must return results within 2 seconds on USB serial connection
- **Rationale**: Interactive responsiveness threshold; delays reduce usability
- **Measurement**: Time from command send to response display (end-to-end)

**NFP5 - Boot Time:**
- System must complete boot and enter Listening state within 5 seconds of power-on
- **Rationale**: Field recovery scenarios (watchdog reset) should recover quickly
- **Measurement**: Serial log timestamp from reset to "Listening mode" log entry

### Reliability

**NFR1 - Relay Fail-Safe Behavior:**
- Relay MUST default to OFF state in 100% of error conditions (NVS corruption, BLE init failure, firmware crash, watchdog reset)
- **Rationale**: Safety-critical requirement (Success Criteria: zero stuck-on relay incidents)
- **Measurement**: Inject faults (corrupt NVS, crash firmware) and verify relay state = OFF

**NFR2 - Watchdog Timer:**
- Watchdog timer must reset MCU within 10 seconds if firmware hangs
- Relay must de-energize within 1 second of watchdog reset trigger
- **Rationale**: Prevents indefinite relay-on state if firmware deadlocks
- **Measurement**: Simulate firmware hang (infinite loop) and measure time to reset + relay OFF

**NFR3 - NVS Write Durability:**
- Configuration writes to NVS must survive 10,000 power cycle events with <0.1% corruption rate
- **Rationale**: Installed modules may experience frequent power interruptions
- **Measurement**: Automated power cycling test with configuration verification

**NFR4 - Sensor Detection Reliability:**
- System must detect and process BLE sensor events with >95% success rate at RSSI ≥ -70 dBm
- **Rationale**: Defines minimum acceptable signal strength for reliable operation and troubleshooting guidance
- **Measurement**: Trigger sensor 100 times at -70 dBm RSSI, count missed events

**NFR5 - Continuous Operation:**
- System must operate continuously for 168 hours (1 week) without firmware crash or memory leak
- **Rationale**: 24/7 deployment requires stability for installer credibility and user confidence
- **Measurement**: Soak test with periodic sensor triggers, monitor heap usage and crash logs

### Usability

**NFU1 - Time to First Success:**
- 80% of first-time users must successfully register sensor and test relay within 10 minutes
- **Rationale**: Success Criteria metric for Marcus's journey
- **Measurement**: Usability testing with 10+ first-time users, screen recording + timer

**NFU2 - CLI Error Message Quality:**
- 100% of error messages must include actionable guidance (what to do next)
- **Rationale**: Self-service troubleshooting reduces support burden and improves user experience
- **Measurement**: Review all error code paths, verify each has actionable message

**NFU3 - Status Command Comprehensiveness:**
- Status command must answer these questions without additional commands:
  - Is relay ON or OFF?
  - Is a sensor registered? (MAC address visible)
  - Is sensor in range? (RSSI + last seen timestamp)
  - What's the current timer setting?
  - Are there any errors? (error log summary)
- **Rationale**: Sofia's verification workflow, Marcus's troubleshooting workflow
- **Measurement**: Task-based testing: "Diagnose why relay isn't triggering" using only status command

**NFU4 - CLI Help Quality:**
- Every CLI command must have auto-generated help text with:
  - Clear description (what it does)
  - Argument constraints (ranges, valid values)
  - At least one usage example
- **Rationale**: Self-service support for DIY users (Marcus) and installers (Sofia)
- **Measurement**: Review help output for all commands against checklist

**NFU5 - LED Feedback Clarity:**
- Users must be able to distinguish operating modes via LED patterns without documentation:
  - Fast blink vs. slow blink (10 users identify correctly >90% accuracy)
  - Error patterns (single/double/triple blink distinguishable)
- **Rationale**: Visual feedback for non-CLI troubleshooting
- **Measurement**: User testing: "What state is the module in?" based on LED observation

### Maintainability

**NFM1 - Code Readability Target:**
- Firmware codebase must achieve ≥4.0/5.0 readability rating from 3+ independent embedded developers
- **Rationale**: Success Criteria for Priya's learning journey
- **Measurement**: Standardized code review survey (1-5 scale on clarity, structure, comments)

**NFM2 - Architecture Documentation:**
- Documentation must include:
  - State machine diagram with all transitions
  - Serial protocol specification (command format, error codes, response format)
  - NVS schema (keys, value types, defaults)
- **Rationale**: Priya needs reference documentation for extension, Sofia needs protocol spec for AT commands (V1.3)
- **Measurement**: Documentation review against checklist

**NFM3 - Extension Development Time:**
- Intermediate developers must be able to add support for new sensor type (e.g., temperature) in ≤4 hours using extension guide
- **Rationale**: Success Criteria for Priya's temperature sensor extension
- **Measurement**: Task-based testing with 2+ intermediate developers

**NFM4 - Inline Comment Coverage:**
- Non-obvious code sections must have explanatory comments:
  - BTHome packet parsing logic
  - State transition conditions
  - NVS checksum validation
  - Watchdog timer configuration
- **Rationale**: Supports Priya's learning and community contributions
- **Measurement**: Code review identifies sections needing comments, verify coverage

### Compatibility

**NFC1 - CLI Cross-Platform Support:**
- CLI must run on Windows 10+, macOS 11+, Linux (Ubuntu 20.04+) without platform-specific code paths
- **Rationale**: User base spans multiple platforms; unified codebase reduces maintenance burden
- **Measurement**: Functional testing on all three platforms with representative hardware

**NFC2 - Serial Port Auto-Detection:**
- CLI must correctly identify ESP32-C3 USB-UART bridges (CP2102, CH340) on all supported platforms
- Manual selection fallback must work when multiple serial devices present
- **Rationale**: Marcus's edge case with Arduino also plugged in
- **Measurement**: Test with 0, 1, 2+ serial devices on each platform

**NFC3 - ESP-IDF Version Compatibility:**
- Firmware must compile and run on ESP-IDF 5.1.x LTS and 5.2.x
- **Rationale**: Users may have different ESP-IDF versions installed; LTS ensures long-term support
- **Measurement**: CI build testing on both versions

**NFC4 - Shelly BLU Sensor Compatibility:**
- System must correctly parse BTHome v2 packets from:
  - Shelly BLU Button (all press types: single, double, triple)
  - Shelly BLU Motion (motion detected event)
  - Shelly BLU Door/Window (open/close events)
- **Rationale**: User journeys specify these three sensor types for MVP
- **Measurement**: Integration testing with physical sensors of each type

**NFC5 - Python Version Support:**
- CLI must run on Python 3.8+ (no Python 3.7 or earlier)
- **Rationale**: Python 3.8 widely available on modern systems, enables Typer/Rich framework usage
- **Measurement**: CI testing on Python 3.8, 3.9, 3.10, 3.11, 3.12
