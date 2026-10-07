# ESP32-C3 Relay Module - ESPHome Version

ESPHome firmware for the ESP32-C3 Relay Module with direct BLE control from Shelly BLU sensors.

## Core Value Proposition

**Direct local BLE trigger — no Home Assistant in the loop.**

BLU sensor event → ESP32 → relay. Sub-second response. No WiFi round-trip, no HA automation latency. The relay responds to BLE events directly at the hardware level.

Home Assistant integration is for *monitoring and configuration* — not the trigger path.

## Features

- **Shelly BLU Sensor Support** - Button, Motion, Door/Window sensors
- **Direct BLE Control** - Sub-second relay response, no hub required
- **Auto-Off Timer** - Configurable 1-600 seconds with retrigger modes
- **Scanner Mode** - Discover BLU device MAC addresses
- **Battery Monitoring** - Track sensor battery in Home Assistant
- **LED Status Indicators** - Visual feedback for all device states
- **Full HA Integration** - Control, configure, and monitor from dashboard

## Supported Sensors

| Sensor | Default Action | Timer |
|--------|---------------|-------|
| **Button** (single press) | Turn ON relay | Yes |
| **Button** (double/triple/long/hold) | Customizable | - |
| **Motion** (detected) | Turn ON relay | Yes |
| **Door/Window** (opened) | Turn ON relay | Yes |

## Hardware

| GPIO | Function | Notes |
|------|----------|-------|
| GPIO7 | Relay | Active HIGH, drives MOSFET |
| GPIO10 | Status LED | White |
| GPIO0 | Error LED | Red |

## LED States

| Status LED | Error LED | Meaning |
|------------|-----------|---------|
| Off | Solid | Not configured (no MAC set) |
| Fast blink (200ms) | Off | Scanner mode active |
| Slow blink (1s) | Off | Listening (relay off) |
| Solid | Off | Relay ON |
| (current) | Single blink | Sensor battery low (<20%) |

## Quick Start

### 1. Create Secrets File

Create `secrets.yaml` in your ESPHome config directory (see `secrets.yaml.example`):

```yaml
wifi_ssid: "YourWiFiNetwork"
wifi_password: "YourWiFiPassword"
ota_password: "your-ota-password"
```

### 2. Find Your Sensor MAC Address

**Option A: Use Scanner Mode**

Edit `esp32c3-relay.yaml`:
```yaml
substitutions:
  scanner_mode: "true"
```

Flash and check logs:
```
[SCANNER] MAC: AA:BB:CC:DD:EE:FF | RSSI: -45 dBm | Type: Button
```

**Option B: Shelly App**

Open Shelly app → Device Settings → Device Information

### 3. Configure the Device

Edit substitutions in `esp32c3-relay.yaml`:

```yaml
substitutions:
  device_name: "esp32c3-relay"
  friendly_name: "ESP32-C3 Relay"

  # Your Shelly BLU MAC address
  shelly_blu_mac: "AA:BB:CC:DD:EE:FF"

  # Sensor type: "button", "motion", or "door"
  sensor_type: "button"

  # Disable scanner mode for normal operation
  scanner_mode: "false"

  # Auto-off timer (seconds, 1-600)
  default_timer: "300"

  # Retrigger mode: "extend" or "ignore"
  default_retrigger: "extend"
```

### 4. Flash the Device

```bash
esphome run esp32c3-relay.yaml
```

Or use ESPHome Dashboard in Home Assistant.

### 5. Initial WiFi Setup (if needed)

If WiFi fails:
1. Device creates AP: "ESP32-C3 Relay Setup"
2. Connect with password: `configure123`
3. Browse to `192.168.4.1`
4. Enter WiFi credentials

## Configuration

### Substitutions Reference

| Parameter | Values | Default | Description |
|-----------|--------|---------|-------------|
| `device_name` | string | `esp32c3-relay` | ESPHome device name |
| `friendly_name` | string | `ESP32-C3 Relay` | Display name in HA |
| `shelly_blu_mac` | `XX:XX:XX:XX:XX:XX` | `00:00:00:00:00:00` | Sensor MAC address |
| `sensor_type` | `button`, `motion`, `door` | `button` | Sensor type (informational) |
| `scanner_mode` | `true`, `false` | `false` | Enable device discovery |
| `default_timer` | `1`-`600` | `300` | Auto-off seconds |
| `default_retrigger` | `extend`, `ignore` | `extend` | Timer retrigger behavior |

### Timer Behavior

The timer activates for **all sensor types** (button, motion, door):

- **Timer starts** when sensor triggers relay ON
- **Timer expires** → relay turns OFF automatically
- **Retrigger EXTEND** → new trigger resets countdown
- **Retrigger IGNORE** → new triggers ignored while active

All sensors work identically: trigger → relay ON → timer starts.

### Manual Override

When you control the relay directly from Home Assistant:
- Timer does NOT start
- Sensor triggers are IGNORED while relay is manually ON
- Turn relay OFF via HA to re-enable sensor triggers

## Home Assistant Entities

### Controls

| Entity | Type | Description |
|--------|------|-------------|
| Relay | `switch` | Turn relay on/off |
| Timer Duration | `number` | Set auto-off timer (1-600s) |
| Retrigger Mode | `select` | EXTEND or IGNORE |
| Restart | `button` | Reboot device |

### Sensors

| Entity | Type | Description |
|--------|------|-------------|
| Timer Remaining | `sensor` | Seconds until auto-off (0 if inactive) |
| Sensor Battery | `sensor` | BLU device battery percentage |
| WiFi Signal | `sensor` | Connection strength (dBm) |
| Uptime | `sensor` | Device uptime |
| Status | `binary_sensor` | Online/offline |

### Diagnostics

| Entity | Type | Description |
|--------|------|-------------|
| Device Mode | `text_sensor` | Current mode: Scanner, Unconfigured, or configured MAC |
| Last Event MAC | `text_sensor` | MAC of last BLU event |
| Last Event RSSI | `sensor` | Signal strength of last event |
| Last Event Type | `text_sensor` | Event type (e.g., `motion_detected`) |

## Customization

### Add Actions for Button Events

Edit the BLE lambda to add custom actions:

```cpp
else if (event == 0x02) {  // Double press
  id(last_event_type_str) = "button_double";
  ESP_LOGI("shelly_blu", "Button: double press");
  // Add your action here, e.g.:
  // id(relay).turn_on();
  // id(auto_off_timer).execute();
}
```

### Add Action for Door Close

```cpp
} else {  // Closed
  id(last_event_type_str) = "door_closed";
  ESP_LOGD("shelly_blu", "Door/Window: closed");
  // Add your action here, e.g.:
  // id(relay).turn_off();
}
```

## Troubleshooting

### Device not appearing in Home Assistant

- Verify WiFi credentials in `secrets.yaml`
- Review ESPHome logs for connection errors
- Ensure Home Assistant and device are on same network

### Sensor not triggering relay

- Verify MAC address is correct (case-insensitive)
- Check `scanner_mode` is `"false"`
- Ensure sensor is within BLE range (~10-15m)
- Look for event logs: `"Button: single press"`, `"Motion: detected"`

### Timer not working

- Check relay wasn't turned on manually via HA (manual control disables timer)
- Verify `default_timer` is set (1-600)
- Check "Timer Remaining" sensor in HA to see countdown

### Scanner mode shows no devices

- Ensure Shelly BLU sensor is awake (press button, trigger motion)
- Check logs at INFO level or higher
- BLU devices only advertise during/after events

### Relay not switching

- Check 3.3V power supply capacity
- Verify GPIO7 wiring to MOSFET gate
- Test relay directly via HA switch entity

## BTHome v2 Reference

Pre-decoded object IDs (for advanced customization):

| Object | ID | Values |
|--------|-----|--------|
| Battery | `0x01` | 0-100 (%) |
| Motion | `0x21` | 0=clear, 1=detected |
| Door/Window | `0x2D` | 0=closed, 1=open |
| Button | `0x3A` | 1=single, 2=double, 3=triple, 4=long, 128=hold |

Service UUID: `0xFCD2`

## License

MIT - Same as parent project
