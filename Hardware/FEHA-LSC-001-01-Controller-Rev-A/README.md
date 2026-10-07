# FEHA-LSC-001-01 Rev A — LED Strip Controller

24 V COB LED-strip controller, Shelly BLU / BLE triggered, ESP32-C3

**Status:** Manufacturing files generated 2026-10-07 — schematic and layout gates closed
2026-10-06 (50 × 30 mm, 4 layers, routed). Next: fab gate review (fiducials, position file). See [`DESIGN-CHANGES.md`](DESIGN-CHANGES.md) for the
change record and the design spec in
[`../../Docs/design/led-strip-controller-design.md`](../../Docs/design/led-strip-controller-design.md)
for intent, architecture and mechanical constraints — this README does not restate them.

## Project layout

- `FEHA-LSC-001-01-Controller-Rev-A.kicad_pro` / `.kicad_sch` / `.kicad_pcb`
- `Library/_FEHA-LSC-001.kicad_sym` + `Library/_FEHA-LSC-001.pretty/` — the **project-local** symbol and
  footprint libraries, wired in via `sym-lib-table` / `fp-lib-table` using `${KIPRJMOD}`. The
  nickname comes from the product part number, not the directory name.
- `Library/3DModels/` — STEP models, referenced as `${KIPRJMOD}/Library/3DModels/<name>.step`.
- `Library/_FEHA-LSC-001.kicad_blocks/` — the project **design-block** library (`design-block-lib-table`)
  for reusable schematic/layout fragments; empty until a block is saved into it from KiCad.
- `scripts/lcsc_to_kicad.py` — imports LCSC/EasyEDA parts into the project library.
- `scripts/setup_board.py` — board setup: outline and antenna notch, centre origin, keep-out
  rule areas, GND pours (re-runnable; see its docstring).
- `production/` — fab outputs from the KiCad Fabrication Toolkit (2026-10-07):
  `LED_Strip_Controller_A.zip` (Gerbers and drill files), `bom.csv`, `positions.csv`,
  `designators.csv`, `netlist.ipc`. The toolkit names the zip from the board title and
  revision, not the `FEHA-LSC-001-01-Controller-Rev-A-<fab>-<YYYYMMDD>.zip` convention.

## Tooling

`kicad-cli` is not on PATH:
`/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli`

Files are **KiCad 10 format** (`version 20260306`).

## Revision history

- Rev A — initial design: schematic and layout complete, manufacturing files generated
  2026-10-07, not yet ordered.
