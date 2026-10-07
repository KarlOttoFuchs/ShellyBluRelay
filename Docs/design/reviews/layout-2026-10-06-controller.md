# FEHA-LSC-001-01 Controller Rev A — layout gate review — 2026-10-06

Run: full · Variants: — · Gate: layout · Checklist: v1.3
Source: `FEHA-LSC-001-01-Controller-Rev-A.kicad_pcb` as saved 2026-10-06 22:28 (working tree on
top of 9c9f3f1; no KiCad GUI running). `kicad-cli pcb drc --schematic-parity --refill-zones
--severity-all` on a scratch copy, plus the same without refill on the saved fill; geometry from
KiCad 10 `pcbnew` Python (pads, tracks, vias, zones). ESP32-C3-MINI-1 datasheet v2.2 p.38
(Fig. 11-1 land pattern).
Scope: whole board after routing. Changes since 9c9f3f1: teardrops on all pads (72 teardrop
zones), D− segments merged, third GND via at Q2 source (132.65, 114.75). Schematic unchanged
since the schematic gate closed (parity 0), so schematic-gate items are not re-run.
Coverage (initial pass): 25 items + 10 project rows · 0 Blocker · 3 Major · 3 Minor · 2 Advisory ·
6 N-A · 2 needs-info · 19 pass. Round 1 adds L-7 (Minor)
Carried deferrals (not re-raised): hot-plug (DEC-11), no-esd-protection (DEC-19),
no-input-fuse (DEC-12), ss34-40v-freewheel (DEC-22), PART-5-status-led-current. Register A-1
(R_0402 SMD attribute) is settled by Karl and is not re-raised; the `footprint_type_mismatch`
ignore belongs to it.

## Method note

- DRC: 0 unconnected, 0 parity, 30 warnings. 27 are `lib_footprint_issues` ("configuration does
  not include the footprint library 'PCM_JLCPCB'"): the PCM libraries resolve only in the GUI,
  so kicad-cli cannot compare those footprints with the library. Not a board defect. The other 3
  are U2 silk clipped by the antenna notch (A-L1).
- Saved fill = refilled fill (same DRC result with and without `--refill-zones`).
- Courtyard overlap evidence is incomplete: five parts have no courtyard and the
  `missing_courtyard` check is set to ignore (L-3). A clean `courtyards_overlap` result does not
  cover those five parts.

## Findings (most severe first)

| ID | Sev | Status | Ref | Note (source) | Disposition |
|----|-----|--------|-----|---------------|-------------|
| L-1 | Major | finding | VIN_N vias (111.67, 118.81), (103.30, 118.84) | VIN_N carries the full 1 A return from CN1.2 to Q1 drain and changes layer F→B→F through **one** 0.45/0.3 via at each end. DEC-33 asks for two vias per 1 A layer change; DEC-39 applied it to +24V only. Fix: second 0.45/0.3 via beside each, as on +24V. (pcbnew) | **closed 2026-10-06** — Karl 2026-10-06: fix. Second 0.45/0.3 via at (110.97, 118.81) and (104.00, 118.81), each joined by a 1.0 mm F.Cu stub (DEC-40) |
| L-2 | Major | finding | U2 EPAD | The 8 GND EPAD pads (123.0–127.0 × 110.3–114.3) have **no vias** and reach the F.Cu GND pour through thermal spokes (zone connection thermal). Espressif Fig. 11-1 (p.38) shows vias in the thermal-pad gaps; the module is the largest heat source on the board (≈ 0.35 W, §7). Fix: 0.45/0.3 vias in the gaps between the EPAD pads (tented), and solid zone connection on the U2 EPAD pads. (pcbnew, PDF p.38) | **withdrawn 2026-10-06** — Karl 2026-10-06: disagrees; module EPAD without vias works on GeyserSense and other boards. Profile deferral `module-epad-no-vias` |
| L-3 | Major | finding | Q2, U1, CN1, CN2, L1 | These five project-library footprints have **no courtyard**, and DRC `missing_courtyard` is set to ignore, so overlaps with them are not checked (two problems, L-DFA-1). Fix: add F.Courtyard to the five `_FEHA-LSC-001` footprints, update the board from the library, set the logo footprint "Exempt from courtyard requirement", then set `missing_courtyard` back to error. (pcbnew, .kicad_pro) | **closed 2026-10-06** — Karl 2026-10-06: fix. F.CrtYd rectangles (body/pads + 0.25 mm) added in `_FEHA-LSC-001.pretty` and copied to the five board instances; logo set `allow_missing_courtyard`; `missing_courtyard` = error. The re-enabled check exposes L-7 |
| L-4 | Minor | finding | TP4 | TP4 is **+24V**, but its silk label at (113.37, 103.19) reads `3V3`. TP5 (+3V3) is also labelled `3V3`. Fix: change the text to `24V`. (pcbnew) | **closed 2026-10-06** — Karl 2026-10-06: TP4 label changed to `24V` (board saved 22:43) |
| L-5 | Minor | finding | §7 rule 9 | No bare-copper thermocouple pad on the drain pour: the LED_N pour has no mask opening. TP6 (LED_N) sits 7 mm from Q2 on the 1.0 mm D1–CN2.2 track. V_DS can be measured at TP6 against the GND plated hole CN3.4 (no load current flows there). Fix: a ≈ 3 × 3 mm F.Mask opening on the `LED_N drain F.Cu` pour, or accept the thermocouple on the Q2 body and say so in §10. (pcbnew) | **closed 2026-10-06** — Karl 2026-10-06: thermocouple taped on; no bare pad. §7 rule 9 and §10 updated (DEC-40) |
| L-6 | Minor | finding | spec §7 rules 3/4 | The board deviates from two §7 rules, and no DEC row records either deviation. +24V runs on F.Cu down the left side to the buck and from the via pair to CN2/D1/C10 (rule 4 says L4; only §0 mentions it). VIN_N is a 1.0 mm track, not a pour (rule 3 says a wide pour; this is within rule 6). The B.Cu +24V run under the module does follow rule 4. Fix: a DEC row superseding those parts of rules 3/4, or reroute. (pcbnew, spec) | **closed 2026-10-06** — Karl 2026-10-06: fix the document. §7 rules 3 and 4 state the board as built (DEC-40) |
| L-7 | Minor | finding | Q2/CN2, U1/L1, C5/L1, CN1/L1 | Found by the courtyard check that L-3 re-enabled: 4 `courtyards_overlap` errors, all courtyard-margin only. Overlaps 0.29 × 3.45 / 0.66 × 0.97 / 0.19 × 3.33 / 2.23 × 0.22 mm. Nearest pads Q2.3–CN2.2 0.26 mm (both LED_N), U1.6–L1.2 0.92 mm (both SW), C5.1–L1.1 0.36 mm (both +3V3), CN1.2–L1.2 2.42 mm. No pad or body collision (body outlines from the footprint silk). Decide: accept as DRC exclusions, or move parts. (DRC, pcbnew) | **accepted 2026-10-06** — Karl 2026-10-06: accept; the four violations are DRC exclusions in the board file (courtyard margin only; moving parts would loosen the buck loop and the §7 output corner). Hand-rework note: hot air on L1 also reflows U1/C5, on Q2 part of CN2. Profile deferral `courtyard-margin-overlaps` |
| A-L1 | Advisory | finding | U2 silk | The antenna notch clips U2's silk (3 DRC warnings, already open in §0). The fab clips silk at the edge anyway. Accept, or trim the silk line in the board copy. (DRC) | **accepted 2026-10-06** — Karl 2026-10-06: the 3 `silk_edge_clearance` warnings excluded in DRC (fab clips silk at the edge) |
| A-L2 | Advisory | finding | §7 rule 7 | The commutation loop Q2 drain → D1 → +24V → C10 → GND vias → Q2 source occupies ≈ 13 × 12 mm (target ≈ 10 × 10). D1 sits above the LED− pin (anode 4 mm from CN2.2), and its cathode returns down x = 141.45 under the CN2 housing to the + pin (8 mm). C10 is 3 mm from CN2.1, and Q2 drain is 2.5 mm from CN2.2. The loop closes over solid L2 GND. Acceptable; note only. (pcbnew) | **accepted 2026-10-06** — Karl 2026-10-06: accept as built. Estimated overshoot ≈ 0.2 V (≈ 10 nH × 17 A/µs) against 40 V V_DS; edge energy ends ≈ 5 MHz, far below BLE; the strip cable loop dominates. Revisit only on a faster gate drive, > 1.5 A, or a 1–30 MHz EMC peak (then 100 nF at D1 cathode to GND) |

## Project must-check rules

| Rule | Status | Evidence |
|------|--------|----------|
| §7-1 drain pour | pass (DEC-39) | `LED_N drain` pours 5.5 × 6 mm on F.Cu and B.Cu, ≈ 0.33 cm² filled per side (≈ 0.65 cm² total, DEC-39), solid connection, 6 stitching vias all outside pads (closest edge gap: D1.2, 0.4 mm) |
| §7-2 Q2 source | pass | 3 GND vias at 0.78 / 0.82 / 1.69 mm from Q2.2 |
| §7-3 Q1 | finding → L-1, L-6 | Q1 source: 2 GND vias at ≤ 1.06 mm. D5 2.1 mm and R3 1 mm from the gate. VIN_N is a 1.0 mm track with single vias |
| §7-4 planes | finding → L-6 | In1/In2 GND filled 1321 mm² each (solid apart from via holes and the notch). +24V on L4 under the module, but on F.Cu elsewhere |
| §7-5 heat spread | pass | Q1 (x 103.5), buck (110–119), module (118.5–131.5), Q2 (133.8) are spread along the board. Q1/U1/Q2 are ≥ 5 mm from the long edges. Module at the +Y edge by DEC-31; rails/ribs superseded by the groove (DEC-29) |
| §7-6 1 A paths | pass | +24V, VIN_N, LED_N 1.0 mm or pour. The only thin +24V copper is the 0.3 mm R3 bias feed (µA) |
| §7-7 switching loop | pass, see A-L2 | CN2, D1, Q2, C10 in one corner |
| §7-8 TVS | pass | D2 4.5 mm from CN1.1, on +24V after Q1 |
| §7-9 test access | finding → L-5 | TP6 on LED_N; GND at CN3.4 / TP7; no thermocouple pad |
| A-4 USB pair | pass | D+ 14.7 / D− 14.5 mm (Δ 0.2 mm), 0.2 mm, F.Cu only, no vias, over L2 GND, routed together from U2.26/27 to CN3, in the −Y/OUT quadrant away from the notch. Bench pigtail ≤ 15 cm stays a bench note |

## Coverage table

| ID | Status | Note |
|----|--------|------|
| L-DRC-1 | pass | Patterns resolve: PWR_1A = /+24V, /VIN_N, /LED_N; PWR_3V3 = +3V3; GND = GND; rest Default. No empty class. No `.kicad_dru`; nothing needs one (no isolation domains) |
| L-DRC-2 | finding | Ignored: `missing_courtyard` (→ L-3), `footprint_type_mismatch` (register A-1, settled), `footprint_filters_mismatch`, `track_not_centered_on_via`, tuning-profile checks (harmless). Silk/courtyard-overlap checks remain active (warning/error) |
| L-DRC-3 | pass | 0 unconnected, 0 parity, 0 errors. Config gaps stated in Method note |
| L-DRC-4 | pass | Saved fill = refilled fill |
| L-PWR-1 | N-A | Single SELV domain (24 V); no isolation barrier |
| L-PWR-2 | finding | → L-1. Widths OK (1.0 mm on 1 A nets) |
| L-PWR-3 | pass | +24V layer changes in via pairs (DEC-39); VIN_N → L-1 |
| L-PWR-4 | pass | Copper-edge 0.3 mm (DEC-34) enforced by DRC; no mounting hardware |
| L-PWR-5 | pass | C4/C6 at module +3V3; C9 HF at U1 VIN; R5/R6 at U1 FB; C10 at CN2 |
| L-PLACE-1 | N-A | No isolation or analog split |
| L-PLACE-2 | pass | §7 corner realised (A-L2); antenna over the notch (DEC-31) |
| L-PLACE-3 | needs-info | Wire entry/insertion against the groove tube: enclosure step 4 |
| L-PLACE-4 | N-A | No mounting holes (groove retention, DEC-29) |
| L-PLACE-5 | needs-info | CON-3 keep-outs enforced; the enclosure fit waits for step 4 (`enclosure_rev_b.py` update) |
| L-SI-1 | pass | No controlled-Z nets (§7); USB pair per A-4 |
| L-SI-2 | N-A | §7: full-speed USB, module antenna |
| L-RF-1 | pass | Antenna keep-out rule area on L1–L4 over the notch; DRC clean; module 1.3 mm in |
| L-RF-2 | pass | DEC-31; silk clip A-L1 |
| L-EMC-1 | pass | SW pour hugs U1.6/C7/L1; output loop A-L2 |
| L-EMC-2 | N-A | No cable-entry filtering in the schematic (DEC-19) |
| L-THRM-1 | finding | → L-2 (module EPAD). MOSFET pours per rule 1/DEC-39 |
| L-DFA-1 | finding | → L-3 |
| L-DFA-2 | finding | → L-4; A-L1 |
| L-DFA-3 | pass | TP1/3/4/5/6/7 per §10. Fiducials: checked at the fab gate |
| L-DFA-4 | N-A | `functional` test strategy, no jig |

## Not assessable at this gate

L-PLACE-3 and L-PLACE-5 (enclosure step 4). Fiducials and position-file content (fab gate).

## Previously resolved this session (do not re-raise)

Round 1 (Karl 2026-10-06): L-1, L-3 fixed; L-4 fixed by Karl; L-5, L-6 closed by document
change (DEC-40); L-2 withdrawn.
Round 2 (Karl 2026-10-06): L-7 accepted as DRC exclusions; A-L1 accepted (silk warnings
excluded). Round 3 (Karl 2026-10-06): A-L2 accepted. Verified: the saved `.kicad_pro` holds the 7 exclusions; fresh DRC 0 unconnected, 0 parity,
0 unexcluded errors, 27 `lib_footprint_issues` warnings (CLI-only, Method note). After the fixes: DRC 0 unconnected, 0 parity, 4
courtyards_overlap errors (L-7), 27 + 3 warnings as in the Method note.

## Gate status

**CLOSED (2026-10-06)** — Karl 2026-10-06: close. Full run, checklist v1.3, plus re-verification of
the fixed board (fresh DRC: 0 unconnected, 0 parity, 0 unexcluded errors). No open Blocker,
Major or Minor; all findings dispositioned (L-1, L-3, L-4 fixed; L-5, L-6 by DEC-40; L-2
withdrawn; L-7, A-L1, A-L2 accepted). Carried to the fab gate: L-PLACE-3 and L-PLACE-5
(enclosure step 4), fiducials and position-file content. Profile `current_gate: fab`.

## Addendum 2026-10-07 — carried enclosure items

L-PLACE-3 and L-PLACE-5 closed on the model: `Enclosure/FEHA-LSC-001-02-Enclosure-Rev-A/build_enclosure.py` builds the groove tube around the board's KiCad STEP, sweeps every board part along the full tube length and checks both caps: no contact. The push-in wire entries (y ±2.0, 6.7 mm above the tube floor) are centred on the Ø6.5 cable exit. Physical confirmation is the tube test print (design spec §0 step 4).
