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

## CHG-02 — Debug header in standard USB pin order

**Status:** Implemented · **Severity:** Major · **Origin:** schematic review 2026-10-02, FUNC-7
([`schematic-2026-10-02-controller.md`](../../Docs/design/reviews/schematic-2026-10-02-controller.md))

**Problem.** The debug header had D+ and D− in the reverse of the standard USB order, so the bench
pigtail (5V, D−, D+, GND) would cross the data pair and the board would not enumerate.
**Change.** Header pins 2 and 3 swapped: pin 2 is D−, pin 3 is D+ (spec §6).
**Impact.** None outside this board; the header is DNP and bench-only.
**Verify.** Bring-up: board enumerates as USB-Serial-JTAG through the pigtail and flashes (spec §10).


## CHG-03 — Operating limits on the top silkscreen

**Status:** Implemented · **Severity:** Minor · **Origin:** Karl, 2026-10-07 (after the layout gate)

**Problem.** The board carried no installer-facing statement of its limits; the supply
requirement (CON-7) is the board's only overcurrent protection and lived only in the spec.
**Change.** Three F.SilkS texts, 1.2 mm bold, 0.24 mm stroke (matching the IN/OUT labels),
left/top justified: `24V DC` / `ONLY` at (101.33, 101.33), top-left by CN1 (SELV input);
`LOAD MAX 1A` / `PSU MAX 30W` at (135.19, 101.33), right of the antenna notch (CON-1, CON-7);
`USB:24V OFF` at (132.62, 127.37), under CN3 (DEC-24). Karl's final wording; the first draft
also carried "wire with power off", "current lim" and "amb 0-35°C". The load is printed as the
1 A rating (CON-1), not the 1.5 A thermal estimate (§7).
**Impact.** Silkscreen only. DRC unchanged: 0 unconnected, 0 parity, only the 27 existing
`lib_footprint_issues` warnings; no silk overlap.
**Verify.** Fab gate: texts legible in the Gerber viewer.

## Deferred / Not doing

## Open questions
