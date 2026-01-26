# Sprint Change Proposal - Door/Window Sensor Retriggering Limitation

**Date:** 2026-01-25
**Project:** ESP32C3 Relay Module
**Submitted by:** Bob (Scrum Master)
**Status:** ✅ APPROVED by Karl

---

## 1. Issue Summary

### Problem Statement

Door/window sensors have fundamentally different event behavior than motion sensors, which creates a limitation with the current binary retriggering modes (EXTEND/IGNORE):

- **Motion sensors**: Can trigger repeatedly while motion continues → EXTEND mode works perfectly (timer resets on each motion event)
- **Door/window sensors**: Binary state changes (OPEN/CLOSE), each triggers once
  - OPEN event should turn light ON and start timer
  - CLOSE event should ideally turn light OFF immediately (cancel timer)
  - **Current behavior**: EXTEND mode treats CLOSE as "retrigger" → resets timer instead of turning light OFF

### Discovery Context

Issue identified during design review of real-world use cases:

1. **Pantry cupboard scenario**: Open door → light ON → close door → **want light OFF immediately** (not timer restart)
2. **Kitchen motion sensor scenario**: Motion detected → light ON → more motion → **want timer to extend** (works correctly)
3. **Timeout scenario**: Pantry door stays open → **timer should expire** and turn light OFF (works correctly)

### Evidence

**Use Case Analysis:**
- Motion sensors: Current EXTEND/IGNORE modes handle all scenarios correctly
- Door sensors: No way to distinguish "new event of same type" from "state change to opposite" (door CLOSE should cancel timer, not restart it)

**Impact Assessment:**
- Severity: Minor
- Workaround: Available (see Section 3)
- MVP blocker: No

---

## 2. Impact Analysis

### Epic Impact

**Current Sprint Status:**
- Epics 1-4B: ✅ Done (hardware, BLE, sensors, relay control, state machine, safety)
- Epic 5: Backlog (Event Logging & Diagnostics)
- Epic 6: Backlog (Python CLI Tool)
- Epic 7: Backlog (Documentation & Educational Quality)

**Epic-Level Changes Required:**
- ✅ **None** - All current epics remain valid and unchanged

**Affected Epics:**
- Epic 5 (Event Logging): ✓ No impact - diagnostics unaffected
- Epic 6 (Python CLI): ⚠ **Minor** - CLI help text should mention sensor type considerations
- Epic 7 (Documentation): ⚠ **Minor** - Add limitation documentation and workaround guidance

**Future Epic Consideration:**
- V1.1 (Multi-Sensor Support): Ideal time to add event-specific action mapping or sensor-type-specific logic

### Artifact Conflicts

**PRD (Product Requirements Document):**
- ✅ **No conflicts** - PRD correctly specifies support for Button, Motion, and Door sensors with EXTEND/IGNORE modes
- Nothing in PRD states that door CLOSE events MUST immediately cancel relay timer
- Current implementation fully meets stated requirements

**Architecture Document:**
- ✅ **No conflicts** - Architecture correctly implements EXTEND/IGNORE retriggering per specification
- ⚠ **Documentation addition needed**: Add "Known Limitations" section explaining door sensor behavior and recommended workaround

**UI/UX Specifications:**
- ✅ N/A - No UI/UX specs (CLI-only project)

**Other Artifacts:**
- ⚠ **Extension Guide** (Epic 7, Story 7.2): Should note event-specific behavior as future enhancement opportunity
- ⚠ **CLI Help Text** (Epic 6, Story 6.3): SET_RETRIGGER command help should mention sensor type considerations

### Story Impact

**Current Stories:**
- ✅ No existing stories require modification
- ✅ No new stories required for MVP

**Story Additions (Documentation Only):**
- Epic 7, Story 7.1: Add "Known Limitations" section to architecture.md
- Epic 7, Story 7.2: Update extension guide with event-specific behavior note
- Epic 6, Story 6.3: Enhance CLI help text for SET_RETRIGGER command

**Technical Impact:**
- Code changes: None
- NVS schema changes: None
- Serial protocol changes: None
- Testing impact: None (no new functionality)

---

## 3. Recommended Approach

### Selected Path: Option D - Document Limitation, Defer to V1.1

**Approach:**
- Keep current EXTEND/IGNORE retriggering implementation as-is for MVP
- Document door/window sensor limitation in architecture documentation
- Provide clear workaround guidance for door sensor use cases
- Defer proper event-specific handling to V1.1 (Multi-Sensor Support)

**Effort Estimate:** Low (documentation updates only)

**Risk Level:** Low (no code changes)

**Timeline Impact:** None (MVP ships on schedule)

### Rationale

**Why defer to V1.1:**

1. **MVP Focus**: Current implementation fully meets core success criteria
   - Marcus's garage motion sensor scenario: ✅ Works perfectly with EXTEND mode
   - Sofia's installer workflow: ✅ Unaffected (primarily motion sensors for office lighting)
   - Priya's learning journey: ✅ Firmware quality and extensibility unchanged

2. **Workaround Exists**: IGNORE mode + appropriate timeout provides functional door sensor control (see below)

3. **Complexity vs. Value**: Adding event-specific logic now adds significant complexity for marginal MVP benefit
   - Would require: NVS schema changes, serial protocol extensions, CLI command additions, state machine modifications
   - Benefit: Improves one use case (pantry cupboard) that already has workable alternative

4. **V1.1 Natural Fit**: Multi-sensor support (V1.1) is the architecturally correct time to add event-specific configuration
   - V1.1 already plans per-sensor configuration
   - Event-specific action mapping fits naturally with multi-sensor registry

### Workaround for Door/Window Sensors (MVP)

**Configuration Recommendation:**

```
Sensor type: DOOR
Retrigger mode: IGNORE
Timer duration: 60 seconds (or user preference)

Behavior:
1. Door OPEN → relay ON → 60-second timer starts
2. Door CLOSE → relay stays ON (timer continues countdown)
3. Timer expires → relay OFF

Result: Light stays on for fixed duration regardless of door state.
```

**Workaround Trade-offs:**
- ✅ **Pros**: Simple, predictable, prevents indefinite ON if door left open
- ⚠ **Cons**: Light doesn't turn off immediately when door closed (waits for timeout)

**User Guidance:**
- Motion sensors: Use EXTEND mode (recommended)
- Door sensors: Use IGNORE mode with appropriate timeout (30-120 seconds typical)
- Document this in CLI help text and architecture guide

### Alternatives Considered (Rejected for MVP)

**Option A: Sensor-Type-Specific Event Logic**
- DOOR sensors: OPEN starts timer, CLOSE immediately turns relay OFF
- Rejected: Hardcodes behavior, reduces flexibility, adds complexity for single use case

**Option B: New Retriggering Mode "EVENT_CANCEL"**
- Add third mode where specific events can cancel relay/timer
- Rejected: Requires NVS schema change, serial protocol extension, more complex configuration

**Option C: Event-Level Configuration in NVS**
- Store per-event actions (DOOR_OPEN → relay ON, DOOR_CLOSE → relay OFF)
- Rejected: Significantly more complex, difficult to configure via CLI, over-engineered for MVP

---

## 4. Detailed Change Proposals

### Documentation Changes (Epic 7)

**Story 7.1: Architecture Documentation Update**

**File:** `docs/architecture.md`

**Section to Add:** "Known Limitations (MVP)"

```markdown
## Known Limitations (MVP)

### Door/Window Sensor Retriggering Behavior

**Limitation:**
Current retriggering modes (EXTEND/IGNORE) are optimized for motion sensors and don't support event-specific actions for door/window sensors.

**Specific Issue:**
- Door CLOSE events cannot immediately cancel relay timer
- Both OPEN and CLOSE events are treated as generic "triggers"
- EXTEND mode: CLOSE event resets timer (undesired for instant-off behavior)
- IGNORE mode: CLOSE event is ignored while relay is ON (acceptable workaround)

**Recommended Workaround:**
For door/window sensors, use IGNORE mode with appropriate timeout:

Configuration:
- Sensor type: DOOR
- Retrigger mode: IGNORE
- Timer duration: 60 seconds (adjust based on use case)

Behavior:
- Door OPEN → relay ON → timer starts
- Door CLOSE → relay stays ON (timer continues)
- Timer expires → relay OFF

Trade-off: Light remains on for fixed duration regardless of door state. Prevents indefinite ON if door left open.

**Future Enhancement (V1.1+):**
Event-specific action mapping or sensor-type-specific logic will be added in V1.1 (Multi-Sensor Support) to enable:
- Door OPEN → relay ON + start timer
- Door CLOSE → relay OFF immediately (cancel timer)
- Motion detected → relay ON + extend timer (current EXTEND behavior)
```

**Story 7.2: Extension Guide Update**

**File:** `docs/adding-sensors.md`

**Section to Add:** Note about event-specific behavior

```markdown
## Future Enhancement: Event-Specific Actions

**Current MVP Limitation:**
All sensor events are treated generically as "triggers" - retriggering modes (EXTEND/IGNORE) apply uniformly to all event types from a sensor.

**Future Enhancement Opportunity (V1.1+):**
Implement event-specific action mapping to support different relay behaviors based on event type:

Example use case:
- DOOR_OPEN event → Action: Turn relay ON, start timer
- DOOR_CLOSE event → Action: Turn relay OFF immediately, cancel timer
- MOTION_DETECTED event → Action: Turn relay ON, extend timer if already ON

**Implementation Approach:**
1. Extend NVS schema to store event-to-action mappings
2. Update serial protocol with EVENT_ACTION command
3. Modify state machine to consult event action map before applying retriggering logic
4. Update CLI to configure event actions

This enhancement would enable intuitive door/window sensor control (instant-off on door close) while maintaining current motion sensor behavior.
```

### CLI Help Text Update (Epic 6)

**Story 6.3: CLI Commands - SET_RETRIGGER Help Text**

**File:** `src/relay_cli/commands/set_retrigger.py`

**Enhanced Help Text:**

```python
@app.command("set-retrigger")
def set_retrigger(
    mode: str = typer.Argument(..., help="Retrigger mode: extend or ignore")
):
    """
    Configure relay retriggering behavior when sensor triggers while relay is already ON.

    MODES:

    extend - Reset timer on each new trigger (recommended for motion sensors)
      Example: Motion sensor in kitchen keeps light on while working
      Behavior: Each motion event resets timer to full duration

    ignore - Ignore new triggers until timer expires (recommended for door sensors)
      Example: Door sensor in pantry cupboard
      Behavior: Door OPEN starts timer, subsequent events ignored
      Note: Door CLOSE will not immediately turn off light in MVP
            (workaround: set appropriate timeout duration)

    SENSOR TYPE CONSIDERATIONS:

    Motion sensors: Use "extend" mode
      - Motion can trigger repeatedly while present
      - Timer extending keeps light on during continuous activity

    Door/Window sensors: Use "ignore" mode with appropriate timeout
      - Door events are binary (OPEN/CLOSE)
      - IGNORE mode prevents timer resets on door state changes
      - Set timer to desired ON duration (e.g., 60 seconds)
      - Note: Light stays on for full timeout even if door closes

    Examples:
      relay-cli set-retrigger extend    # Motion sensor (kitchen)
      relay-cli set-retrigger ignore    # Door sensor (pantry)
    """
```

---

## 5. Implementation Handoff

### Change Scope Classification

**Classification:** Minor (Documentation-only updates)

### Handoff Recipients

**Epic 7 - Documentation Agent (Tech Writer):**
- **Responsibility**: Update architecture.md and extension guide with limitation documentation
- **Deliverables**:
  1. Add "Known Limitations" section to architecture.md
  2. Update extension guide with event-specific behavior future enhancement note
- **Timeline**: During Epic 7 implementation (after Epic 5-6 complete)

**Epic 6 - CLI Development Agent (Dev):**
- **Responsibility**: Enhance SET_RETRIGGER command help text
- **Deliverables**:
  1. Update help text in set_retrigger.py with sensor type considerations
  2. Include workaround guidance for door sensors
- **Timeline**: During Story 6.3 implementation

**Scrum Master (Bob):**
- **Responsibility**: Update sprint-status.yaml with this decision, ensure Epic 7 acceptance criteria include limitation documentation
- **Deliverables**:
  1. Archive this Sprint Change Proposal
  2. Confirm Epic 7 stories include documentation updates
- **Timeline**: Immediate

### Success Criteria

**This change is considered successful when:**

1. ✅ Architecture.md includes "Known Limitations" section with door sensor workaround
2. ✅ Extension guide notes event-specific behavior as future enhancement
3. ✅ CLI SET_RETRIGGER help text explains sensor type considerations
4. ✅ Users understand door sensor limitation and workaround via documentation
5. ✅ MVP ships on schedule with no code changes required

---

## 6. Workflow Completion Summary

### Issue Addressed
Door/window sensors don't support "close event cancels timer" behavior due to binary EXTEND/IGNORE retriggering modes designed for motion sensors.

### Change Scope
**Minor** - Documentation updates only, no code changes

### Artifacts Modified
1. Architecture.md - Add "Known Limitations" section
2. Extension guide - Add event-specific behavior note
3. CLI help text - Enhance SET_RETRIGGER command guidance

### Routed To
- **Tech Writer** (Epic 7): Documentation updates
- **Dev** (Epic 6): CLI help text enhancement
- **Scrum Master**: Sprint tracking and Epic 7 acceptance criteria verification

### Decision Rationale
Deferring proper event-specific handling to V1.1 keeps MVP focused, ships on schedule, and addresses the issue at the architecturally correct time (multi-sensor support). Workaround provides functional door sensor control for MVP users.

---

**✅ Correct Course workflow complete, Karl!**

**Next Steps:**
1. This proposal is archived at: `_bmad-output/planning-artifacts/sprint-change-proposal-2026-01-25.md`
2. Epic 7 implementation will include the documentation updates
3. Epic 6 Story 6.3 will include CLI help text enhancement
4. MVP continues as planned - no delays, no scope changes

**V1.1 Enhancement Tracking:**
Consider adding to V1.1 backlog:
- Event-specific action mapping (DOOR_CLOSE → relay OFF immediately)
- Per-event configuration in NVS schema
- Enhanced CLI for event action configuration
