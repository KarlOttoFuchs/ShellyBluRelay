# Sprint Change Proposal: Defer Epic 6 (Python CLI Tool) to V1.1

**Date:** 2026-01-27
**Requested By:** Karl
**Change Scope:** Moderate (backlog reorganization, document updates)
**Status:** APPROVED & IMPLEMENTED

---

## Section 1: Issue Summary

### Problem Statement

The Python CLI Tool (Epic 6, FR27-FR32) provides enhanced user convenience through auto-port detection, Rich-formatted tables, and auto-generated help. However, this functionality is **not essential for MVP value delivery**.

The firmware already implements a complete serial protocol with all commands:
- `STATUS` - Comprehensive system status (JSON)
- `REGISTER_SENSOR` - Sensor registration
- `SET_TIMER` / `SET_RETRIGGER` - Configuration
- `TEST_RELAY` - Manual relay control
- `GET_EVENTS` / `GET_ERRORS` - Diagnostics
- `HELP` - Command reference

These commands are accessible via any serial terminal (PuTTY, screen, minicom, Arduino Serial Monitor), providing full functionality without the Python CLI wrapper.

### Context

- **When Discovered:** During sprint planning review (2026-01-27)
- **Trigger:** Strategic decision to accelerate V1.0 firmware release
- **Evidence:**
  - Epics 1-5 (firmware) complete and validated
  - Epic 6 has 0/3 stories started
  - Serial protocol fully documented and working
  - Users can configure via terminal as documented in PRD user journeys

### Rationale for Deferral

1. **Firmware delivers core value** - BLE sensor → relay automation works without CLI
2. **Serial terminal is viable** - All commands accessible, JSON responses parseable
3. **Reduces MVP scope** - Focus on firmware stability and documentation
4. **CLI can be V1.1** - Natural enhancement after firmware proves reliable

---

## Section 2: Impact Analysis

### Epic Impact

| Epic | Current Status | Impact | Action |
|------|----------------|--------|--------|
| Epic 1 | done | None | No change |
| Epic 2 | done | None | No change |
| Epic 3 | done | None | No change |
| Epic 4A | done | None | No change |
| Epic 4B | done | None | No change |
| Epic 5 | done | None | No change |
| **Epic 6** | **backlog** | **DEFER** | Mark as "deferred" in sprint-status.yaml |
| Epic 7 | backlog | Minor | Story 7.4 marked N/A (depends on CLI) |

### Story Impact

**Epic 6 Stories (All Deferred):**
- 6.1: CLI Project Structure with Typer + Rich Frameworks → DEFERRED
- 6.2: Serial Port Auto-Detection with Manual Fallback → DEFERRED
- 6.3: Implement Core CLI Commands → DEFERRED

**Epic 7 Stories:**
- 7.1: Architecture Documentation → No change
- 7.2: Extension Guide → No change
- 7.3: Inline Comments → No change
- 7.4: CLI Auto-Generated Help → **N/A** (depends on CLI existing)
- 7.5: Firmware Readability Review → No change

### Artifact Conflicts

| Artifact | Section | Conflict | Resolution |
|----------|---------|----------|------------|
| PRD | MVP Feature List | FR27-FR32 in MVP | Move to Post-MVP V1.1 |
| PRD | Success Criteria | CLI references | Add note about terminal access |
| Epics | Epic 6 | In MVP scope | Add DEFERRED header |
| Epics | Story 7.4 | Depends on CLI | Mark N/A |
| Sprint Status | epic-6 | Status: backlog | Status: deferred |
| Architecture | CLI sections | Documented for MVP | Retain for V1.1 (no removal) |

### Technical Impact

- **Code Changes:** None required - firmware complete
- **Infrastructure:** None - no CLI deployment needed
- **Deployment:** Simpler - firmware-only release
- **Testing:** Reduced scope - no CLI test suite needed for V1.0

---

## Section 3: Recommended Approach

### Selected Path: Direct Adjustment + MVP Scope Reduction

**Rationale:**
- Zero code changes required
- Firmware functionality unaffected
- Documentation updates only
- Clear V1.1 roadmap for CLI

**Effort Estimate:** Low (2-3 hours document updates)

**Risk Assessment:** Low
- No regression risk (no code changes)
- User workflows still supported via serial terminal
- Architecture decisions preserved for V1.1

**Timeline Impact:** Positive
- Removes Epic 6 (estimated 20-28 hours per PRD)
- Accelerates V1.0 release
- Epic 7 (Documentation) becomes final MVP epic

### Alternatives Considered

| Alternative | Assessment | Why Not Selected |
|-------------|------------|------------------|
| Keep CLI in MVP | Adds 20-28 hours | Delays V1.0, not essential |
| Remove CLI entirely | Loses planned feature | V1.1 value for user experience |
| Partial CLI (status only) | Scope creep risk | Better to defer completely |

---

## Section 4: Detailed Change Proposals

### Change 1: sprint-status.yaml

**File:** `_bmad-output/implementation-artifacts/sprint-status.yaml`

**OLD (lines 99-103):**
```yaml
  epic-6: backlog
  6-1-cli-project-structure-with-typer-rich-frameworks: backlog
  6-2-serial-port-auto-detection-with-manual-fallback: backlog
  6-3-implement-core-cli-commands-status-register-sensor-set-timer-test-relay-clear-sensor-logs: backlog
  epic-6-retrospective: optional
```

**NEW:**
```yaml
  # Epic 6 DEFERRED to V1.1 (Sprint Change Proposal 2026-01-27)
  # Rationale: CLI provides convenience but serial terminal access delivers core value
  # Firmware serial protocol complete - users can configure via PuTTY/screen/minicom
  epic-6: deferred
  6-1-cli-project-structure-with-typer-rich-frameworks: deferred
  6-2-serial-port-auto-detection-with-manual-fallback: deferred
  6-3-implement-core-cli-commands-status-register-sensor-set-timer-test-relay-clear-sensor-logs: deferred
  epic-6-retrospective: deferred
```

**Rationale:** Introduces "deferred" status to distinguish from "backlog" (planned but not started) vs "deferred" (explicitly postponed to future release).

---

### Change 2: sprint-status.yaml - Story 7.4

**File:** `_bmad-output/implementation-artifacts/sprint-status.yaml`

**OLD (line 109):**
```yaml
  7-4-implement-cli-auto-generated-help-system: backlog
```

**NEW:**
```yaml
  # Story 7.4 N/A - depends on CLI (Epic 6) which is deferred to V1.1
  7-4-implement-cli-auto-generated-help-system: not-applicable
```

**Rationale:** Story 7.4 is about CLI help system - cannot be implemented without CLI.

---

### Change 3: epics.md - Epic 6 Header

**File:** `_bmad-output/planning-artifacts/epics.md`

**OLD (line 280):**
```markdown
### Epic 6: Python CLI Tool - Core Commands
```

**NEW:**
```markdown
### Epic 6: Python CLI Tool - Core Commands [DEFERRED TO V1.1]

> **⚠️ DEFERRED:** This epic has been postponed to V1.1 per Sprint Change Proposal (2026-01-27).
>
> **Rationale:** The firmware serial protocol (Epic 5) provides complete command access via any serial terminal. The CLI adds convenience (auto-port detection, Rich formatting, auto-generated help) but is not essential for MVP value delivery.
>
> **User Access:** Until CLI is available, users can access all commands via serial terminal (PuTTY, screen, minicom) at 115200 baud. See `docs/serial-protocol.md` for command reference.
```

**Rationale:** Clear visibility that Epic 6 is intentionally deferred, not forgotten.

---

### Change 4: epics.md - Story 7.4

**File:** `_bmad-output/planning-artifacts/epics.md`

**OLD (lines 1900-1945):**
```markdown
### Story 7.4: Implement CLI Auto-Generated Help System
...
```

**NEW:**
```markdown
### Story 7.4: Implement CLI Auto-Generated Help System [NOT APPLICABLE - V1.1]

> **⚠️ NOT APPLICABLE:** This story depends on Epic 6 (Python CLI Tool) which has been deferred to V1.1.
>
> This story will be implemented when Epic 6 is developed in V1.1.

~~As a user,
I want comprehensive auto-generated help for all CLI commands,
So that I can learn how to use the CLI without reading separate documentation.~~

**Deferred to:** V1.1 (when Epic 6 CLI is implemented)
```

**Rationale:** Story depends on CLI existence.

---

### Change 5: prd.md - MVP Feature List

**File:** `_bmad-output/planning-artifacts/prd.md`

**OLD (lines 122):**
```markdown
10. **Python CLI (Core Commands)** - status, test-relay, register-sensor, set-timer, set-retrigger, logs, clear-sensor
```

**NEW:**
```markdown
10. ~~**Python CLI (Core Commands)**~~ → **DEFERRED TO V1.1** - Serial terminal access provides equivalent functionality via firmware commands (STATUS, REGISTER_SENSOR, SET_TIMER, etc.)
```

**Rationale:** Removes CLI from MVP checklist while preserving traceability.

---

### Change 6: prd.md - FR27-FR32 Scope

**File:** `_bmad-output/planning-artifacts/prd.md`

**ADD after line 1064 (after FR26):**
```markdown
### CLI Configuration Interface (FR27-FR32) [DEFERRED TO V1.1]

> **⚠️ DEFERRED:** Requirements FR27-FR32 have been moved to Post-MVP V1.1 per Sprint Change Proposal (2026-01-27).
>
> **Interim Solution:** All CLI functionality is accessible via direct serial terminal commands at 115200 baud. The firmware implements complete serial protocol (STATUS, REGISTER_SENSOR, SET_TIMER, etc.) usable with PuTTY, screen, minicom, or Arduino Serial Monitor.
```

**Rationale:** Clear documentation that CLI requirements are deferred, not removed.

---

## Section 5: Implementation Handoff

### Change Scope Classification: **MODERATE**

This change requires backlog reorganization and document updates, not code changes.

### Handoff Recipients

| Recipient | Responsibility |
|-----------|----------------|
| **Scrum Master (Bob)** | Update sprint-status.yaml with new statuses |
| **Scrum Master (Bob)** | Update epics.md with deferral notices |
| **Product Owner** | Approve PRD scope changes |
| **Development Team** | No code changes required |

### Implementation Steps

1. **Update sprint-status.yaml** - Add "deferred" status for Epic 6 and stories
2. **Update sprint-status.yaml** - Mark Story 7.4 as "not-applicable"
3. **Update epics.md** - Add DEFERRED header to Epic 6
4. **Update epics.md** - Add N/A notice to Story 7.4
5. **Update prd.md** - Move CLI from MVP to Post-MVP
6. **Update prd.md** - Add deferral note to FR27-FR32 section
7. **Verify sprint-status.yaml** - Regenerate if needed to confirm Epic 7 is next

### Success Criteria

- [ ] sprint-status.yaml shows Epic 6 as "deferred"
- [ ] sprint-status.yaml shows Story 7.4 as "not-applicable"
- [ ] epics.md clearly marks Epic 6 as deferred to V1.1
- [ ] prd.md MVP section reflects CLI deferral
- [ ] Epic 7 (Documentation) is ready to proceed as final MVP epic

---

## Approval

**Proposed By:** Bob (Scrum Master)
**Date:** 2026-01-27

**Approval Status:** APPROVED

- [x] Karl (Product Owner) - Approved scope change (2026-01-27)
- [x] Implementation team - Acknowledged no code changes needed

**Implementation Completed:** 2026-01-27
All document updates applied successfully.

---

*Generated by Correct Course Workflow*
