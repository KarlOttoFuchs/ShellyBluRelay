# Power Cycle Durability Test Procedure

**Story:** 3.1 - Implement NVS Storage Component with CRC Validation
**AC:** 9, 10 - Power Cycle Durability
**Requirement:** NFR3 - Config survives 10,000 power cycles with <0.1% corruption

## Purpose

Validate that NVS configuration persists correctly across power cycles with CRC integrity maintained.

## Test Configuration

```c
sensor_config_t test_config = {
    .sensor_mac = "5C:C7:C1:F5:C9:AC",
    .sensor_type = SENSOR_TYPE_BUTTON,  // 1
    .timer_seconds = 45,
    .retrigger_mode = RETRIGGER_EXTEND,  // 0
    .config_version = CONFIG_VERSION,    // 1
    .config_crc = 0  // Calculated by nvs_save_config()
};
```

## Equipment Required

- ESP32-C3 Relay Module (target hardware)
- USB cable for serial connection
- Terminal program (115200 baud)
- Power control (USB disconnect/reconnect or controlled power supply)
- Optional: Automated power cycling hardware (relay controlling USB power)

## Manual Test Procedure (100 cycles)

### Phase 1: Initial Setup

1. Flash firmware: `idf.py flash`
2. Connect serial monitor: `idf.py monitor`
3. Clear any existing config using serial command: `CLEAR_SENSOR`
4. Verify "No sensor config found" message on boot

### Phase 2: Save Test Configuration

1. Use serial command to register test sensor:
   ```
   TEST_REGISTER 5C:C7:C1:F5:C9:AC
   ```
2. Verify serial output shows:
   ```
   Config saved (CRC: 0xXXXXXXXX)
   ```
3. Record the CRC value: ____________

### Phase 3: Power Cycle Validation (Repeat 100 times)

For each cycle (1-100):

1. **Power Off:**
   - Disconnect USB power
   - Wait 2 seconds

2. **Power On:**
   - Reconnect USB power
   - Wait for boot sequence to complete

3. **Verify:**
   - Check serial output for: "Config loaded successfully (CRC: 0xXXXXXXXX)"
   - Verify CRC matches original value
   - Verify no "NVS corruption detected" message
   - Verify no error LED double blink

4. **Record Result:**
   | Cycle # | CRC Match | Error | Notes |
   |---------|-----------|-------|-------|
   | 1       | Yes/No    | Yes/No|       |
   | 2       | Yes/No    | Yes/No|       |
   | ...     | ...       | ...   | ...   |
   | 100     | Yes/No    | Yes/No|       |

### Phase 4: Final Verification

1. After cycle 100, use STATUS command to verify config:
   ```
   STATUS
   ```
2. Expected output:
   ```
   Sensor: 5C:C7:C1:F5:C9:AC
   Type: BUTTON
   Timer: 45s
   Retrigger: EXTEND
   Config CRC: 0xXXXXXXXX (valid)
   ```

## Automated Test Script (Optional)

```bash
#!/bin/bash
# power_cycle_test.sh
# Requires: controllable USB hub or relay

CYCLES=100
LOG_FILE="power_cycle_results.log"
SERIAL_PORT="/dev/ttyUSB0"
EXPECTED_CRC="$1"  # Pass expected CRC as argument

echo "Power Cycle Test - $CYCLES cycles" > $LOG_FILE
echo "Expected CRC: $EXPECTED_CRC" >> $LOG_FILE
echo "Started: $(date)" >> $LOG_FILE

for i in $(seq 1 $CYCLES); do
    echo "Cycle $i/$CYCLES"

    # Power off (using uhubctl or similar)
    uhubctl -a off -l 1-1 -p 1
    sleep 2

    # Power on
    uhubctl -a on -l 1-1 -p 1
    sleep 5  # Wait for boot

    # Capture boot log
    timeout 3 cat $SERIAL_PORT > /tmp/boot_log.txt 2>&1

    # Check for CRC in log
    if grep -q "Config loaded successfully" /tmp/boot_log.txt; then
        CRC=$(grep -oP 'CRC: 0x[0-9A-Fa-f]+' /tmp/boot_log.txt | head -1)
        if [[ "$CRC" == "CRC: $EXPECTED_CRC" ]]; then
            echo "Cycle $i: PASS - CRC match" >> $LOG_FILE
        else
            echo "Cycle $i: FAIL - CRC mismatch ($CRC)" >> $LOG_FILE
        fi
    elif grep -q "NVS corruption detected" /tmp/boot_log.txt; then
        echo "Cycle $i: FAIL - Corruption detected" >> $LOG_FILE
    else
        echo "Cycle $i: UNKNOWN - Could not verify" >> $LOG_FILE
    fi
done

echo "Completed: $(date)" >> $LOG_FILE
echo "Results in $LOG_FILE"
```

## Pass/Fail Criteria

**PASS:** All 100 cycles complete with:
- CRC matches original value every cycle
- No "NVS corruption detected" errors
- No error LED patterns
- Config values unchanged

**FAIL:** Any of the following:
- CRC mismatch on any cycle
- "NVS corruption detected" logged
- Error LED double blink observed
- Config values corrupted

## Test Results

**Test Date:** ____________
**Firmware Version:** ____________
**Tester:** ____________

**Summary:**
- Cycles Completed: _____ / 100
- CRC Matches: _____ / 100
- Corruptions Detected: _____
- Result: PASS / FAIL

**Notes:**
_____________________________________________
_____________________________________________
_____________________________________________

## Troubleshooting

### CRC Mismatch Detected
1. Verify NVS partition not erased during flash
2. Check for power brownouts during write
3. Verify CRC calculation is consistent

### Config Not Found After Reboot
1. Verify nvs_commit() was called after save
2. Check NVS partition table configuration
3. Ensure NVS_NAMESPACE matches

### Error LED Blink on Boot
1. Check serial log for specific error
2. May indicate NVS corruption - run nvs_clear_config()
3. Re-save config and retry test
