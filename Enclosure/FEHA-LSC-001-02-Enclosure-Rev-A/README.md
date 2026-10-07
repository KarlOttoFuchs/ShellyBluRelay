# FEHA-LSC-001-02 Rev A — Enclosure

Inline slide-in tube with two identical end caps (one STL, printed twice) for the LED Strip
Controller PCB `FEHA-LSC-001-01`. Each cap has a screw tab centred under its cable exit and clicks
on with snap detents; once mounted, the two tab screws fix the caps and trap the tube. Overall
103.4 (incl. tabs) × 37.8 × 15.8 mm; tube 50.4 × 34.2 × 12.2 mm.

**Status:** Modelled to the board as routed (50 × 30 mm, DEC-35) with the wall groove (DEC-29),
2026-10-06; fit check passes. Not yet printed: a short tube test section comes first.
Constraints it puts on the PCB are the design spec's CON-2 and CON-3
([`../../Docs/design/led-strip-controller-design.md`](../../Docs/design/led-strip-controller-design.md)).

## Board retention (DEC-29)

The PCB slides into a **full-length groove in each side wall**; the wall is 2.8 mm thick to carry
it. Groove 1.9 mm tall (1.6 mm board + 0.3 mm), 1.0 mm deep (0.7 mm over the board edge + 0.3 mm
side clearance), 1.8 mm of wall behind it. The cap stop ribs block the board corners at each
end. **Print the tube standing on end** (no overhangs, no supports; slot height set by XY
accuracy), with a brim; the tube STL is exported standing up. Print a short test section first:
FDM slots come out 0.1–0.2 mm tight. Rationale and rejected options: design spec DEC-29.

## The FreeCAD model

`FEHA-LSC-001-02-Enclosure-Rev-A.FCStd` is fully parametric:

| Object | What it is |
|---|---|
| `Params` | Spreadsheet: every dimension by alias. Derived values (tube size, groove, overall size) are formulas |
| `Tube` | PartDesign Body: shell, bore, board groove, four detent grooves, pinhole over S1, LED window over D3 |
| `Cap` | PartDesign Body: the IN cap, the one printed part. Sleeve, cable chamber, stop ribs, snap ridges, cable exit, screw tab with gussets and countersink |
| `Cap_OUT` | `App::Link` to `Cap`, turned 180° about Z, so the OUT cap is the same part by construction |
| `PCB` | The populated controller board, exported from KiCad as STEP (DNP parts left out), placed in the groove by expression |

Every feature dimension and position is an expression on `Params`: edit a cell, recompute, and
the tube, both caps and the board position all update. Coordinates match the board (DEC-33):
origin at the tube centre in X and Y, Z up from the tube's bottom face, IN end at −X, antenna at
+Y, component side up (+Z faces the room when mounted).

`build_enclosure.py` is the source of truth. It reads the board outline and the S1/D3 positions
from the `.kicad_pcb`, re-exports the board STEP with `kicad-cli`, rebuilds the document from
scratch, and runs the fit check. The check sweeps each board part's Y/Z envelope along the full
tube length, so the board must slide in without touching, and it must clear both caps. Values
settled in the GUI belong in the script's `PARAMS` table, because a re-run overwrites the
document. Re-run it after any board change:

```zsh
cd Enclosure/FEHA-LSC-001-02-Enclosure-Rev-A
/Applications/FreeCAD.app/Contents/Resources/bin/freecadcmd -c "exec(open('build_enclosure.py').read())"
```

Run it from FreeCAD instead (Macro › Macros… › User macros location = this folder ›
`build_enclosure.py` › Execute) to also save colours, see-through tube and caps, and a fitted
view into the `.FCStd`.

## Files

- `build_enclosure.py` — the generator (above).
- `FEHA-LSC-001-02-Enclosure-Rev-A.FCStd` — generated parametric model.
- `FEHA-LSC-001-02-Enclosure-Rev-A_Tube.stl` / `_Cap.stl` — print files (tube standing on end;
  cap as modelled, print it twice).
- `FEHA-LSC-001-02-Enclosure-Rev-A_Tube.step` / `_Cap.step` — the printed parts as STEP.
- `fit_page.py` + `fit_page_template.html` — generate `enclosure-fit.html`, true-scale sections
  sliced from the model with the board in place (run after `build_enclosure.py`, same
  `freecadcmd -c "exec(open('fit_page.py').read())"` form). Dimension labels come from
  `Params`; the template's prose quotes numbers too, so re-read it after a parameter change.
  The claude.ai artifact "LSC Tube Fit" is published from the same page. The template is
  input only: it holds the page text and `%%MID%%`-style markers where the drawings go, so
  opened on its own it shows the markers and no drawings.
- `enclosure-fit.html` — generated fit page; this is the one to open in a browser.
