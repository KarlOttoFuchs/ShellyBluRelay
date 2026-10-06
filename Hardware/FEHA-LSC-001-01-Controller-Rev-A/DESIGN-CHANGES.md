# FEHA-LSC-001-01 Rev A — Design Changes

**Assembly:** `FEHA-LSC-001-01` Controller · **Baseline:** none (new product; it borrows circuits from
`FEHA-RM-001` Rev A but does not replace it)
**Status:** In design · **Opened:** 2026-10-02

Rev A starts from an empty schematic; there is no baseline to diverge from. Changes are recorded
once the design review raises findings or a settled part of the design is reworked — routine
drawing-in of the initial circuit is design work, not a change. The decisions made before
scaffolding, while this board was still planned as relay-module "Rev B", are the design spec's
decision log; the original notes are archived in
[`../../Docs/design/archive/rev-b-notes-2026-10-02.md`](../../Docs/design/archive/rev-b-notes-2026-10-02.md).

---

## CHG-01 — 100 nF HF bypass at the buck input

**Status:** Implemented · **Severity:** Major · **Origin:** schematic review 2026-10-02, PWR-3
([`schematic-2026-10-02-controller.md`](../../Docs/design/reviews/schematic-2026-10-02-controller.md))

**Problem.** The buck input (`/+24V` at U1 VIN) had only 10 µF 1206 bulk capacitors (C3, C8); TI
asks for a 0.1 µF high-frequency capacitor at the VIN/GND pins (TPS560430 §9.2.2.6).
**Change.** Added C9, 100 nF 50 V X7R 0402 (Samsung CL05B104KB54PNC, C307331, basic), `/+24V` to
GND. C1 (IO9 debounce), C6 (module 3V3) and C7 (bootstrap) moved to the same part number, so all
100 nF positions are one BOM line.
**Impact.** BOM: C1525 line replaced by C307331 (×4), no new fee line. Layout: C9 at U1 pins 5/2, closest of
the input capacitors.
**Verify.** Layout gate: C9 placement against TI §11 rule 1. Bring-up: buck ripple check (spec §10).


## Deferred / Not doing

## Open questions
