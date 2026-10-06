# LED Strip Controller — Design Spec

Board: **LED Strip Controller** — `FEHA-LSC-001-01` Rev A (Controller); enclosure `FEHA-LSC-001-02` Rev A

Scope of this document: the product's design intent and every settled decision. §0 is the
session pickup point; §11 is the decision log; Appendix A records which datasheets back which
values. The review skill checks the design against this document — an envelope that is not
written here cannot be reviewed against (DOC-4).

## 0. Roadmap — session pickup point

Last updated: 2026-10-06

- **Settled:** load and output stage (DEC-01…DEC-06), power (DEC-07, DEC-08), input protection
  (DEC-09…DEC-12), connectors, USB, UI and enclosure (DEC-13…DEC-16), module (DEC-27), status
  LED (DEC-28). Cost pass 2026-10-02: adjustable FPWM buck, no input fuse (supply requirement
  CON-7 instead), HL2310A reverse MOSFET, MINI-1-H4X module; 5 extended lines.
- **Open questions:** the supply's overcurrent behaviour (not published; bench test in §10).
- **Schematic gate: CLOSED 2026-10-06**
  ([`reviews/schematic-2026-10-02-controller.md`](reviews/schematic-2026-10-02-controller.md),
  checklist v1.3, full re-run on a fresh netlist). Changes from the review: CHG-01 (buck input
  100 nF), CHG-02 (debug header in standard USB order), yellow status LED (DEC-28). §7 carries
  the connector loss and the in-tube current ceiling (1.5 A practical). From here the decision
  log is superseded, not edited (§11 baseline rule).
- **Blocked on:** —
- **Layout decisions 2026-10-06:** wall groove (DEC-29); board 50 × 25 mm, 4 layers, stack-up
  L1 routing + GND / L2 GND / L3 GND / L4 routing + GND (DEC-30); antenna notch (DEC-31); debug
  header above CN2, adapter lying flat over the +Y edge (DEC-32); vias, netclasses, origin
  (DEC-33). Placement proven at courtyard level by
  `Hardware/FEHA-LSC-001-01-Controller-Rev-A/scripts/fit_placement.py` (all 40 parts, no overlaps).
- **Layout steps 1–2 done 2026-10-06:** placement applied, then Karl placed the buck as one
  block; it did not fit, so the board went to **50 × 30 mm (DEC-35)**: buck block under the
  module, +Y-referenced parts up 5 mm. Karl then refined the placement by hand (incl. D3/R7);
  the board file is the placement of record (`fit_placement.py` holds the superseded
  50 × 25 table). Board set up by `scripts/setup_board.py` (width argument; re-runnable, keeps
  hand-drawn pours): 4 layers, outline with 0.5 mm notch fillets, origin at the centre, CON-3
  part keep-outs on F.Cu, antenna keep-out on L1–L4, GND pours L1–L4. Board rules DEC-34;
  netclasses and pre-defined sizes DEC-36–38
  (PWR_3V3 and GND 0.5 mm; tracks 0.25/0.3/0.5/1.0).
  Buck pours on F.Cu (priority 1, solid): `SW F.Cu` hugging U1.6/C7.2/L1.2 (TI §11.1.1: SW node short, just wide
  enough); `+24V buck in F.Cu` on U1 VIN pins and C3/C8/C9 pin 1, kept off the IC interior so
  GND reaches U1 pin 2. Routing done by Karl (DRC 0 unconnected).
  Polarity silkscreen at both connectors: F.SilkS `+`/`-` 1.4 mm bold beside each pin at the
  board edge (1.4 is the largest that clears D2's silk) plus `IN` (CN1) / `OUT` (CN2) 1.2 mm;
  B.SilkS `+`/`-` 2.5 mm behind the pins plus `IN`/`OUT` 2.5 mm. CN1 has + on the top pin
  (y = 113), CN2 has + on the bottom pin (y = 117).
- **Next:** layout gate, in this order:
  3. Karl routes; check against §7 rules 1–9 and register A-4 (USB pair as a pair, clear of
     the antenna). CN1: tie each pin's two pads together via 2 × 0.45/0.3 vias per pad in the
     gap under the housing (not in the pads), +24V on L4; VIN_N is its own net to Q1.
     Open DRC items: U2 silk clipped by the antenna notch (3 warnings).
  4. Enclosure: update `enclosure_rev_b.py` to the groove (DEC-29), 30 mm board width (DEC-35) and the
     new button/LED/connector positions from the final placement; regenerate the STLs. Then
     rebuild the enclosure-fit page (claude.ai artifact "LSC Tube Fit",
     https://claude.ai/artifact/8Cz9GvjaSQ5Euj6Yx1rkBk) and `Enclosure/.../enclosure-concept.html`
     to show the final state only: groove section, final placement, PCB rules. No drafts (the
     page still shows the superseded rails/ribs and earlier proposals).

## 1. What this board is

A 24 V low-side PWM dimmer for under-cabinet COB LED strips and cupboard lights, switched by
Shelly BLU sensors over BLE, in an inline printed tube between the power supply and the strip.

| | |
|---|---|
| MCU | ESP32-C3-MINI-1-H4X module, −40 to 105 °C (DEC-27; antenna over a notch in the board's +Y edge, module set 1.3 mm in from it, DEC-29, DEC-31) |
| Power source | Futurelight PS002A, 24 V DC 30 W (1.25 A) surge-protected LED supply, IP20, also feeding the strip (DEC-21); USB 5 V via the debug header for bring-up |
| Comms | BLE (Shelly BLU / BTHome triggers, GATT setup page); SoftAP fallback for setup only |
| Operating temperature | 0 to 35 °C ambient around the tube (DEC-18); the §7 thermal budget is worked at the 35 °C top of the range |
| Ingress / exposure | Indoor, inside kitchen cabinets and cupboards; closed PETG tube, not sealed |
| Mains / SELV class | SELV only (24 V DC); no mains on the board |
| ESD exposure | None designed for (DEC-19): push-in terminals are wired with the supply off, the USB header is bench-only, and the board lives inside a closed tube |
| EMC target | None formal at prototype; keep switching loops small near the BLE radio (§7 rule 7) |
| Enclosure | `FEHA-LSC-001-02`: slide-in tube, two identical snap-on end caps, 119.4 × 27.8 × 15.8 mm |
| Test strategy | Functional, JLCPCB PCBA, small batches (DEC-20; mirrors `review-profile.yml`) |

### 1.1 Hard constraints and requirements (with sources)

| ID | Constraint | Source |
|---|---|---|
| CON-1 | Load 24 V COB strip, 8 W/m: 1.5 m (0.5 A) normal use, rated to 3 m (1.0 A) continuous at 100 % on with no time limit | DEC-01 |
| CON-2 | Board outline 50 × 30 mm, 4 layers, with a 15.2 × 6.6 mm antenna notch in the +Y edge; no part taller than the 4.5 mm connectors | Enclosure model, DEC-16, DEC-30, DEC-31, DEC-35 |
| CON-3 | Top side clear of parts within 1.0 mm of both long edges, full board length (the board edges run in a wall groove that overlaps them by 0.7 mm; the board slides in, so the whole edge passes the groove lip). Bottom side: no parts | Enclosure model, DEC-16, DEC-29 |
| CON-4 | Tube inner wall < 65 °C at 3 m / 1 A after soak (PETG softens ~80 °C, connectors rated 85 °C) | DEC-01, §10 |
| CON-5 | Output stays off through power-up, ROM bootloader and flashing | DEC-03 |
| CON-6 | Whole board, including the strip, survives a reversed 24 V input | DEC-09 |
| CON-7 | Supply: 24 V DC, current-limited, rated ≤ 30 W (1.25 A), current limit below 2 A. The supply's limit is the board's only overcurrent protection | DEC-12, DEC-21 |

### 1.2 Deliberate simplifications (each with an exit)

- **No hot-plug protection** (DEC-11): the board is never connected to a live 24 V lead. Exit:
  if that stops holding, add the input damping network and rerun a hot-plug test (VIN peak < 36 V).
- **No overcurrent protection for the output MOSFET itself** (DEC-12): with the 30 W supply
  (DEC-21) a shorted strip is current-limited by the supply at a collapsed voltage, so the
  MOSFET carries ≈ 2 A at a few hundred mV, well inside its rating. This holds only for a
  current-limited supply of this size; a stiff high-current supply would destroy the MOSFET.
  Exit: a current-limited high-side switch. Verified by the short-circuit test (§10).
- **No input fuse** (DEC-12): with a supply meeting CON-7, a 2 A fuse could never open, because
  the supply limits every fault current below its rating. Exit: if CON-7 stops holding (a
  stiffer supply), restore a fuse in the +24 V input ahead of the TVS (Littelfuse
  0466002.NRHF, C3105) together with the high-side switch above.

## 2. Power architecture

```
24 V IN ─ +24 V ─┬─ TVS ─ (board GND)
                 ├─ LED+ (OUT connector)
                 ├─ 1 µF local bypass at the output stage
                 └─ TPS560430XF FPWM buck (51 k / 22 k divider) ─ +3V3 ─ ESP32-C3 module, status LED
USB 5 V (debug header) ─ B5819W SL ─ buck VIN
24 V return ─ reverse-polarity MOSFET ─ board GND
```

- **Buck** (DEC-07): TI TPS560430XF, adjustable, forced PWM at 1.1 MHz (no PFM bursts, so no
  audible singing; datasheet §5 Device Comparison Table), 600 mA rating against ≈ 350 mA peak
  load (ESP32-C3 BLE/Wi-Fi peaks). Output set by RFBT 51 kΩ / RFBB 22 kΩ, 1 % 0402:
  V_OUT = 1.0 V × (1 + 51/22) = 3.32 V; V_REF ±1.5 % plus 1 % resistors keeps it within
  ≈ 3.22–3.42 V, inside the module's 3.0–3.6 V. Both resistors close to FB (datasheet layout
  rule 2).
- **Inductor** (DEC-08): 10 µH molded, Isat 2.2 A against the IC's 1.4 A maximum peak limit; TI's
  table suggests 12 µH. At 24 V in, ripple ≈ 0.26 A, peak ≈ 0.48 A, under the 0.8 A minimum
  current limit. Verify on the scope at bring-up.
- **USB 5 V** (DEC-14): via a Schottky into buck VIN, for flashing without the 24 V supply. USB
  ground is board GND, which bypasses the reverse-polarity MOSFET, so USB and the 24 V supply are
  never connected at the same time (DEC-24, §10).
- **Supply** (DEC-21): Futurelight PS002A, 24 V DC, 30 W = 1.25 A rated, surge protected, IP20,
  135 × 35 × 23 mm. Load at 3 m ≈ 1.0 A strip + ≈ 15 mA board = 82 % of rating; at 1.5 m ≈ 42 %.
  The retail page publishes no overcurrent behaviour (hiccup or constant-current, and at what
  level); measure it (§10). Its limit (typically 1.1–1.5 × rating, ≈ 1.4–1.9 A) is the board's
  only overcurrent protection (CON-7, DEC-12): it ends a shorted strip or a TVS failed short.

## 3. Input and protection

- **Reverse polarity** (DEC-09, DEC-02): HL2310A (60 V) in the negative line. Drain to the input −
  terminal, source to board GND, gate pulled to +24 V through 100 kΩ, BZT52C10 zener gate → source
  (cathode at gate). Normal: gate a little below the zener's 9.5–10.5 V rating, because the
  bias current is only ≈ 0.14 mA against its 5 mA test current; never above 10.5 V, inside the
  ±20 V gate limit. R_DS(on) ≤ 105 mΩ at 10 V, ≤ 125 mΩ guaranteed at 4.5 V. Reversed: body diode blocks 24 V
  of 60 V; the zener conducts forward and holds the gate ≈ −0.7 V, so the MOSFET stays off and
  its gate never sees −24 V. Board GND ≠ input − terminal.
- **TVS** (DEC-10): SMBJ26A across +24 V and board GND, after the reverse MOSFET. Standoff 26 V,
  breakdown 28.9–31.9 V, below the output MOSFET's 40 V and the buck's 38 V abs max. Its 42.1 V clamp
  figure applies only at the full 14 A pulse rating.
- **No fuse** (DEC-12): overcurrent protection is the supply's current limit (CON-7). The
  +24 V input runs straight from the IN connector to the TVS.
- **No bulk electrolytic** (DEC-11): it set the enclosure height.

## 4. Sensor / analogue front end

Not applicable: no sensors on the board. Triggers arrive over BLE.

## 5. ADC / measurement

Not applicable. The thermal fault cut-off uses the ESP32-C3 internal temperature sensor (§8).

## 6. MCU and interfaces

| Signal | Pin | Notes |
|---|---|---|
| PWM to output MOSFET gate | GPIO7 (MTDO) | No internal pull at reset, 5 ns low glitch only (DEC-03) |
| Button | GPIO9 | Boot/recovery strap, pin-hole in the enclosure (DEC-15) |
| Status LED | GPIO10 | 5 ns low glitch at reset (harmless) |
| USB D− / D+ | GPIO18 / GPIO19 | Native USB-Serial-JTAG on the 1×4 debug header (DEC-14), standard USB order 5V, D−, D+, GND to mate with a stock USB-to-header pigtail |

Avoid for the PWM output: GPIO6 (pull-up at reset), GPIO18/19 (USB; GPIO18 has a 50 µs high
glitch at power-up), GPIO20/21 (UART, pulled up), GPIO2/8/9 (strapping pins).

### 6.1 Output stage

- **Switch** (DEC-02): Vishay SI2356DS low side, gate driven directly from GPIO7. ≤ 70 mΩ
  guaranteed at a 2.5 V gate, V_GS(th) 0.6–1.5 V, 40 V, V_GS ±12 V.
- **Gate network** (DEC-04): 100 Ω series (≈ 55–60 ns drain edges), 10 kΩ pull-down to source
  (holds the gate low even against an internal 45 kΩ pull-up), GPIO at maximum drive strength.
- **Freewheel** (DEC-05): SS34 Schottky across the strip, cathode to LED+ (DC-only output). With
  the MOSFET on it blocks the full rail: 24 V is 60 % of its 40 V rating, and with hot-plug waived
  (DEC-11) and a surge-protected supply the rail stays below the TVS's 28.9 V breakdown (DEC-22).
- **Local bypass** (DEC-05): 1 µF 50 V X7R 0805 from +24 V to GND at the output stage, supplying
  the turn-on current edge locally.
- **PWM** (DEC-06): 19.5 kHz, 12-bit LEDC; full brightness is a static high, never 99.x %.

## 7. PCB layout constraints

Thermal basis: steady state is reached in 20–30 min, so long retriggering equals permanently on;
design for 100 % on with no time limit. Estimates, closed tube in 35 °C ambient, tube shedding
≈ 20 °C/W:

| | 1.5 m / 0.5 A | 3 m / 1.0 A |
|---|---|---|
| Output SI2356DS, full on (typ / worst) | 0.015 / 0.02 W | 0.06 / 0.085 W |
| Output SI2356DS, ~90 % PWM, 100 Ω gate (worst) | ≈ 0.035 W | ≈ 0.11 W |
| Reverse-protection HL2310A (≤ 105 mΩ at 10 V gate; × 1.2 warm) | ≤ 0.03 W | ≤ 0.13 W |
| IN/OUT connector contacts (4 × ≤ 20 mΩ) + copper | ≤ 0.03 W | ≤ 0.11 W |
| ESP32-C3 + buck | ≈ 0.35 W | ≈ 0.35 W |
| Total in the tube (worst) | ≈ 0.45 W | ≈ 0.72 W |
| Air inside the tube | ≈ 44 °C | ≈ 49 °C |
| Output MOSFET junction (Tj max 150 °C) | ≈ 50 °C | ≈ 65 °C worst |

Worst case = datasheet maximum on-resistance at a 2.5 V gate (70 mΩ) × 1.2 for a warm junction;
switching loss from ≈ 55–60 ns per edge (100 Ω gate + ≈ 17 Ω GPIO driver, typical Qgd 0.81 nC
at a ≈ 1.6 V plateau, datasheet p.3), ≈ 80 ns with a hot junction. SI2356DS: RthJA 175 °C/W max
steady state on 1" × 1" FR4, junction-to-foot (drain) 75 °C/W max. SOT-23 has no exposed pad,
so the drain copper is the heatsink. The real limit is the PETG tube and the 85 °C connectors.
Connector contact resistance is the HDGC4001 datasheet maximum (20 mΩ per contact, four contacts
in the load path); copper allows ≈ 10–30 mΩ of 1 oz path.

Copper pour sets junction temperature, not tube temperature: all the heat leaves through the
tube whatever the copper, so the wall follows total watts. At 3 m worst case the junctions sit at
≈ 80 °C on minimal pads, ≈ 71 °C on the datasheet's 1″² copper and ≈ 64 °C with the rule 1/3
pours stitched to the GND plane (≈ 120 °C/W, estimate).

**Current ceiling (this BOM, closed tube, 35 °C ambient):** solved with R_DS(on) tracking
junction temperature (≈ +0.65 %/°C, both datasheets) and the tube air rising with load. The
HL2310A reverse MOSFET limits first. With the rule 1/3 pours, worst-case parts reach 125 °C
junction at ≈ 1.7 A and 150 °C (abs max) at ≈ 1.85 A, with the tube air at ≈ 65–71 °C, so the
tube wall and the MOSFETs run out together. On minimal copper: ≈ 1.45 A / 1.6 A. Beyond
≈ 2.2 A there is no stable operating point (thermal runaway). **Practical maximum: 1.5 A
continuous** (4.5 m at 8 W/m; junctions ≈ 100 °C, tube ≈ 60 °C). The datasheet I_D ratings
(3.2 A / 3 A at 25 °C on 1″² FR4) do not apply inside the tube. The 3 m / 1.0 A rating (CON-1)
has ≈ 1.6× headroom. Any load above 1.25 A also needs a supply outside CON-7, which reopens
DEC-12. The soak test's V_DS at full on (§10) calibrates these estimates.

No impedance-controlled nets: USB is full-speed over a few centimetres and the module carries its
own antenna (L-SI-2).

1. **Output MOSFET drain (pin 3, LED− net):** solid copper pour on the top layer around pin 3,
   about 1 cm² (ample at 1.5 m; grow towards 2–3 cm² only if the 3 m soak test asks for it,
   and keep it within the tight loop of rule 7), mirrored on the bottom layer, stitched with 6–8 vias
   (0.3 mm drill) placed just outside the pad — no via-in-pad (solder wicks away). Connect
   pads solidly, no thermal reliefs. This net is the PWM switch node (24 V swing at 19.5 kHz):
   keep the pour compact rather than sprawling, and keep it away from the module antenna area,
   the buck feedback node and the button/LED lines.
2. **Output MOSFET source (pin 2):** straight into the GND plane through 2–3 vias at the pad.
3. **Reverse-protection MOSFET:** drain is the input − net — run it as a wide pour from the IN
   connector to pin 3 (carries the full return current and spreads heat); source into the GND
   plane. Zener and 100 kΩ next to the gate.
4. **Ground planes (DEC-30):** L2 and L3 are solid GND under everything except the antenna
   notch; L1 and L4 carry routing plus GND pours, stitched to the planes. +24 V reaches the buck
   and CN2 (LED+) on L4, with both planes between it and the module. The planes are the main
   heat spreader. "Bottom layer" in rules 1–3 means L4.
5. **Spread the heat sources:** keep the two MOSFETs, the buck and the ESP32 module apart rather
   than clustered; keep each heat source ≥ 3 mm from the long board edges, where the PETG
   rails and hold-down ribs touch the board.
6. **Current paths for 1 A:** +24 V, input −/GND return and LED−/LED+ as pours or ≥ 1.0 mm
   traces on 1 oz copper. 2 oz copper is optional, not needed.
7. **Switching loop:** at every PWM edge up to 1 A moves between two paths in ~60 ns. With
   the MOSFET on, current runs +24 V → LED+ → strip → LED− → MOSFET → GND. At turn-off the
   strip/cable inductance keeps it flowing round LED− → SS34 → LED+ → strip. Any loop
   carrying that changing current radiates (proportional to its area: unwelcome next to the
   BLE radio) and its track inductance adds a voltage spike on the drain (V = L·di/dt). Keep
   the on-board part of both loops as small as possible: OUT connector pins, SS34, output
   MOSFET and the 1 µF local bypass placed together in a ~10 × 10 mm corner by the OUT
   connector:

   ```
           OUT connector
          ┌──────────────┐
          │ LED+    LED− │
          └──┬────────┬──┘
             │  SS34  │        SS34 directly across the two pins
             ├──|◄────┤        (cathode to LED+)
             │        │
      1µF ═══╪═╗    ┌─┴─┐
             │ ║    │ Q │      output MOSFET: drain on LED−, a few mm away
      +24V ──┘ ║    └─┬─┘
               ╚══════╧══ GND plane (MOSFET source and cap: vias straight down)
   ```

   The strip and its cable are outside the board's control; this keeps the board's own
   contribution small.
8. **Input:** TVS close to the IN connector, after the reverse-protection MOSFET.
9. **Test access:** a bare copper test pad on the output drain pour for a thermocouple, and
   test points on drain and GND for measuring V_DS at full on during the soak test.

## 8. Firmware

ESP-IDF v5.5 (DEC-25), in `Firmware/`. The `ESPHome/` configuration belongs to the relay module
`FEHA-RM-001` and is not used for this product. Hardware/firmware contract:

- PWM on GPIO7, 19.5 kHz, 12-bit LEDC, `GPIO_DRIVE_CAP_3`; full brightness = static high.
- Dimming curve (DEC-26). The MOSFET cannot follow the shortest pulses: one LEDC step is 12.5 ns
  against ≈ 20–30 ns to reach the gate threshold and ≈ 55 ns per drain edge, so codes 1–2 give
  no light and codes ≈ 3–10 do not track the duty (estimates; the cut-over varies with the
  MOSFET threshold and temperature). The same happens mirror-image at the top, where the last
  few codes already look fully on. Firmware therefore:
  - maps brightness to duty through a perceptual lookup table (CIE 1931 lightness or gamma
    ≈ 2.2), starting at `min_code`, the first code that gives clean light (measured, §10);
    brightness 0 is duty 0, exactly;
  - runs every fade, on → off and off → on, through that one table, stepping the duty from an
    `esp_timer` at ≈ 200 Hz with `ledc_set_duty()` + `ledc_update_duty()` (the change takes
    effect at the next PWM period, so there are no glitches). The built-in LEDC hardware fade is
    linear in duty and would spend most of a fade in the bright half;
  - goes from the top of the table to full-scale duty (static high) at the end of a fade up,
    and starts a fade down from there, so full brightness has no switching loss (DEC-06).
- Thermal fault cut-off (DEC-23): read the internal temperature sensor; above a threshold well
  beyond normal operation (set from the soak test, roughly 85 °C die), switch the strip off and
  blink the status LED until the temperature falls. It is a fault response, not cooling: about
  60 % of the heat in the tube at 3 m (more at 1.5 m) is the ESP32 and buck, and the reverse
  MOSFET's share does not fall with duty, so dimming the strip barely changes it.
- Configuration: BLE GATT setup page (LE Secure Connections, 6-digit passkey on the label);
  button long-press opens a SoftAP serving the same page for 10 min; 10 s hold = factory reset.

## 9. Bill of materials and sourcing

JLCPCB PCBA. Extended (fee) lines: ESP32-C3 module, buck, inductor, connector, output MOSFET = 5.
Every other line is basic or preferred (no fee); parts still to be chosen (buck input/output
capacitors) are picked from basic/preferred parts.

| Function | Part | LCSC |
|---|---|---|
| MCU module | Espressif ESP32-C3-MINI-1-H4X | C41349510 |
| Output MOSFET | Vishay SI2356DS-T1-GE3 | C74127 |
| Gate series / pull-down | 100 Ω / 10 kΩ 0402 | C25076 / C25744 |
| Reverse MOSFET | hongjiacheng HL2310A | C7420347 |
| Reverse-MOSFET gate pull-up / zener | 100 kΩ 0402 / BZT52C10 | C25741 / C19077408 |
| Freewheel Schottky | SS34 | C8678 |
| Output-stage bypass | Samsung CL21B105KBFNNNE 1 µF 50 V X7R 0805 | C28323 |
| Buck / inductor | TI TPS560430XFDBVR / cjiang FXL0420-100-M | C523980 / C177242 |
| Buck feedback RFBT / RFBB | 51 kΩ / 22 kΩ 1 % 0402 | C25794 / C25768 |
| TVS | SMBJ26A | C19077580 |
| USB Schottky | B5819W SL | C8598 |
| Status LED / series resistor | Hubei KENTO KT-0805Y, yellow ≈ 590 nm, 0805 / 470 Ω 0402 (≈ 2–3 mA) (DEC-28) | C2296 / C25117 |
| Button | XUNPU TS-1088-AR02016, 4 × 3 mm, 2 mm tall | C720477 |
| IN and OUT connectors (×2) | HDGC4001SMD-S-2P push-in, 18–24 AWG | C5197184 |

## 10. Verification and bring-up plan

- **Test-point plan:** +24V, +3V3 (3.22–3.42 V), module EN, output drain (V_DS at full on) and
  two GND test points, all in the schematic; a bare copper pad on the output drain pour for a
  thermocouple (§7 rule 9).
- **USB rule** (DEC-24): flash and debug over the USB header with the 24 V supply disconnected;
  the board runs from USB alone. Disconnect USB before connecting 24 V.
- **Functional test** (each board, by hand on the bench): flash over the USB header (24 V
  disconnected); remove USB, power from 24 V and check +3V3; strip on, full brightness (static high) and a fade; button press and
  long-press (SoftAP opens); status LED; a trigger from a paired Shelly BLU device switches
  the strip; reversed 24 V input leaves the board unpowered and undamaged (one board per batch).
- **Soak test (3 m / 1 A only; also sets the DEC-23 cut-off threshold from the measured die temperature):** closed printed tube, 60 min at full brightness, then 60 min at
  90 % PWM; thermocouples on the output MOSFET drain copper and the tube's inner wall; measure
  V_DS at full on. Pass: tube wall < 65 °C (CON-4). Fail: move the output switch to AOS AON7264E
  (DFN 3×3, exposed pad, new footprint). 1.5 m needs no soak test.
- **Buck:** check ripple and inductor current on the scope at 24 V in (DEC-08).
- **Dimming low end (DEC-26):** in a dark room, step the duty up from 0 with a scope on the
  output MOSFET drain; record the first code that gives a clean full-height pulse and visible
  light, and set `min_code` a few codes above it. Then fade on → off and off → on over ≈ 1 s
  and ≈ 5 s, by eye and on the scope. Pass: no visible step at the bottom, no snap to off at
  the end of a fade down, no flash at the start of a fade up, and no visible jump into full
  brightness at the top.
- **Short circuit (one board, with the PS002A):** short LED+ to LED− with the output on, for
  10 s. Record the supply's behaviour (hiccup or constant current, and the current). Pass: the
  supply limits below 2 A (CON-7); the board keeps running from the buck or recovers when the
  short is removed; the output MOSFET survives.
- **Bring-up order** (each step passes before the next):
  1. Unpowered: no short +24V–GND or +3V3–GND.
  2. USB pigtail only (24 V disconnected, DEC-24): +3V3 in range, EN rises ≈ 10 ms after +3V3,
     board enumerates as USB-Serial-JTAG; flash. Start SoftAP (Wi-Fi TX) on USB power and
     confirm +3V3 stays above 3.0 V; if the buck fails to start or resets, use a powered hub
     (buck VIN ≈ 4.1–4.3 V after the Schottky against a 4.0 V maximum start threshold).
  3. USB removed; 24 V from a bench supply limited to ≈ 100 mA, no strip: input current
     settles at ≈ 15–30 mA, +24V and +3V3 in range; buck ripple and switch node on the scope.
  4. Strip connected, 24 V from the PS002A: functional test above (reversed input on one board
     per batch).
  5. 3 m board only: soak test, dimming low end, short circuit.

## 11. Decision log

| ID | Date | Decision | Rationale |
|---|---|---|---|
| DEC-01 | 2026-10-01 | Product is a 24 V COB LED-strip controller, not a relay module; new product number `FEHA-LSC-001` | Change of role; relay board `FEHA-RM-001` Rev A stays as built |
| DEC-02 | 2026-10-02 | Output MOSFET: Vishay SI2356DS. Reverse MOSFET: hongjiacheng HL2310A | Output: guaranteed ≤ 70 mΩ at a 2.5 V gate. Rejected for the output: HL2310A (specified only at ≥ 4.5 V), AO3422 (≤ 200 mΩ at 2.5 V), AO3400A (30 V: ≈ 85 % of rating at 25.2 V rail plus SS34 drop, unclamped below the TVS's 28.9 V), photo-MOSFET SSRs (too slow for PWM); JLCPCB has no no-fee 40 V logic-level part. Reverse: its gate sits at ≈ 10 V from the zener, so logic-level drive is not needed; HL2310A is specified at 10 V (≤ 105 mΩ), 60 V, ±20 V gate, preferred (no fee) at ≈ $0.04 against $0.35. Costs ≈ +0.07 W at 1 A (≈ +1.5 °C tube air). Using SI2356DS for both (one part number) was the earlier choice, changed in the 2026-10-02 cost pass |
| DEC-03 | 2026-10-02 | PWM on GPIO7 | No pull at reset; continuity with Rev A. GPIO0/1/3/4/5 equally safe |
| DEC-04 | 2026-10-02 | 100 Ω gate series, 10 kΩ pull-down | Ample drive margin at a 1.6 V plateau; slower edges near the BLE radio for ≈ +15 mW. 33 Ω and 100 kΩ rejected |
| DEC-05 | 2026-10-01 | SS34 freewheel across the strip; 1 µF local bypass at the output stage | DC-only output; turn-on edge supplied locally |
| DEC-06 | 2026-10-01 | 19.5 kHz, 12-bit PWM; full brightness = static high | Inaudible, smooth fades; no switching loss in the commonest state |
| DEC-07 | 2026-10-02 | TPS560430XF forced-PWM buck, adjustable, 51 kΩ / 22 kΩ divider for 3.32 V | No PFM singing. XF and X3F are both 1.1 MHz FPWM (datasheet §5); XF is $0.62 against $1.39 for the fixed X3F, for two basic 0402 resistors. TI Table 1's 51 k / 22.1 k replaced by 22 k (basic part). Rejected: X3F (price), AP63201 FPWM (no stock), AP63203 (Rev A; PFM), the no-fee bucks (TPS5430: non-synchronous, SOIC-8-EP, 4.4 mA quiescent; TPS54331: 28 V max; XL1509/LM2596: 150 kHz) |
| DEC-08 | 2026-10-01 | 10 µH FXL0420-100-M inductor | Isat margin over the 1.4 A peak limit; verify on scope |
| DEC-09 | 2026-10-01 | Reverse-polarity N-MOSFET in the negative line | ≤ 0.06 W at 1 A against ≈ 0.45 W for the series SS34 it replaced |
| DEC-10 | 2026-10-01 | SMBJ26A TVS after the reverse MOSFET | Clamps supply overshoot below the buck's 38 V abs max |
| DEC-11 | 2026-10-02 | Hot-plug waived: no damping network, no hot-plug test, no bulk electrolytic | Board never connected to a live lead; electrolytic set the enclosure height |
| DEC-12 | 2026-10-02 | No input fuse; overcurrent protection is the supply's current limit, stated as requirement CON-7 | The 30 W PS002A limits at ≈ 1.4–1.9 A, below the 2 A fuse's rating (466 series carries 100 % of rating for ≥ 4 h), so with this supply a shorted strip or a TVS failed short is ended by the supply and the fuse could never open. Saves an extended line ($3 per order) and $0.07 per board. A 0 Ω placeholder was not adopted (jumper current rating unverified). Earlier choice: Littelfuse 0466002.NRHF 2 A in the +24 V input, removed in the 2026-10-02 cost pass; it returns with any supply that does not meet CON-7 |
| DEC-13 | 2026-10-01 | Push-in 2-pin connectors in and out (HDGC4001SMD-S-2P) | Tool-free field wiring, 18–24 AWG |
| DEC-14 | 2026-10-01 | USB-C and USBLC6 removed; 1×4 press-fit debug header, 5 V via B5819W SL | Bench-only access; same Schottky as Rev A D6 |
| DEC-15 | 2026-10-01 | GPIO9 button and one status LED; strip used as feedback. Red error and green power LEDs removed | Enclosure is closed; fewer parts |
| DEC-16 | 2026-10-01 | Inline slide-in tube enclosure (`FEHA-LSC-001-02`) | Sets CON-2 and CON-3 |
| DEC-17 | 2026-10-01 | Not doing: mmWave footprint, joining home Wi-Fi | Out of scope for a BLE-triggered light |
| DEC-18 | 2026-10-02 | Operating range 0 to 35 °C ambient | Indoor cabinets and cupboards; matches the §7 thermal budget |
| DEC-21 | 2026-10-02 | Supply: Futurelight PS002A 24 V 30 W surge-protected LED supply | Karl's chosen supply; 1.25 A covers 3 m at 82 %; its current limit protects the output stage from a shorted strip |
| DEC-22 | 2026-10-02 | Keep the 40 V SS34 freewheel; no 60 V part | 24 V is 60 % of rating; rail never reaches the TVS breakdown with hot-plug waived; the 40 V output MOSFET has the same exposure, so a 60 V diode alone adds no margin |
| DEC-23 | 2026-10-02 | Replace the dim-to-70 % thermal fallback with a fault cut-off (strip off above ≈ 85 °C die) | Dimming saves ≈ 0.02 W of ≈ 0.6 W in the tube (≈ 0.4 °C); tube air worst case ≈ 47 °C vs the 65 °C wall limit; the die sensor tracks the ESP32, not the MOSFET or tube wall |
| DEC-24 | 2026-10-02 | USB header and 24 V supply never connected together (procedural, no circuit change) | USB GND bypasses the reverse MOSFET: with an earthed supply output, a reversed lead and an earthed host, host VBUS is shorted through the B5819W. The board runs from USB alone for flashing |
| DEC-25 | 2026-10-02 | Firmware framework: ESP-IDF v5.5 | Karl's choice; ESPHome is not used for this product |
| DEC-26 | 2026-10-02 | Fades through a perceptual lookup table with a measured `min_code`, stepped in software, same table both directions | The MOSFET cannot resolve the lowest 12.5 ns codes; LEDC hardware fade is linear in duty |
| DEC-28 | 2026-10-06 | Status LED is yellow (KT-0805Y, C2296), off in normal operation; it signals setup and fault states by blink pattern | The LED faces down into the room through the enclosure window: white carries no meaning and reads as strip light leakage; yellow reads as "attention" for both setup and fault. Its ≈ 2 V forward voltage lets the 470 Ω resistor set the current (≈ 2–3 mA), where the white part's 2.6–3.2 V left brightness to the LED bin. Rejected: addressable RGB (WS2812B-2020-V6 C52917434, XL-1615RGBC C5349954; JLCPCB has none without a setup fee), red/green pair (second GPIO, LED and window), red alone (alarming for setup states) |
| DEC-19 | 2026-10-02 | No ESD protection on the terminals or USB header | Terminals wired unpowered; USB bench-only; board enclosed |
| DEC-20 | 2026-10-02 | Test strategy: functional, JLCPCB PCBA, small batches | Low volume; no fixture or ATE |
| DEC-27 | 2026-10-02 | Module: ESP32-C3-MINI-1-H4X (C41349510) | Same module as GeyserSense (FEHA-GTS-001). Chip revision v1.1, −40 to 105 °C, cheaper than the MINI-1-N4 ($2.95 against $3.03), which Espressif lists as NRND (MINI-1 datasheet v2.2). Rejected: ESP8684-MINI-1 / ESP32-C2 (low stock, firmware port, tighter RAM for BLE + GATT + SoftAP), bare ESP32-C3 chip (crystal, flash, antenna matching and RF layout for no saving at small batches) |
| DEC-29 | 2026-10-06 | Board retention in the tube: a full-length groove in each side wall, formed by thickening the wall to 2.8 mm. Groove 1.9 mm tall (1.6 mm board + 0.3 mm), 1.0 mm deep (0.7 mm over the board edge + 0.3 mm side clearance), 1.8 mm of wall behind it. It replaces the support rails and the 10 mm hold-down ribs; the cap stop ribs stay. Tube printed standing on end. Sets CON-3 | The tube is one piece and the board slides in, so its whole edge passes the entry-end retention: the old CON-3 (clear only the last 10 mm) let the edge-flush module collide with a hold-down rib on insertion. Standing on end every layer has the same outline: no overhangs, no supports, and the slot height comes from XY accuracy. The outside width is unchanged (24.2 mm); the 1 mm per side lost inside is the PCB edge keep-out anyway. Holds the board along its full length instead of at the ends. Rejected: rails + 10 mm ribs as modelled (insertion collision), rails + full-length 1 mm rib (same slot, but a thin rib that warps and prints rough), groove cut into the 1.8 mm wall (≈ 0.8 mm left behind it), printing flat (the slot roof becomes a bridge that sags into a 0.3 mm clearance). Print a short test section first: FDM slots come out 0.1–0.2 mm tight. Consequence for the PCB: ESP32 module set 1.3 mm in from the +Y edge (clears the wall face above the groove by 0.3 mm) |
| DEC-30 | 2026-10-06 | Board 50 × 25 mm, 4 layers (JLCPCB standard 1.6 mm). Stack-up: L1 routing + GND pour, L2 solid GND, L3 solid GND, L4 routing + GND pour. Supersedes the 66 × 20 mm placeholder in CON-2 | CN2's LED+ pin is +24 V, so 24 V must run the full board length. On a 20 mm board the module covers all but 0.85 mm of the width: on 2 layers 24 V could only pass under the module on the bottom layer, cutting the GND plane. With two inner GND planes, 24 V runs on L4 shielded from the module. Courtyard fit test of all 40 parts: 50 × 24 fails, 50 × 25 fits (64 % part area). JLCPCB prices 1–8 layer boards up to 50 × 50 mm at its lowest tier. Tube becomes 29.2 mm wide and ≈ 16 mm shorter (≈ +2 °C tube air at 3 m, estimate). Rejected: 58 × 20 mm 2-layer (fits, but cuts the plane under the module), 60 × 20 mm 4-layer (outside the 50 × 50 price tier), 50 × 20 mm (parts do not fit: 14 overlaps), 24 V on an inner layer (Karl: both inners solid GND) | superseded-by: DEC-35 (board size; stack-up stands)
| DEC-31 | 2026-10-06 | Antenna: board notch 15.2 × 6.6 mm in the +Y edge, centred on the module; keep-out on all 4 layers | Espressif: if the antenna cannot sit outside the board, cut the board away below and on both sides of it. Footprint antenna area is 5.4 mm deep; with the 1.3 mm inset (DEC-29) a 6.6 mm notch reaches the antenna boundary and stays 0.4 mm clear of the module's top pad row; 15.2 mm = module + 1 mm each side. Espressif also asks ≈ 15 mm clear of metal around the antenna in the housing: PETG tube, do not mount against metal |
| DEC-32 | 2026-10-06 | Debug header CN3 at the OUT end, +Y edge, above CN2; pin 1 (V) towards the OUT end. The bench USB-C adapter (10 × 13 mm, pin row 2 mm from its edge) goes on straight pins and lies flat, its socket overhanging the +Y edge; VBUS diode D4 next to the header | Bench-only (DEC-14), board out of the tube. Flat on the edge puts only ≈ 5 mm of the adapter over the board and keeps the space beside the module for the module's own parts. D−/D+ run ≈ 15 mm, fine for full speed (A-4). Rejected: header on the module's USB side with the adapter upright on a right-angle header (Karl prefers the edge overhang), header on the IN side of the module (USB pair round the module) |
| DEC-33 | 2026-10-06 | Vias 0.45/0.3 mm everywhere (two side by side where a 1 A net changes layer). Netclasses: Default (0.25 mm track) and PWR_1A (1.0 mm: `/+24V`, `/VIN_N`, `/LED_N`). Board origin at the board centre | Via size proven on GeyserSense (FEHA-GTS-001). The tube and both caps are symmetric about their mid-plane, so centre coordinates map straight onto the enclosure and survive length changes | superseded-by: DEC-36 (netclasses; vias and origin stand)
| DEC-34 | 2026-10-06 | Board rules: copper-to-edge clearance 0.3 mm; minimum via 0.4 mm, minimum annular ring 0.075 mm. Stack-up nominal 1.6 mm (KiCad default 4-layer), built to JLCPCB's standard 4-layer stack-up | The 0.5 mm default failed on settled geometry: the U2 GND pad row sits 0.4 mm from the notch (DEC-31), CN1/CN2 pads 0.3 mm from the short edges. JLCPCB's routed-edge minimum is 0.2 mm. The via minimums are GeyserSense's (FEHA-GTS-001), where DEC-33's 0.45/0.3 via was proven; the 0.5 / 0.1 defaults reject it. No impedance-controlled nets (§7), so the dielectric split does not matter. Rejected: per-item DRC exclusions, a custom rule for U2/CN1/CN2 only |
| DEC-35 | 2026-10-06 | Board 50 × 30 mm (was 50 × 25, DEC-30); stack-up unchanged. The buck sits as one block under the module, parts referenced to the +Y edge (module, notch, D2/TP1, CN3/D4, module-side passives) move up 5 mm, CN1/CN2 stay centred | Karl's buck placement (TI layout: input caps, U1, L1, output cap and divider in one tight block) is 14.2 × 9.3 mm; the strip under the module was 5.8 mm tall. The fit needs ≥ 28.7 mm; Karl chose 30. Stays inside JLCPCB's 50 × 50 mm price tier. Tube ≈ 34.2 mm wide (was 29.2). Rejected: lengthening (a 14 mm buck column gives ≈ 64 mm, outside the price tier, as for DEC-30), squeezing the buck flat to fit 5.8 mm (loses the tight layout) |
| DEC-36 | 2026-10-06 | Netclasses: Default 0.25 mm, PWR_1A 1.0 mm (`/+24V`, `/VIN_N`, `/LED_N`), new PWR_3V3 0.6 mm (`+3V3`); vias 0.45/0.3 in all. Pre-defined sizes: tracks 0.25 / 0.4 / 0.6 / 1.0 mm, via 0.45/0.3 | The buck output feeds the ESP32's radio current peaks (a few hundred mA); 0.25 mm Default is thin for that. 0.6 mm on 1 oz carries it with margin and still fits beside the module. Pre-defined sizes let W / Shift+W step widths; the netclass width stays the default. GND stays in Default (planes carry it) | superseded-by: DEC-37 (widths), DEC-38 (GND class); vias stand
| DEC-37 | 2026-10-06 | Narrower widths: PWR_3V3 0.5 mm (was 0.6); pre-defined tracks 0.25 / 0.3 / 0.5 / 1.0 mm (0.4 → 0.3, 0.6 → 0.5). Default and PWR_1A unchanged. The two +3V3 segments already routed at 0.6 set to 0.5 | Karl: 0.6 and 0.4 were just a bit too wide. 0.5 mm on 1 oz still carries the few-hundred-mA radio peaks with margin. DRC unchanged (no new violations) |
| DEC-38 | 2026-10-06 | New netclass GND 0.5 mm (`GND`), vias 0.45/0.3, priority 2 | Karl: 0.5 mm is a good size for the short stubs from cap pads to GND vias. Replaces DEC-36's "GND stays in Default"; planes still carry GND. DRC unchanged |

**Baseline rule:** until the first schematic gate closes, this table is a *baseline* — rows are
edited in place, not superseded. After that gate, a changed decision gets
`superseded-by: DEC-mm` appended to its row — never a strikethrough, never deletion.

## Appendix A — datasheet provenance

Vendor PDFs in `../datasheets/` are ground truth.

| Part | File | Note |
|---|---|---|
| SI2356DS | `Vishay-SI2356DS.pdf` | Doc 62893 Rev. A; gate charge p.3 |
| HL2310A | `hongjiacheng-HL2310A.pdf` | Rev 2.1; R_DS(on) ≤ 105 mΩ at 10 V, V_GS ±20 V |
| TPS560430 | `TI-TPS560430.pdf` | SLVSE22B; §5 variants (XF = FPWM adjustable), V_REF 1.0 V (§8.3.2), Table 1 L/C/divider values |
| ESP32-C3 | `Espressif-ESP32-C3.pdf` | v2.4; pin reset states Tables 2-1/2-2 |
| ESP32-C3-MINI-1 | `Espressif-ESP32-C3-MINI-1.pdf` | v2.2; H4X variant, 105 °C, chip v1.1 (Rev A used MINI-1-H4) |
| BZT52C10 | `hongjiacheng-BZT52C10.pdf` | |
| SMBJ26A | `hongjiacheng-SMBJ26A.pdf` | |
| SS34 | `MDD-SS34.pdf` | |
| B5819W SL | `CJ-B5819W-SL.pdf` | 40 V |
| 0466002.NRHF | `Littelfuse-0466.pdf` | Not fitted (DEC-12); the part for a supply outside CON-7 |
| CL21B105KBFNNNE, CL31A106KBHNNNE, CL31A226KAHNNNE | `Samsung-CL21B105KBFNNNE.pdf` | Samsung MLCC general catalogue (2014): series ratings and part-number key; no part-specific DC-bias curves |
| CL10A106MA8NRNC | `Samsung-CL10A106MA8NRNC.pdf` | Part specification sheet (2024) |
| KT-0805Y | `KENTO-KT-0805Y.pdf` | Approval spec A.0; V_F 1.8–2.4 V at 20 mA, I_F 25 mA max |
| FXL0420-100-M | `cjiang-FXL0420-100-M.pdf` | Series catalogue |
| HDGC4001SMD-S-2P | `HDGC-HDGC4001SMD-S-2P.pdf` | |
