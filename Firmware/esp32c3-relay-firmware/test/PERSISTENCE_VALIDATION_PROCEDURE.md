# Persistence Validation Test Procedure

Story 3.4: Clear Sensor Configuration & Persistence Validation
AC9, AC10: Verify configuration persistence across power cycles

## Purpose

This procedure validates that sensor configuration persists correctly across power cycles,
meeting NFR3 requirement (config survives 10,000 power cycles with <0.1% corruption).

## Test Equipment Required

- ESP32-C3 Relay Module (flashed with latest firmware)
- USB Serial Terminal (PuTTY, screen, minicom, or equivalent)
- USB power switch or hub with per-port power control
- Stopwatch (optional)
- Test log sheet or spreadsheet

## Pre-Test Setup

1. **Flash Latest Firmware**
   ```bash
   cd Firmware/esp32c3-relay-firmware
   idf.py build flash
   ```

2. **Connect Serial Terminal**
   - Baud rate: 115200
   - Data bits: 8
   - Parity: None
   - Stop bits: 1

3. **Clear Existing Configuration**
   ```
   CLEAR_SENSOR
   ```
   Expected: `OK|cleared`

4. **Verify Unconfigured State**
   ```
   STATUS
   ```
   Expected: `{"state":"UNCONFIGURED","sensor_registered":false,"sensor_mac":"","sensor_type":""}`

## Test Procedure: 10-Cycle Persistence Test

### Test Parameters

| Parameter | Value |
|-----------|-------|
| Test MAC | AA:BB:CC:DD:EE:FF |
| Sensor Type | BUTTON |
| Timer | 10 seconds (default) |
| Power-off Duration | 5 seconds minimum |

### Cycle Procedure

**For each cycle (1-10):**

1. **Register Sensor**
   ```
   REGISTER_SENSOR AA:BB:CC:DD:EE:FF BUTTON
   ```
   Expected: `OK|registered|AA:BB:CC:DD:EE:FF|BUTTON`

2. **Verify Registration**
   ```
   STATUS
   ```
   Expected: Contains `"sensor_mac":"AA:BB:CC:DD:EE:FF"` and `"sensor_registered":true`

3. **Record Pre-Cycle Values**
   - sensor_mac: AA:BB:CC:DD:EE:FF
   - sensor_type: 1 (BUTTON)
   - timer_seconds: 10
   - state: LISTENING

4. **Power Cycle**
   - Disconnect USB power
   - Wait 5 seconds
   - Reconnect USB power
   - Wait for boot complete (LED blink pattern starts, serial shows "Firmware initialized")

5. **Verify Persistence**
   ```
   STATUS
   ```
   Expected: All values match pre-cycle recording

6. **Record Result**
   - PASS: All values match
   - FAIL: Any value mismatch or corruption detected

7. **Clear for Next Cycle**
   ```
   CLEAR_SENSOR
   ```
   Expected: `OK|cleared`

### Test Log Template

| Cycle | Pre-Cycle MAC | Post-Cycle MAC | Pre-Cycle State | Post-Cycle State | Result |
|-------|---------------|----------------|-----------------|------------------|--------|
| 1     | AA:BB:CC:DD:EE:FF | | LISTENING | | |
| 2     | AA:BB:CC:DD:EE:FF | | LISTENING | | |
| 3     | AA:BB:CC:DD:EE:FF | | LISTENING | | |
| 4     | AA:BB:CC:DD:EE:FF | | LISTENING | | |
| 5     | AA:BB:CC:DD:EE:FF | | LISTENING | | |
| 6     | AA:BB:CC:DD:EE:FF | | LISTENING | | |
| 7     | AA:BB:CC:DD:EE:FF | | LISTENING | | |
| 8     | AA:BB:CC:DD:EE:FF | | LISTENING | | |
| 9     | AA:BB:CC:DD:EE:FF | | LISTENING | | |
| 10    | AA:BB:CC:DD:EE:FF | | LISTENING | | |

## Pass/Fail Criteria

### Pass Criteria
- All 10 cycles complete with matching configuration values
- No CRC errors detected
- No corruption warnings in serial log
- State correctly restored to LISTENING after each boot

### Fail Criteria
- Any cycle shows configuration mismatch
- CRC error detected during boot ("Config corrupted, relay disabled")
- State incorrect after boot
- sensor_mac, sensor_type, or timer_seconds mismatch

## Optional: Corruption Injection Test

**WARNING: This test intentionally corrupts NVS. Only perform on test devices.**

### Purpose
Verify AC11-AC14: Corruption detection, error LED, relay safety, and recovery.

### Procedure

1. **Register a sensor**
   ```
   REGISTER_SENSOR AA:BB:CC:DD:EE:FF BUTTON
   ```

2. **Verify registration via STATUS**

3. **Simulate corruption** (requires NVS debug tools or direct flash modification)
   - Option A: Use `nvs_partition_gen.py` to create corrupted partition
   - Option B: Modify a single byte in NVS using esptool

4. **Power cycle device**

5. **Verify corruption detection**
   Expected serial output:
   ```
   E (xxx) MAIN: Config corrupted, relay disabled
   ```

6. **Verify error LED**
   - Error LED should show double blink pattern

7. **Verify relay state**
   - Relay should be OFF (audible check or multimeter)

8. **Test recovery via CLEAR_SENSOR**
   ```
   CLEAR_SENSOR
   ```
   Expected: `OK|cleared`

9. **Verify recovered state**
   ```
   STATUS
   ```
   Expected: `{"state":"UNCONFIGURED","sensor_registered":false,...}`

## Extended Durability Test (Optional)

For full NFR3 compliance verification (10,000 cycles):

1. Create automated test script using Python + PySerial
2. Implement power cycling via USB hub control (e.g., uhubctl)
3. Log all results to CSV file
4. Run overnight with 5-second cycle time (~14 hours for 10,000 cycles)

### Sample Python Script Structure
```python
import serial
import subprocess
import time

def power_cycle():
    subprocess.run(["uhubctl", "-a", "off", "-p", "1"])
    time.sleep(5)
    subprocess.run(["uhubctl", "-a", "on", "-p", "1"])
    time.sleep(3)  # Boot time

def run_persistence_test(cycles=10000):
    for i in range(cycles):
        # Register sensor
        # Verify STATUS
        # Power cycle
        # Verify STATUS matches
        # Log result
        pass
```

## References

- FR14: System stores single sensor config persistently in NVS
- FR15: Users can clear registered sensor
- NFR3: Config survives 10,000 power cycles with <0.1% corruption
- Architecture Decision 3: NVS Storage Schema
