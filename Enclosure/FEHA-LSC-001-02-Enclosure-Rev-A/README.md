# FEHA-LSC-001-02 Rev A — Enclosure

Inline slide-in tube with two identical end caps (one STL, printed twice) for the LED Strip
Controller PCB `FEHA-LSC-001-01`. Each cap has a screw tab centred under its cable exit and clicks
on with snap detents; once mounted, the two tab screws fix the caps and trap the tube. Overall
119.4 × 27.8 × 15.8 mm.

**Status:** Concept. Constraints it puts on the PCB are the design spec's CON-2 and CON-3
([`../../Docs/design/led-strip-controller-design.md`](../../Docs/design/led-strip-controller-design.md)).

## Board retention (DEC-29, 2026-10-06)

The PCB slides into a **full-length groove in each side wall**; the wall is thickened to 2.8 mm to
carry it. Groove 1.9 mm tall (1.6 mm board + 0.3 mm), 1.0 mm deep (0.7 mm over the board edge +
0.3 mm side clearance), 1.8 mm of wall behind it. This replaces the support rails and the 10 mm
hold-down ribs that `enclosure_rev_b.py` still models; the cap stop ribs stay. **Print the tube
standing on end** (no overhangs, no supports; slot height set by XY accuracy), with a brim. Print a
short test section first: FDM slots come out 0.1–0.2 mm tight. Rationale and rejected options:
design spec DEC-29.

**Pending:** update `enclosure_rev_b.py` (and regenerate the STLs) once the PCB outline is fixed.

## Files

- `enclosure_rev_b.py` — parametric FreeCAD model; run with
  `/Applications/FreeCAD.app/Contents/Resources/bin/freecadcmd enclosure_rev_b.py`. It writes the
  `.FCStd` and one STL per printed part. The `rev_b` file names date from when this was planned as
  relay-module Rev B; they are kept so the script and its outputs still match.
- `Enclosure_Rev_B.FCStd`, `Enclosure_Rev_B_Tube.stl`, `Enclosure_Rev_B_Cap.stl` — generated outputs.
- `enclosure-concept.html` — concept sketch and dimensions.
