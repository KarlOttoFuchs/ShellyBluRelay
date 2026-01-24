# Sprint Change Proposal: Remove Firmware Unit Tests

**Date:** 2026-01-24
**Status:** Approved
**Scope:** Minor

---

## 1. Issue Summary

**Problem Statement:**
ESP32-C3 embedded firmware unit tests (ESP-IDF Unity framework) require compilation and execution on hardware, unlike traditional TDD unit tests that run locally. The architecture mandated unit tests following TDD patterns, but actual validation occurs through direct hardware testing. Unit tests have been compiled/run only 2 times across 13 completed stories, consuming development tokens without providing value.

**Discovery Context:**
Observed during ongoing Epic 4a implementation after completing Epics 1-3. Pattern consistent across all firmware development.

**Evidence:**
- 15 test files created but rarely compiled/run
- Primary validation method: direct hardware testing via serial commands
- Token consumption for test creation without corresponding value

---

## 2. Impact Analysis

### Epic Impact
- No epic scope changes required
- Epic 4a-4 (backlog): will be created without unit test mandate
- Epics 4b, 5, 7: future stories exclude firmware unit tests
- Epic 6 (CLI): pytest tests **retained** (run locally without hardware)

### Artifact Conflicts

| Artifact | Conflict | Resolution |
|----------|----------|------------|
| PRD (lines 781-786) | Mandates "Unit Testing" | Replace with "Firmware Validation" |
| Architecture.md | Unity test framework, test/ structure, test patterns | Remove all firmware unit test references |
| Firmware codebase | 15 test files, test_app directory | Delete |

### Technical Impact
- No code changes to production firmware
- No functionality changes
- Component CMakeLists.txt files are clean (no test references)

---

## 3. Recommended Approach

**Selected:** Direct Adjustment

**Rationale:**
1. Lowest effort - documentation updates + file deletion
2. Zero risk to functionality
3. Reduces future token consumption
4. Aligns documentation with actual embedded development practice
5. CLI pytest tests preserved (they provide value for Python code)

**Alternatives Considered:**
- Rollback: Not viable - tests aren't blocking anything
- MVP Review: Not needed - MVP unaffected

**Effort Estimate:** Low
**Risk Assessment:** Low
**Timeline Impact:** Positive (saves time on future stories)

---

## 4. Detailed Change Proposals

### 4.1 PRD Update (lines 781-786)

**Section:** Testing Strategy

**OLD:**
```markdown
**Testing Strategy:**
- **Hardware Validation**: Test every interface (LEDs, relay, button, BLE scanning, USB serial) with simple test firmware before architecture implementation
- **Unit Testing**: BTHome parser unit tests with real packet captures from Shelly BLU devices
- **Integration Testing**: End-to-end workflows matching user journeys (Marcus, Sofia, Priya scenarios)
- **Error Recovery Testing**: NVS corruption, firmware crashes, multiple port detection, config validation failures
- **Load Testing**: 24-hour continuous operation, 1000+ trigger cycles, sensor battery health monitoring
```

**NEW:**
```markdown
**Testing Strategy:**
- **Hardware Validation**: Test every interface (LEDs, relay, button, BLE scanning, USB serial) with test firmware before architecture implementation
- **Firmware Validation**: Direct hardware testing of all functionality using serial commands and physical sensor triggers
- **CLI Testing**: pytest unit tests for Python CLI tool (runs locally without hardware)
- **Integration Testing**: End-to-end workflows matching user journeys (Marcus, Sofia, Priya scenarios)
- **Error Recovery Testing**: NVS corruption, firmware crashes, multiple port detection, config validation failures
- **Load Testing**: 24-hour continuous operation, 1000+ trigger cycles, sensor battery health monitoring
```

**Rationale:** Aligns documented testing strategy with actual embedded development practice.

---

### 4.2 Architecture.md Updates

**Change 1 - Testing Framework Section (lines 206-214):**

Remove ESP-IDF Unity framework reference, update to hardware validation approach.

**Change 2 - Firmware Project Structure (lines 941-948):**

Remove `test/` directory from project structure diagram.

**Change 3 - Testing Patterns Section (lines 1222-1258):**

Remove Unity test examples, keep pytest examples for CLI.

---

### 4.3 File Deletions

**Directories to delete:**
```
Firmware/esp32c3-relay-firmware/test/
Firmware/esp32c3-relay-firmware/test_app/
Firmware/esp32c3-relay-firmware/components/button_input/test/
Firmware/esp32c3-relay-firmware/components/ble_scanner/test/
Firmware/esp32c3-relay-firmware/components/led_control/test/
Firmware/esp32c3-relay-firmware/components/bthome_parser/test/
Firmware/esp32c3-relay-firmware/components/relay_control/test/
Firmware/esp32c3-relay-firmware/components/serial_protocol/test/
```

**Files (15 total):**
- test_button_input.c (2 copies)
- test_led_control.c (2 copies)
- test_ble_scanner.c (2 copies)
- test_bthome_parser.c
- test_sensor_button.c
- test_sensor_motion.c
- test_sensor_door.c
- test_nvs_storage.c
- test_serial_protocol.c (2 copies)
- test_relay_control.c
- test_relay_control.h
- test_main.c

---

## 5. Implementation Handoff

**Scope Classification:** Minor - Direct implementation

| # | Action | Owner | Status |
|---|--------|-------|--------|
| 1 | Update PRD Testing Strategy section | SM | Pending |
| 2 | Update Architecture.md testing patterns | SM | Pending |
| 3 | Delete firmware test directories | Manual/Dev | Pending |
| 4 | Verify build still works | Dev | Pending |
| 5 | Create future stories without unit test ACs | SM | Ongoing |

**Success Criteria:**
- PRD and Architecture.md updated
- All test directories deleted
- Firmware builds successfully
- Future stories do not mandate firmware unit tests
- CLI pytest requirements unchanged

---

**Approved by:** Karl
**Date:** 2026-01-24
