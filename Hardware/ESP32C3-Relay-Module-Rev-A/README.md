# FEHA-RM-001 Rev A — ESP32-C3 Relay Module

BLE-triggered relay module: ESP32-C3-MINI-1-H4, SRD-03VDC-SL-C relay (3 V coil, AO3400A driver),
AP63203 3.3 V buck, USB-C or 5–24 V DC input, push button and status LEDs.

**Status:** Fabricated (JLCPCB). The KiCad files are frozen and must match the boards in hand.

## Project layout

- `ESP32C3-Relay-Module-Rev-A.kicad_pro` / `.kicad_sch` / `.kicad_pcb` — the design. The
  directory and file stem predate the `<PN>-<Slug>-Rev-<X>` convention and are kept as built.
- `ESP32C3-Pin-Mapping.md` — GPIO and connector map for this board.
- `production/` — the fabrication outputs that were ordered (`ESP32C3_Relay_Module_A.zip`, BOM,
  placement, IPC netlist).

## Libraries

This revision has **no project-local library**. Symbols and footprints resolve from machine-global
libraries: the custom library (`${CUSTOM_LIBRARY}`), the JLCPCB plugin library and KiCad's
3rd-party folder (`${KICAD8_3RD_PARTY}`). On a machine without them the schematic and board open
with missing parts. The LED Strip Controller (`FEHA-LSC-001-01`) carries its own library instead.

## Revision history

- Rev A — first build. Superseded in role, not in revision: its planned successor became a new
  product, `FEHA-LSC-001` (see `../../Docs/design/part-numbering.md`).
