# ESP32-C3 Relay Module - ESPHome Edition Spec

## Purpose

Provide an ESPHome firmware alternative for the ESP32-C3 Relay Module that integrates with Home Assistant while supporting Shelly BLU sensors via BTHome v2 protocol.

## Core Value Proposition

**Direct local BLE trigger — no Home Assistant in the loop.**

BLU button press → ESP32 → relay. Sub-second response. No WiFi round-trip, no HA automation latency, no failures when internet hiccups. The relay responds to BLE events directly at the hardware level.

Home Assistant integration is for *monitoring and configuration* — not for the trigger path.

## Design Philosophy

This is a **KISS template starting point** for Home Assistant enthusiasts:

- **Works out of the box** — set your MAC, flash, done
- **Extensible by design** — it's ESPHome; users can fork and customize (multiple sensors, complex automations, etc.)
- **BTHome decoding included** — users shouldn't need to read protocol specs to add a sensor
- **Single sensor default** — keeps the template simple; complexity is opt-in via user modifications

## Target User

**Competent ESPHome user** who:
- Understands ESPHome YAML configuration
- Has flashed ESPHome devices before
- Should NOT need to decode BTHome v2 protocol to add sensors

## Scope

### In Scope

| Feature | Description |
|---------|-------------|
| **Shelly BLU Button** | Single, double, long press actions |
| **Shelly BLU Motion** | Motion detected / clear events |
| **Shelly BLU Door/Window** | Open / closed events |
| **Battery monitoring** | Report sensor battery % to Home Assistant |
| **Scanner mode** | Log discovered BLU devices with MAC addresses |
| **Timer** | Configurable auto-off duration (1-600 seconds) |
| **Retrigger modes** | EXTEND (reset timer) or IGNORE (skip while active) |
| **LED indicators** | Status patterns matching original firmware |
| **Home Assistant UI config** | Timer duration settable from HA dashboard |
| **Single sensor** | One registered BLU device per module |

### Out of Scope

| Feature | Rationale |
|---------|-----------|
| Timer presets (button cycling) | HA UI provides better UX |
| Learning mode (auto-register) | Scanner + manual MAC is sufficient for ESPHome users |
| Serial protocol | Replaced by HA integration |
| Multiple sensors | KISS — template stays simple; users can extend if needed |
| Encrypted BTHome packets | Shelly BLU devices use unencrypted by default |

---

## Functional Requirements

### FR1: BLE Scanner Mode

**Purpose:** Help users discover Shelly BLU device MAC addresses without external tools.

| Requirement | Detail |
|-------------|--------|
| FR1.1 | Scanner mode enabled via substitution flag (`scanner_mode: "true"`) |
| FR1.2 | When enabled, log ALL BTHome v2 devices seen (MAC, RSSI, device type if detectable) |
| FR1.3 | Log format: `[SCANNER] MAC: AA:BB:CC:DD:EE:FF | RSSI: -45 dBm | Type: Button` |
| FR1.4 | Scanner mode should NOT trigger relay actions |
| FR1.5 | User disables scanner and sets MAC after finding their device |
| FR1.6 | **Scanner mode is mutually exclusive with operation** - when enabled, device only logs (no relay control even if MAC configured) |

### FR2: Sensor Support

**Purpose:** Support all three Shelly BLU sensor types with pre-built handlers.

#### FR2.1: Button Events (Object ID 0x3A)

| Event | Value | Default Action |
|-------|-------|----------------|
| Single press | 0x01 | Toggle relay |
| Double press | 0x02 | Configurable (default: none) |
| Triple press | 0x03 | Configurable (default: none) |
| Long press | 0x04 | Configurable (default: none) |
| Hold | 0x80 | Configurable (default: none) |

#### FR2.2: Motion Events (Object ID 0x21)

| Event | Value | Default Action |
|-------|-------|----------------|
| Motion detected | 0x01 | Turn ON relay, start timer |
| Motion clear/timeout | 0x00 | No action (timer handles off) |

#### FR2.3: Door/Window Events (Object ID 0x2D)

| Event | Value | Default Action |
|-------|-------|----------------|
| Door/window opened | 0x01 | Turn ON relay, start timer |
| Door/window closed | 0x00 | Configurable (default: none) |

#### FR2.4: Battery Monitoring (Object ID 0x01)

| Requirement | Detail |
|-------------|--------|
| FR2.4.1 | Parse battery percentage (0-100%) from BTHome packets |
| FR2.4.2 | Expose as Home Assistant sensor entity |
| FR2.4.3 | Update on each received advertisement containing battery data |

### FR3: Timer Functionality

**Purpose:** Auto-off relay after configurable duration with retrigger behavior.

| Requirement | Detail |
|-------------|--------|
| FR3.1 | Timer duration configurable 1-600 seconds |
| FR3.2 | Default timer: 300 seconds (5 minutes) |
| FR3.3 | Expose timer duration as HA `number` entity for UI adjustment |
| FR3.4 | Timer starts when relay turns ON via sensor trigger |
| FR3.5 | Timer does NOT start for manual HA switch control (user controls manually). **Manual override rule:** If relay is already ON from manual control, sensor triggers are ignored (no timer starts). Timer behavior only applies to sensor-initiated activation. |
| FR3.6 | When timer expires, relay turns OFF automatically |

### FR4: Retrigger Behavior

**Purpose:** Control behavior when sensor triggers while relay already active.

| Requirement | Detail |
|-------------|--------|
| FR4.1 | Two modes: EXTEND and IGNORE |
| FR4.2 | EXTEND (default): New trigger resets timer countdown |
| FR4.3 | IGNORE: New triggers ignored while relay active |
| FR4.4 | Expose as HA `select` entity for UI selection |

### FR5: LED Indicators

**Purpose:** Visual feedback matching original firmware behavior.

| State | Status LED (White) | Error LED (Red) |
|-------|-------------------|-----------------|
| Unconfigured (no MAC) | OFF | Solid ON |
| Scanner mode active | Fast blink (200ms) | OFF |
| Listening (relay off) | Slow blink (1s) | OFF |
| Relay active | Solid ON | OFF |
| Sensor battery low (<20%) | (current state) | 1 blink |

### FR6: Home Assistant Entities

**Purpose:** Full integration with Home Assistant dashboard.

| Entity | Type | Description |
|--------|------|-------------|
| Relay | `switch` | Control relay on/off |
| Timer Duration | `number` | Set auto-off timer (1-600s) |
| Retrigger Mode | `select` | EXTEND or IGNORE |
| Timer Remaining | `sensor` | Seconds remaining (0 if inactive) |
| Sensor Battery | `sensor` | BLU device battery % |
| Last Event MAC | `text_sensor` | MAC address of last seen BLU event (diagnostics) |
| Last Event RSSI | `sensor` | Signal strength of last event in dBm (diagnostics) |
| Last Event Type | `text_sensor` | Event type: "button_single", "motion_detected", etc. (diagnostics) |
| WiFi Signal | `sensor` | Connection strength dBm |
| Uptime | `sensor` | Device uptime |
| Restart | `button` | Reboot device |

---

## Configuration Parameters

### Substitutions (User Must Configure)

```yaml
substitutions:
  device_name: "esp32c3-relay"
  friendly_name: "ESP32-C3 Relay"

  # Shelly BLU device MAC (required for operation)
  shelly_blu_mac: "00:00:00:00:00:00"

  # Sensor type: "button", "motion", or "door"
  sensor_type: "button"

  # Enable scanner mode to discover devices (set "false" for normal operation)
  scanner_mode: "false"

  # Default timer duration in seconds
  default_timer: "300"

  # Default retrigger mode: "extend" or "ignore"
  default_retrigger: "extend"
```

---

## BTHome v2 Reference

Pre-decoded for user convenience (users should NOT need to look these up):

| Sensor Type | Object ID | Values |
|-------------|-----------|--------|
| Battery % | 0x01 | 0-100 (percentage) |
| Motion | 0x21 | 0=clear, 1=detected |
| Door/Window | 0x2D | 0=closed, 1=open |
| Button | 0x3A | 1=single, 2=double, 3=triple, 4=long, 128=hold |

Service UUID: `0xFCD2`

---

## Implementation Notes

1. **Sensor type selection** - Use `sensor_type` substitution to include only relevant parsing logic
2. **Scanner vs operational** - Scanner mode and operational mode are mutually exclusive; scanner overrides all operation even if MAC is configured
3. **Timer persistence** - Timer duration and retrigger mode should persist across reboots (use ESPHome `restore_value`)
4. **Manual control override** - When user toggles relay via HA, timer does NOT start. Additionally, if relay is manually ON, sensor triggers are ignored entirely — manual control takes priority until user turns relay OFF
5. **Button actions** - Double/triple/long press actions left as customization points with clear comments
6. **Diagnostic sensors** - Last Event MAC/RSSI/Type sensors update on ANY BTHome event from configured MAC, enabling runtime debugging without scanner mode

---

## File Structure

```
ESPHome/
├── esp32c3-relay.yaml      # Main configuration (updated)
├── secrets.yaml.example    # Template for secrets
├── SPEC.md                 # This specification
└── README.md               # User documentation (updated)
```
