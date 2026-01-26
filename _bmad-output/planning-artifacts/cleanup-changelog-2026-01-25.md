# Test Command Cleanup - Production Readiness

**Date:** 2026-01-25
**Status:** ✅ COMPLETED
**Build Status:** ✅ PASSING

---

## Summary

Removed 7 redundant/debug test commands from firmware serial protocol, reducing command count from 16 to 11 production-ready commands. This cleanup improves user experience, reduces confusion, and removes dangerous debug functionality from production firmware.

---

## Changes Made

### Commands REMOVED (7 total)

1. **TEST_LED** - Hardware validation for LED testing
   - **Reason:** Development scaffolding, no production value
   - **Replacement:** LED patterns controlled by state machine

2. **TEST_BUTTON** - Hardware validation for button input
   - **Reason:** Development scaffolding, no production value
   - **Replacement:** Button automatically triggers learning mode

3. **TEST_REGISTER** - Simple MAC registration (no sensor type)
   - **Reason:** Duplicate of REGISTER_SENSOR
   - **Replacement:** Use `REGISTER_SENSOR <MAC> <TYPE>` instead

4. **TEST_UNREGISTER** - Clear registered sensor
   - **Reason:** Duplicate of CLEAR_SENSOR
   - **Replacement:** Use `CLEAR_SENSOR` instead

5. **TEST_SAVE_CONFIG** - Internal NVS config saving with CRC display
   - **Reason:** Internal debug tool, exposes low-level details
   - **Replacement:** Production commands (REGISTER_SENSOR, SET_TIMER, SET_RETRIGGER) handle config automatically

6. **TEST_LOAD_CONFIG** - Internal NVS config loading with CRC display
   - **Reason:** Internal debug tool, exposes low-level details
   - **Replacement:** Use `STATUS` command to view config

7. **TEST_HANG** - **DANGEROUS** - Intentionally hangs firmware to test watchdog
   - **Reason:** Users could accidentally crash their system
   - **Replacement:** Watchdog tested during development, no production use case

### Commands RENAMED (1 total)

**TEST_RELAY → RELAY**
- **Old:** `TEST_RELAY [ON|OFF]`
- **New:** `RELAY [ON|OFF]`
- **Reason:** Cleaner production naming, removes "TEST_" prefix confusion
- **Note:** Fulfills FR32 ("Users can test relay manually via CLI")

---

## Production Commands (11 total)

After cleanup, the firmware has these clean, production-ready commands:

### Core Diagnostics
- **PING** - Test connectivity (responds: pong)
- **STATUS** - Show system state, config, sensor info, firmware version, errors
- **HELP** - Show available commands

### Relay Control
- **RELAY [ON|OFF]** - Manually control relay (renamed from TEST_RELAY)

### Sensor Configuration
- **REGISTER_SENSOR <MAC> <TYPE>** - Register sensor (BUTTON/MOTION/DOOR)
- **CLEAR_SENSOR** - Clear registered sensor configuration

### Timer Configuration
- **SET_TIMER <1-600>** - Set relay timer duration in seconds
- **SET_RETRIGGER [EXTEND|IGNORE]** - Set timer retriggering mode

### BLE Diagnostics
- **BLE_SCAN** - Show recently seen BLE devices
- **BLE_EVENTS** - Show last 10 sensor events

### Error Diagnostics
- **GET_ERRORS** - Show last 10 system errors

---

## Code Impact

### Files Changed
- `Firmware/esp32c3-relay-firmware/components/serial_protocol/serial_protocol.c`
- `Firmware/esp32c3-relay-firmware/components/serial_protocol/serial_protocol.h`

### Statistics
- **Lines Removed:** 333 lines
- **Lines Added:** 22 lines
- **Net Change:** -311 lines (30% reduction in serial_protocol.c)
- **Flash Savings:** ~2.5KB

### Build Verification
```
✅ Build: PASSED
✅ Binary Size: 0x79160 bytes (496KB)
✅ Free Flash: 0x86ea0 bytes (53%)
```

---

## Testing Required

### Pre-Flash Testing (Development Board)
1. ✅ Firmware builds successfully
2. ⚠️ **TODO:** Test renamed command: `RELAY ON` and `RELAY OFF`
3. ⚠️ **TODO:** Verify HELP output shows 11 commands (not 16)
4. ⚠️ **TODO:** Verify removed commands return "Unknown command" error

### Post-Flash Testing (Hardware)
1. ⚠️ **TODO:** Flash firmware to ESP32-C3
2. ⚠️ **TODO:** Run `HELP` and verify clean command list
3. ⚠️ **TODO:** Test RELAY command replaces TEST_RELAY
4. ⚠️ **TODO:** Verify production workflow:
   - `REGISTER_SENSOR AA:BB:CC:DD:EE:FF BUTTON`
   - `SET_TIMER 30`
   - `STATUS` (verify config shows)
   - Trigger sensor → verify relay activates
   - `CLEAR_SENSOR`

---

## Follow-Up Actions

### FUTURE ENHANCEMENT: Comprehensive Hardware Validation Command

**User Request:** "I would not mind a test to just validate that the button works, all the LEDs can at least be powered and that the relay can trigger."

**Proposed Solution:** Create a single `HW_TEST` command that:
- Tests button input (read state)
- Tests both LEDs (status LED and error LED)
- Tests relay (ON → OFF cycle)
- Returns comprehensive PASS/FAIL report

**Advantages:**
- Single command for complete hardware validation
- No conditional compilation needed
- More useful than PING for field testing
- Replaces 3 separate TEST_* commands with 1 comprehensive test

**Epic/Story Reference:** TBD (needs new story created)

**Action Item:** Create new story for `HW_TEST` command implementation

---

## Breaking Changes

### For Users Currently Using TEST_* Commands

**If you use TEST_RELAY:**
- ❌ Old: `TEST_RELAY ON`
- ✅ New: `RELAY ON`

**If you use TEST_REGISTER:**
- ❌ Old: `TEST_REGISTER AA:BB:CC:DD:EE:FF` (MAC only, no type)
- ✅ New: `REGISTER_SENSOR AA:BB:CC:DD:EE:FF BUTTON` (MAC + type required)

**If you use TEST_UNREGISTER:**
- ❌ Old: `TEST_UNREGISTER`
- ✅ New: `CLEAR_SENSOR`

**If you use TEST_SAVE_CONFIG / TEST_LOAD_CONFIG:**
- ❌ Old: Manual config save/load with CRC display
- ✅ New: Config automatically saved by REGISTER_SENSOR, SET_TIMER, SET_RETRIGGER
- ✅ View config: Use `STATUS` command

**If you use TEST_LED / TEST_BUTTON:**
- ❌ Removed (no direct replacement)
- ✅ Wait for new `HW_TEST` command (future enhancement)

**If you use TEST_HANG:**
- ❌ **REMOVED** - This was a dangerous debug command
- ✅ No replacement needed (watchdog tested during development)

---

## Epic 6 (Python CLI) Impact

**Status:** Epic 6 NOT YET IMPLEMENTED

**When implementing Epic 6:**
- Python CLI should use `RELAY ON/OFF` (not TEST_RELAY)
- Update Story 6.3 `test-relay` command to send `RELAY` (not `TEST_RELAY`)
- No other commands affected (REGISTER_SENSOR, SET_TIMER, etc. unchanged)

---

## Conclusion

This cleanup successfully removed 7 test commands, renamed 1 command for production clarity, and reduced code by 311 lines. The firmware now has a clean, professional command interface with 11 production-ready commands.

**Next Steps:**
1. Test renamed RELAY command on hardware
2. Verify HELP output
3. Create new story for comprehensive HW_TEST command
4. Update Epic 6 Python CLI to use RELAY command when implemented

---

**Reviewed By:** Code Review Workflow (Adversarial Mode)
**Approved By:** User (Karl)
**Build Status:** ✅ PASSING (0x79160 bytes, 53% free flash)
