# FEHA-LSC-001-01 Controller Rev A — schematic gate review — 2026-10-02
Run: full · Variants: none · Gate: schematic · Checklist: v1.3
Source: netlist `FEHA-LSC-001-01-Controller-Rev-A.kicad_sch` → kicad-cli 10.0.4 XML (2026-10-02 16:10, no live eeschema/pcbnew, no lock file). Working copy is uncommitted but **electrically identical to commit 6039da4** (netlist compare: same 40 components, values, footprints and fields; same nets — the diff is a file reformat). Vendor PDFs: TI TPS560430 SLVSE22B (pin functions p.3, abs max p.4, EC p.5–6, §8.3.4, §8.4.2, Table 1 p.15, §9.2.2.6–7 p.19, §11 p.20); Espressif ESP32-C3-MINI-1 v2.2 (Table 3-1 pin definitions, Tables 4-2/4-3 strapping, Fig. 9-1 peripheral schematic p.34); hongjiacheng HL2310A Rev 2.1 p.1 (pinout). JLCPCB part data via pcbparts `jlc_get_part` (stock, class, KT-0805W Vf, inductor DCR, connector rating).
Scope: whole board, single sheet — reverse polarity (Q1/D5/R3), TVS (D2), USB OR-ing (D4) and debug header (CN3), buck (U1/L1/C3/C4/C5/C7/C8/R5/R6), module core (U2/C2/C6/R8 and straps R1/R2/R4/C1/S1), status LED (D3/R7), output stage (Q2/R9/R10/D1/C10), connectors CN1/CN2, test points TP1/TP3–TP7.
Coverage: 52 items · 0 Blocker · 2 Major · 3 Minor · 6 Advisory (notes, not checklist items) · 5 N-A · 0 needs-info · 2 deferred · 40 pass
Carried deferrals (not re-raised): hot-plug (DEC-11). Also settled by the spec and not re-raised: no ESD (DEC-19, PROT-1), no fuse / supply current limit (DEC-12, PROT-3), 40 V SS34 (DEC-22), 10 µH inductor (DEC-08), USB and 24 V never together (DEC-24).
Supersedes: the netlist-free items of `schematic-2026-10-02-rev-b-concept.md`, which listed them as not assessable.

## Method note
- ERC from kicad-cli: **0 errors**, 52 warnings, all `lib_symbol_issues` / `footprint_link_issues` for `PCM_JLCPCB*` — the CLI does not load the PCM library configuration (known, see the session memory). Footprints were therefore resolved by globbing the PCM install (`References/KiCad/10.0/3rdparty/.../JLCPCB.pretty`) rather than trusting ERC.
- Diode polarity was read from the embedded PCM symbols: on SS34, SMBJ26A, KT-0805W, B5819W SL and BZT52C10 pin 1 is the cathode (bar side). Every diode in the netlist is oriented as the spec intends (D1 K → +24 V, D2 K → +24 V, D3 K → GND, D4 K → +24 V / A → VBUS, D5 K → G_REV).
- The BOM export (24 lines, grouped) was read: no junk lines, no missing line; TP1/TP3–TP7 and CN3 correctly absent.

## Findings (most severe first)
| ID | Sev | Status | Ref | Note (source) | Disposition |
|----|-----|--------|-----|---------------|-------------|
| PWR-3 | Major | finding | U1 VIN (pin 5) / GND (pin 2) | No high-frequency cap at the buck input. `/+24V` carries only C3, C8 (10 µF 50 V X5R **1206**) and C10 (1 µF 0805, the output-stage bypass, placed at Q2 per §7 rule 7). TI asks for ≥ 2.2 µF bulk **plus "a capacitor with a value of 0.1 µF for high-frequency filtering … as close as possible to the device pins"** (TPS560430 §9.2.2.6 p.19; §11.1.1: the input HF cap is "the key to EMI reduction"). A 1206 10 µF has several times the ESL of a 0402, and the switch-current loop of a 1.1 MHz buck is the board's sharpest edge next to the BLE antenna. Bulk is adequate: 2 × 10 µF at 24 V ≈ 2 × 3–4 µF effective after DC bias, above the 2.2 µF minimum. Proposed: add 100 nF 50 V X7R 0402 between VIN and GND at U1 — Samsung CL05B104KB54PNC, **C307331, basic** (verified 2026-10-02). The board's existing 100 nF (C1525) is 16 V and must not be reused here. | **closed 2026-10-06** — Round 1, Karl 2026-10-06: added C9 (re-annotated; all other refdes unchanged, nets otherwise identical). Re-check found C9 placed as C1525 (16 V at 24 V). Round 2, Karl 2026-10-06: C9 swapped to C307331 (CL05B104KB54PNC, 50 V X7R); C1 and C6 moved to the same part; C7 bootstrap left at C1525 (≤ 5.5 V). Round 3, Karl 2026-10-06: C7 also moved to C307331 (instance fields; one BOM line for all 100 nF). House fields `LCSC Part #`/`Mfr. Part #`/`Package` restored on C1/C6/C9 (cleared by Change Symbol). Verified on a fresh netlist: C9 on `/+24V`–GND with U1.5/U1.2, netlist otherwise unchanged, ERC 0 errors. Recorded as FEHA-LSC-001-01 CHG-01 |
| FUNC-7 | Major | finding | CN3 debug header ↔ bench USB cable | Header pinout is **1 VBUS · 2 D+ · 3 D− · 4 GND** (netlist: CN3.2 on `/USB_MCUD+` → U2.27 IO19/USB_D+, CN3.3 on `/USB_MCUD-` → U2.26 IO18/USB_D−; module pins verified against MINI-1 Table 3-1). This matches the archived Rev-B notes ("5V, D+, D−, GND") and is internally consistent (FUNC-6 passes), but it is **not the standard USB 4-wire order (VBUS, D−, D+, GND)** that ready-made USB-to-header pigtails and the USB-A pin sequence use. The mating cable is not defined anywhere in the spec, so the pair cannot be reconciled. If the bench cable is a standard pigtail, D+/D− cross: no damage, but USB-Serial-JTAG never enumerates and the board cannot be flashed — the Rev-C SWD lesson. Proposed: either swap pins 2/3 to the standard order (Karl redraws two labels), or keep the order and record the header and cable pin map in spec §6 with the cable built/labelled to match. | **closed 2026-10-06** — Karl 2026-10-06: bench pigtail checked, order 5V, D−, D+, GND; CN3 pins 2/3 swapped to match. Verified on a fresh netlist: CN3.2 on `/USB_MCUD-` → U2.26 IO18/USB_D−, CN3.3 on `/USB_MCUD+` → U2.27 IO19/USB_D+, CN3.1 VBUS, CN3.4 GND; no other net changed; ERC 0 errors. Pin order and cable recorded in spec §6. Recorded as FEHA-LSC-001-01 CHG-02 |
| PART-5 | Minor | finding | D3 / R7 status LED | White KT-0805W, Vf 2.6–3.2 V at 25 mA (JLCPCB data; no vendor PDF in the repo), driven from a 3.3 V GPIO through 470 Ω. Headroom is a few hundred mV, so current is ≈ 0.5–1.5 mA and brightness is set by the Vf bin, not by R7 — units from different reels will differ visibly. Spec §9 still lists the status-LED resistor as "still to be chosen", so 470 Ω is undocumented. Proposed: either accept and judge visibility through the pin-hole at bring-up (record R7 in §9), or drop R7 to ≈ 150–220 Ω (basic 0402) so current is ≈ 2–5 mA while staying inside GPIO drive limits. | **accepted 2026-10-06** — Karl 2026-10-06: keep 470 Ω. Evidence: the fabricated relay module `FEHA-RM-001` Rev A drives the same KT-0805W (C34499) from the same IO10 through 1 kΩ (R8, Rev A netlist) and it is clearly visible; 470 Ω gives about twice that current. GeyserSense `FEHA-GTS-001` uses the same 470 Ω. R7 recorded in spec §9. Round 2, Karl 2026-10-06: D3 changed to yellow KT-0805Y (C2296, Vf 1.8–2.4 V), which removes the finding's cause: ≈ 2–3 mA set by R7 (DEC-28). House fields restored on D3 after Change Symbol; netlist otherwise unchanged |
| TEST-3 | Minor | finding | spec §10 | The schematic carries TP1 GND, TP3 EN, TP4 +24V, TP5 +3V3, TP6 LED_N (drain, V_DS), TP7 GND, but §10 still reads "Further rail test points: TBD with the schematic" and "Bring-up order: TBD with the schematic". The thermocouple pad (§7 rule 9) is a layout item. Proposed: list the TPs in §10 and write the bring-up order (current-limited 24 V → TP4/TP5 → EN → flash over USB with 24 V removed, per DEC-24). | **closed 2026-10-06** — Karl 2026-10-06: USB first, then current-limited 24 V; reversed-input test stays per batch. §10 test-point plan and five-step bring-up order written |
| DOC-3 | Minor | finding | Docs/datasheets | Values acted on in this review without a vendor PDF in the repo: KT-0805W (Vf, PART-5), and the input/output MLCCs CL31A106KBHNNNE / CL31A226KAHNNNE / CL10A106MA8NRNC (DC-bias derating, PWR-3/PWR-5). Symbol Datasheet fields are populated for every fitted part. Proposed: add the four PDFs (fetch from the manufacturer, not LCSC's anti-bot page). | open |

### Advisory notes (not checklist items; for later gates or bring-up)
| ID | Ref | Note (source) | Disposition |
|----|-----|---------------|-------------|
| A-1 | R1–R10 footprint `PCM_JLCPCB:R_0402` | The PCM footprint has **no `(attr smd)`** (all other PCM footprints used here have it), so the ten resistors arrive on the board as type "unspecified". Position-file exporters that filter on SMD type will drop them. At the layout gate: set Component type = SMD on those footprints (or fix in the project copy) and confirm R1–R10 appear in the CPL. (footprint file) | open |
| A-2 | PCM 3D models | PCM footprints reference `${KICAD8_3RD_PARTY}/3dmodels/…`; that variable is not set in the KiCad 10 config on this machine. Project-library parts (connectors, module, inductor, U1, Q2) use `${KIPRJMOD}` and resolve. Check in the 3D viewer at layout; the tall parts are all project-local, so LIB-6 itself passes. | open |
| A-3 | USB-only supply headroom | From USB, VIN ≈ 4.75 V − ≈ 0.45 V (B5819W) ≈ 4.3 V. At 350 mA the buck needs ≈ 3.32 V + 0.35 A × (0.45 Ω HS typ + 0.28 Ω DCR) ≈ 3.6 V, ÷ D_MAX 0.89 ≈ 4.1 V before frequency foldback (TPS560430 §8.3.4, EC p.6), and UVLO rising is ≤ 4.0 V. Works on a good port; a weak port or long cable can brown out on Wi-Fi TX (SoftAP). Bring-up: run SoftAP once on USB power and watch +3V3. | open |
| A-4 | USB D+/D− | Espressif Fig. 9-1 shows 0 Ω series resistors and cap placeholders on USB_D+/D−; this board connects direct. Acceptable for full-speed over a few cm and a DNP bench header; note only. | open |
| A-5 | PWR-8 / spec §8 | Hardware brown-out behaviour is sound (R9 holds the gate low as +3V3 collapses; buck UVLO falling 3.25–3.65 V). Clean reset on a sagging rail relies on the ESP-IDF brown-out detector being enabled (default). State that in the §8 hardware/firmware contract so it is not turned off. | open |
| A-6 | review-profile.yml | Only `hot-plug` is recorded as a deferral. DEC-19 (no ESD → PROT-1), DEC-12 (no fuse → PROT-3) and DEC-22 (40 V SS34 → PART-1/PROT-5) are settled decisions the checklist would otherwise re-raise; add them as deferral entries referencing their DEC rows. | open |

## Coverage table (every enabled item)
| ID | Status | Note |
|----|--------|------|
| FUNC-1 | pass | U1 EN tied to VIN — permitted ("Can be tied to VIN", p.3; EN ≤ VIN + 0.3 V holds by construction). U1 CB–SW 100 nF (C7) per §9.2.2.7. U2 EN RC 10 kΩ/1 µF per Fig. 9-1. Unused module GPIOs and NC pins carry no-connect flags; no floating mandatory input. |
| FUNC-2 | pass | GPIO7 has no reset pull; R9 10 kΩ holds Q2 off (CON-5). Straps: IO2 R4 and IO8 R2 pulled up, IO9 R1 10 kΩ up → SPI boot (MINI-1 Table 4-3). IO9's C1 (RC 1 ms, ≈ 99 % at 4.6 ms) settles well before EN's 10 ms RC releases reset plus the 3 ms hold (Table 4-2); changing C1 or C2 changes this margin. |
| FUNC-3 | pass | No shared outputs; no open-drain nets. |
| FUNC-4 | N-A | Module contains the crystal. |
| FUNC-5 | pass | Nets named by label/power symbols; no stray or duplicated label merges (net list reviewed). |
| FUNC-6 | pass | U1 pins 1 CB/2 GND/3 FB/4 EN/5 VIN/6 SW match TI p.3 and the SOT-23-6 pad ring. U2 pins match MINI-1 Table 3-1 (3V3 = 3, EN = 8, IO7 = 21, IO9 = 23, IO10 = 16, IO18/D− = 26, IO19/D+ = 27). Q1 1 G/2 S/3 D = HL2310A p.1; Q2 1 G/2 S/3 D = SI2356DS. CN3 internally consistent (see FUNC-7). CN1 1 = +24V / 2 = VIN_N; CN2 1 = LED+ (+24V) / 2 = LED_N. |
| FUNC-7 | finding | Major — CN3 header vs bench cable, see Findings. CN1/CN2 mate with field wiring; polarity follows silkscreen (layout gate). |
| PWR-1 | pass | U1 600 mA vs ≈ 350 mA module peak (JLCPCB: 350 mA TX) + LED ≈ 1.5 mA. L1 Isat 2.2 A / Irated 1.6 A vs 1.4 A max peak limit. USB host draws ≈ 0.3 A at 4.3 V. |
| PWR-2 | pass | Single rail; no load fed through a signal pin. With USB only, Q1 gate sits at ≈ 4.5 V — harmless (CN1 unconnected). |
| PWR-3 | finding | Major — buck VIN HF cap missing. Module 3V3: C6 100 nF + C4 10 µF + C5 22 µF ≥ Fig. 9-1's 10 µF + 0.1 µF (placement at layout). |
| PWR-4 | pass | V_OUT = 1.0 V × (1 + 51/22) = 3.32 V (V_REF 0.985–1.015 V, p.6); internal soft-start/compensation; Table 1 pair 51 k/22.1 k → 22 k accepted (DEC-07). |
| PWR-5 | pass | All X5R/X7R. C3/C8 50 V at 24 V (48 %); C5 22 µF 25 V at 3.3 V ≈ 17 µF effective + C4 ≈ 5 µF ≈ Table 1's 22 µF. C7 16 V sees ≤ 5.5 V (CB–SW abs max). C10 50 V at 24 V (DEC-05). |
| PWR-6 | N-A | Mains-adapter powered, no sleep budget. |
| PWR-7 | pass | D4 (40 V, 1 A) ORs USB into +24V; reverse 24 V = 60 % of rating (DEC-14, DEC-24). See A-3 for headroom. |
| PWR-8 | pass | See A-5 (firmware contract note). |
| PROT-1 | deferred | DEC-19 — no ESD by decision (see A-6). |
| PROT-2 | pass | Q1 in the negative line, drain to CN1.2, source to GND, R3 100 kΩ gate pull-up, D5 cathode at gate; reversed input blocked by the body diode, gate clamped ≈ −0.7 V (spec §3). D2 after Q1. |
| PROT-3 | deferred | DEC-12 / CON-7 — supply current limit is the protection (see A-6). |
| PROT-4 | N-A | No battery. |
| PROT-5 | pass | SMBJ26A standoff 26 V > 24 V; clamp exceedance only under hot-plug (DEC-11 carried, DEC-22). |
| PART-1 | pass | Q1 60 V, Q2 40 V, D1/D4 40 V at 24 V (60 %, DEC-22); caps above. |
| PART-2 | pass | Q2 4.3 A vs 1 A; Q1 3 A vs 1 A (2 A short-circuit, DEC-12); CN1/CN2 9 A; D1 3 A; R3 ≈ 2 mW; L1 ≈ 35 mW at 350 mA. |
| PART-3 | pass | 0–35 °C (DEC-18): every part rated ≥ −25…+85 °C (connector −25…+85, button −30…+80, LED −30…+85, module −40…+105). |
| PART-4 | pass | All lines in stock 2026-10-02; module 1 317 pcs (thin, but enough for small batches). |
| PART-5 | finding | Minor — status LED current, see Findings. Feedback divider 1 % (spec §2). |
| IF-1 | pass | Single 3.3 V logic domain; Q2 gate driven at 3.3 V against 2.5 V R_DS(on) spec. |
| IF-2 | pass | All pull-ups to +3V3; no cross-domain pull-ups. |
| IF-3 | N-A | No I²C/bus pull-ups. |
| IF-4 | pass | R10 100 Ω on the PWM gate (DEC-04); USB see A-4. |
| IF-5 | N-A | Native USB, no flow-control lines. |
| THRM-1 | pass | Dissipators identified with a worked budget in §7 (Q2, Q1, U1, module). |
| EMC-1 | pass | Cabled I/O: no EMC target (spec §1); buck input HF filtering is PWR-3. |
| EMC-2 | pass | Module PCB antenna; keep-out and edge placement noted for layout (spec §1, §7 rule 4). |
| TEST-1 | pass | USB-Serial-JTAG on CN3 (pin map subject to FUNC-7); EN on TP3. |
| TEST-2 | pass | S1 on IO9 through the enclosure pin-hole (DEC-15); download mode = hold S1 + replug USB. |
| TEST-3 | finding | Minor — §10 not updated, see Findings. |
| TEST-4 | pass | D3 on IO10, firmware-controlled. |
| TEST-5 | pass | Functional strategy: strip, button (short/long), LED, BLE trigger, USB flash, reversed input each have a §10 step. |
| BOM-1 | pass | Field audit: 26 instances clean; CN3 has empty sourcing fields but is DNP + excluded from BOM (footprint-only bench header) — intended. Project-local parts carry all fields. |
| BOM-2 | pass | CN3 DNP. Single-source parts (module, TPS560430, SI2356DS) are the spec's deliberate choices; SI2356DS fallback AON7264E named in §10. |
| BOM-3 | pass | R/L/C `Value` equals the typed value property; qualifiers in their own fields. |
| LIB-1 | pass | 0 empty Footprint fields of 40 components, incl. all six TPs and DNP CN3. |
| LIB-2 | pass | All footprints resolve (project `.pretty`, KiCad TestPoint, PCM `JLCPCB.pretty`). |
| LIB-3 | pass | Packages match (SOT-23, SOT-23-6, SMA, SMB, SOD-123, 0402/0603/0805/1206, 4.4 × 4.2 inductor). |
| LIB-4 | pass | TPs exclude-from-BOM and exclude-from-pos; CN3 DNP + exclude-from-BOM. See A-1 for R_0402's missing SMD attribute. |
| LIB-5 | pass | Symbol pin set = pad set for every part; SOT-23 (Q2) pad 3 alone opposite 1/2; SOT-23-6 (U1) 1–3 / 6–4 ring. Diode pin 1 = cathode throughout. |
| LIB-6 | pass | STEP models present for connectors, module, inductor, U1, Q2 (`${KIPRJMOD}`). See A-2. |
| LIB-7 | pass | Project library: 5 symbols and 6 footprints, every one referenced; no orphans or shadow copies. |
| DOC-1 | pass | ERC 0 errors; 52 warnings are the CLI's PCM library-path noise (Method note). |
| DOC-2 | pass | No decisions made this session yet; dispositions below will update the spec. |
| DOC-3 | finding | Minor — four PDFs missing, see Findings. |
| DOC-4 | pass | Envelopes present (§1 temperature, exposure, ESD, test strategy; §2 load budget; §10 non-empty). Bring-up order gap is in TEST-3. |

## Not assessable at this gate
Placement of C6/C4 at module pin 3, the new VIN HF cap at U1, R5/R6 at FB, the §7 switching-loop corner, antenna keep-out, CN1/CN2 polarity silkscreen, A-1 and A-2 — layout gate.

## Gate status
OPEN — 0 open Major, 1 open Minor (DOC-3). No Blocker. (PWR-3, FUNC-7 and TEST-3 closed, PART-5 accepted, 2026-10-06.)
