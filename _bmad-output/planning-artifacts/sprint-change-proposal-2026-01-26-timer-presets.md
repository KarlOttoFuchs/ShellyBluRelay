# Sprint Change Proposal - Button-Based Timer Preset Feature

**Date:** 2026-01-26
**Project:** ESP32C3 Relay Module
**Submitted by:** Karl (Product Owner)
**Status:** APPROVED

---

## 1. Feature Summary

### Feature Request

Add a button-based timer preset cycling feature that allows users to adjust relay timer duration without requiring serial terminal access. This enables field adjustment of timer settings for devices installed in difficult-to-access locations (e.g., under kitchen counters).

### Context & Motivation

**Installation Scenario:**
- Primary use cases: Under-counter LED lighting (motion sensor, 5 min timeout) and pantry cupboard lighting (door sensor, 5 min timeout)
- Device location: Under kitchen counter - USB port accessible but inconvenient
- Configuration frequency: Rare (configure once, occasional adjustments)
- Current limitation: Timer changes require serial terminal connection

**User Feedback:**
- Python CLI (Epic 6) value is questionable for this use case
- Serial terminal via USB-C is sufficient for rare configuration changes
- Button-based adjustment would eliminate need for laptop/terminal for simple timer changes

### Feature Description

**Short-press button cycles through timer presets:**
- Presets: 30s → 1m → 2m → 5m → wrap to 30s
- LED feedback: Status LED (white) blinks N times to indicate current preset
- 3-second timeout confirms selection and saves to NVS

**Interaction Flow:**
```
[Device in LISTENING state - Status LED slow blinking]

User short-presses button
  → Status LED: N quick blinks (shows current preset position)
  → 3-second window starts for next press

User short-presses again within 3 sec
  → Advances to next preset
  → Status LED: N+1 quick blinks

User waits 3 sec (no press)
  → Saves new timer value to NVS
  → Returns to normal LED pattern (slow blink)
```

**Preset Mapping:**
| Blinks | Duration | Use Case |
|--------|----------|----------|
| 1 | 30 sec | Quick task |
| 2 | 1 min | Brief access |
| 3 | 2 min | Short activity |
| 4 | 5 min | Default - pantry/counter lighting |

---

## 2. Related Changes

### Default Timer Duration Change

**Current:** `DEFAULT_TIMER_SECONDS = 30`
**Proposed:** `DEFAULT_TIMER_SECONDS = 300` (5 minutes)

**Rationale:**
- Primary use cases (motion-activated under-counter LEDs, pantry cupboard lights) require longer timeout
- 30 seconds is too short for typical kitchen/pantry activities
- 5 minutes aligns with user's stated requirements
- Learning mode will register sensors with sensible default out of the box

### Retrigger Mode Default

**Current:** `DEFAULT_RETRIGGER_MODE = RETRIGGER_EXTEND` (0)
**Proposed:** No change - keep EXTEND as default

**Rationale:**
- EXTEND works correctly for motion sensors (timer resets on continued motion)
- EXTEND is harmless for door sensors (no retrigger events occur anyway)
- Per Sprint Change Proposal 2026-01-25, door sensor limitation is documented and deferred to V1.1

---

## 3. Impact Analysis

### Epic Impact

**Current Sprint Status:**
- Epics 1-4B: Done (hardware, BLE, sensors, relay control, state machine, safety)
- Epic 5: Done (Event Logging & Diagnostics)
- Epic 6: Backlog (Python CLI Tool) - **value now questionable**
- Epic 7: Backlog (Documentation & Educational Quality)

**Epic-Level Changes:**

| Epic | Impact | Description |
|------|--------|-------------|
| Epic 1 | **Minor** | Add short-press detection, counted LED blinks |
| Epic 3 | **Minor** | Default timer change (30→300) |
| Epic 4A | **None** | Timer logic unchanged |
| Epic 6 | **Reconsider** | CLI may be deprioritized given button-based config |
| Epic 7 | **Minor** | Document button sequences in user guide |

### Artifact Conflicts

**PRD (Product Requirements Document):**
- FR19 (timer 1-600 seconds): Compatible - presets are within range
- FR2 (button input): Compatible - adds new button interaction
- No conflicts - feature extends existing capabilities

**Architecture Document:**
- Button handling: Requires short-press detection (currently only long-press)
- LED patterns: Requires new "counted blinks" pattern
- NVS: No schema changes (timer_seconds field exists)
- State machine: No changes (preset adjustment works in LISTENING state)

### Story Impact

**New Stories Required:**

**Story 1.7: Button Short-Press Detection**
- Add short-press detection alongside existing long-press
- Short press = release within 500ms of press
- Long press = 2 seconds (unchanged)

**Story 1.8: LED Counted Blink Function**
- Add `led_blink_count(led_id, count)` function
- Uses Status LED (white, GPIO10)
- Timing: 150ms on, 150ms off per blink
- Returns LED to previous pattern after blinks complete

**Story 1.9: Timer Preset Cycling Logic**
- Implement preset array: {30, 60, 120, 300} seconds
- Track current preset position
- 3-second timeout for confirmation
- Save to NVS on timeout

**Modified Stories:**

**Story 3.1: NVS Storage Component**
- Change `DEFAULT_TIMER_SECONDS` from 30 to 300
- No schema changes required

---

## 4. Detailed Implementation Plan

### Component Changes

#### 1. button_input.c - Add Short-Press Detection

**New Function:**
```c
/**
 * Check for button short press (<500ms)
 * Non-blocking, call from main loop
 * Returns true ONCE when short press detected
 */
bool button_check_short_press(void);
```

**Implementation Notes:**
- Track press start time (already done for long press)
- On release, check duration:
  - < 500ms = short press
  - >= 2000ms = long press (existing)
  - 500-2000ms = ignored (debounce zone)

#### 2. led_control.c - Add Counted Blink Function

**New Function:**
```c
/**
 * Blink LED a specific number of times
 * @param led LED_STATUS or LED_ERROR
 * @param count Number of blinks (1-10)
 * @param callback Optional callback when complete
 */
esp_err_t led_blink_count(led_id_t led, uint8_t count, void (*callback)(void));
```

**Implementation Notes:**
- 150ms on, 150ms off timing (matches error pattern timing)
- Non-blocking (uses existing LED task)
- Saves and restores previous pattern after blinks complete
- Use Status LED (white) for user feedback

#### 3. New Component: timer_presets.c

**Location:** `components/timer_presets/`

**API:**
```c
// Initialize timer preset module
esp_err_t timer_presets_init(void);

// Called from main loop - handles button press and timeout
void timer_presets_process(void);

// Get current preset index (0-3)
uint8_t timer_presets_get_current_index(void);

// Get timer value for preset index
uint16_t timer_presets_get_value(uint8_t index);
```

**Preset Array:**
```c
static const uint16_t TIMER_PRESETS[] = {30, 60, 120, 300};
#define TIMER_PRESET_COUNT 4
#define TIMER_PRESET_DEFAULT_INDEX 3  // 300 seconds (5 min)
```

**State Machine:**
```
IDLE
  ↓ [short press]
SHOWING_CURRENT
  → LED blinks N times (current preset)
  → Start 3-second timeout
  ↓ [short press within 3 sec]
CYCLING
  → Advance to next preset (wrap at end)
  → LED blinks N times (new preset)
  → Reset 3-second timeout
  ↓ [3 sec timeout]
SAVING
  → Save timer_seconds to NVS
  → Log "Timer set to X seconds"
  → Return to IDLE
```

#### 4. nvs_storage.h - Default Change

**Change:**
```c
// Before
#define DEFAULT_TIMER_SECONDS   30

// After
#define DEFAULT_TIMER_SECONDS   300
```

#### 5. main.c - Integration

**Add to main loop:**
```c
// In app_main() task loop
while (1) {
    // Existing: long press check for learning mode
    if (button_check_long_press()) {
        learning_mode_start();
    }

    // New: timer preset processing
    timer_presets_process();

    // ... rest of loop
}
```

### File Changes Summary

| File | Change Type | Description |
|------|-------------|-------------|
| `button_input.c` | Modify | Add `button_check_short_press()` |
| `button_input.h` | Modify | Add function declaration |
| `led_control.c` | Modify | Add `led_blink_count()` |
| `led_control.h` | Modify | Add function declaration |
| `nvs_storage.h` | Modify | Change DEFAULT_TIMER_SECONDS 30→300 |
| `components/timer_presets/` | **New** | Timer preset cycling component |
| `main.c` | Modify | Integrate timer_presets_process() |

### Estimated Effort

| Task | Effort |
|------|--------|
| Short-press detection | 1-2 hours |
| LED counted blinks | 1-2 hours |
| Timer presets component | 2-3 hours |
| Integration & testing | 1-2 hours |
| Documentation | 1 hour |
| **Total** | **6-10 hours** |

---

## 5. Acceptance Criteria

### Story 1.7: Button Short-Press Detection

**Given** firmware is running in LISTENING state
**When** user presses and releases button within 500ms
**Then** `button_check_short_press()` returns true once

**And** short press does NOT trigger learning mode (long press only)

**And** presses between 500ms-2000ms are ignored (neither short nor long)

### Story 1.8: LED Counted Blink Function

**Given** Status LED is in slow blink pattern (LISTENING state)
**When** `led_blink_count(LED_STATUS, 4, NULL)` is called
**Then** Status LED produces 4 quick blinks (150ms on/off each)

**And** after blinks complete, LED returns to slow blink pattern

**And** blinks are visually distinct (not confused with error patterns)

### Story 1.9: Timer Preset Cycling

**Given** device is configured with sensor and in LISTENING state
**When** user short-presses button
**Then** Status LED blinks N times indicating current preset position

**When** user short-presses again within 3 seconds
**Then** preset advances to next value (wraps at end)
**And** Status LED blinks N times for new preset

**When** 3 seconds pass without button press
**Then** new timer value is saved to NVS
**And** serial log shows "Timer set to X seconds"
**And** LED returns to normal pattern

**Preset Values:**
- 1 blink = 30 seconds
- 2 blinks = 1 minute (60 seconds)
- 3 blinks = 2 minutes (120 seconds)
- 4 blinks = 5 minutes (300 seconds)

### Default Timer Change

**Given** device has no configuration (fresh NVS)
**When** sensor is registered via learning mode
**Then** timer_seconds defaults to 300 (5 minutes)

**And** STATUS command shows "timer_duration_sec": 300

---

## 6. Testing Plan

### Manual Test Procedures

**Test 1: Short Press Detection**
1. Boot device, enter LISTENING state
2. Quick press and release button (<500ms)
3. Verify LED shows current preset (should be 4 blinks for 5min default)
4. Verify learning mode NOT entered

**Test 2: Preset Cycling**
1. Short press → 4 blinks (5 min)
2. Short press within 3 sec → 1 blink (30 sec, wrapped)
3. Short press within 3 sec → 2 blinks (1 min)
4. Wait 3 seconds
5. Verify STATUS shows timer_duration_sec: 60

**Test 3: Persistence**
1. Set timer to 2 minutes via button (3 blinks)
2. Power cycle device
3. Verify STATUS shows timer_duration_sec: 120
4. Short press → should show 3 blinks (remembered position)

**Test 4: Long Press Still Works**
1. Press and hold button for 2+ seconds
2. Verify learning mode entered (fast blink)
3. Verify short press during learning mode is ignored

**Test 5: Default Timer Change**
1. Clear NVS (CLEAR_SENSOR command)
2. Enter learning mode, register sensor
3. Verify STATUS shows timer_duration_sec: 300 (not 30)

---

## 7. Documentation Updates

### User Documentation (Epic 7)

**Add to hardware-validation.md or new user-guide.md:**

```markdown
## Button Functions

### Long Press (2+ seconds) - Enter Learning Mode
Hold button for 2 seconds to enter sensor learning mode.
Status LED will fast blink. Trigger your sensor within 30 seconds.

### Short Press (<500ms) - Adjust Timer Duration
Quick press to cycle through timer presets:

| LED Blinks | Timer Duration |
|------------|----------------|
| 1 blink | 30 seconds |
| 2 blinks | 1 minute |
| 3 blinks | 2 minutes |
| 4 blinks | 5 minutes (default) |

**To adjust timer:**
1. Short press button - LED shows current setting
2. Short press again within 3 seconds to advance to next preset
3. Wait 3 seconds to confirm and save

**Example:** Change from 5 minutes to 1 minute:
1. Press → 4 blinks (current: 5 min)
2. Press → 1 blink (30 sec)
3. Press → 2 blinks (1 min)
4. Wait 3 seconds → saved
```

### Architecture Documentation

**Add to architecture.md:**

```markdown
## Button Input Modes

The physical button (GPIO9) supports two input modes:

| Mode | Duration | Action |
|------|----------|--------|
| Short press | <500ms | Cycle timer presets |
| Long press | ≥2000ms | Enter learning mode |

Presses between 500ms and 2000ms are ignored (debounce zone).

## Timer Presets

Timer duration can be adjusted via button without serial connection:

Presets: 30s, 60s, 120s, 300s (default)

Adjustment uses Status LED (white) for feedback:
- N blinks indicates preset position (1-4)
- 3-second timeout confirms selection
- Value persists in NVS
```

---

## 8. Epic 6 Reconsideration

### Current Epic 6 Scope

Python CLI with commands: status, register-sensor, set-timer, set-retrigger, test-relay, clear-sensor, logs

### Impact of This Feature

With button-based timer adjustment:
- **set-timer**: Now available via button (partial replacement)
- **status**: Still valuable for diagnostics
- **register-sensor**: Learning mode handles primary use case
- **test-relay**: Serial command still available
- **clear-sensor**: Serial command still available
- **logs**: Serial command still available

### Recommendation

**Option A: Deprioritize Epic 6 entirely**
- Serial terminal (minicom, screen, PuTTY) sufficient for rare config needs
- Button handles the most common adjustment (timer)
- Reduces project scope significantly

**Option B: Implement minimal CLI (status + logs only)**
- Pretty-printed status for diagnostics
- Formatted log viewing
- Skip registration/configuration commands

**Option C: Proceed with Epic 6 as planned**
- Full CLI provides better UX for power users
- Supports future multi-sensor configuration (V1.1)

**Suggested:** Defer Epic 6 decision to after timer preset feature is implemented and tested. User can evaluate if serial terminal is truly sufficient.

---

## 9. Approval & Next Steps

### Approval Required

- [x] **Product Owner (Karl):** Approve feature scope and default changes
- [x] **Architecture:** Confirm component design is appropriate

### Implementation Sequence

1. **Story 1.7:** Button short-press detection
2. **Story 1.8:** LED counted blink function
3. **Story 1.9:** Timer preset cycling component
4. **Story 3.1 (update):** Change DEFAULT_TIMER_SECONDS to 300
5. **Integration:** Main loop updates
6. **Testing:** Manual test procedures
7. **Documentation:** User guide and architecture updates

### Success Criteria

This change is considered successful when:

1. Short press cycles through 4 timer presets with LED feedback
2. Long press still enters learning mode (no regression)
3. Timer selection persists across power cycles
4. Default timer is 300 seconds for new registrations
5. User can adjust timer without laptop/serial connection
6. Documentation explains button functions clearly

---

**Workflow Status:** APPROVED

**Next Steps after Approval:**
1. Create feature branch
2. Implement stories 1.7, 1.8, 1.9 in sequence
3. Update nvs_storage.h default
4. Test all acceptance criteria
5. Update documentation
6. Create PR for review
