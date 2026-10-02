# LED Strip Controller — Design Spec

Board: **LED Strip Controller** — `FEHA-LSC-001-01` Rev A (Controller); enclosure `FEHA-LSC-001-02` Rev A
Status: concept.

Scope of this document: the product's design intent and every settled decision. §0 is the
session pickup point; §11 is the decision log; Appendix A records which datasheets back which
values. The review skill checks the design against this document — an envelope that is not
written here cannot be reviewed against (DOC-4).

## 0. Roadmap — session pickup point

Last updated: 2026-10-02

- **Settled:** load and output stage (DEC-01…DEC-06), power (DEC-07, DEC-08), input protection
  (DEC-09…DEC-12), connectors, USB, UI and enclosure (DEC-13…DEC-16). Concept review register:
  [`reviews/schematic-2026-10-02-rev-b-concept.md`](reviews/schematic-2026-10-02-rev-b-concept.md)
  (no open Blocker or Major).
- **Open questions:** operating-temperature range, ESD exposure and test strategy (§1); the
  supply requirement (§2); SS34 voltage margin, thermal-fallback effectiveness and low-end PWM
  linearity (open register rows).
- **Blocked on:** —
- **Next:** answer the §1 TBD rows, then draw the schematic in
  `Hardware/FEHA-LSC-001-01-Controller-Rev-A/`.

## 1. What this board is

A 24 V low-side PWM dimmer for under-cabinet COB LED strips and cupboard lights, switched by
Shelly BLU sensors over BLE, in an inline printed tube between the power supply and the strip.

| | |
|---|---|
| MCU | ESP32-C3 module (antenna flush with the board's +Y edge) |
| Power source | External 24 V DC supply, also feeding the strip; USB 5 V via the debug header for bring-up |
| Comms | BLE (Shelly BLU / BTHome triggers, GATT setup page); SoftAP fallback for setup only |
| Operating temperature | TBD — Karl. Thermal budget (§7) assumes 35 °C ambient around the tube |
| Ingress / exposure | Indoor, inside kitchen cabinets and cupboards; closed PETG tube, not sealed |
| Mains / SELV class | SELV only (24 V DC); no mains on the board |
| ESD exposure | TBD — Karl. Field-wired push-in terminals; USB header used only on the bench |
| EMC target | None formal at prototype; keep switching loops small near the BLE radio (§7 rule 7) |
| Enclosure | `FEHA-LSC-001-02`: slide-in tube, two identical snap-on end caps, 119.4 × 27.8 × 15.8 mm |
| Test strategy | TBD — Karl (proposed: functional, JLCPCB PCBA, small batches; mirrors `review-profile.yml`) |

### 1.1 Hard constraints and requirements (with sources)

| ID | Constraint | Source |
|---|---|---|
| CON-1 | Load 24 V COB strip, 8 W/m: 1.5 m (0.5 A) normal use, rated to 3 m (1.0 A) continuous at 100 % on with no time limit | DEC-01 |
| CON-2 | Board outline 66 × 20 mm (placeholder); no part taller than the 4.5 mm connectors | Enclosure model, DEC-16 |
| CON-3 | Top side clear within 1.5 mm of both long edges over the last 10 mm at each end (hold-down ribs) | Enclosure model, DEC-16 |
| CON-4 | Tube inner wall < 65 °C at 3 m / 1 A after soak (PETG softens ~80 °C, connectors rated 85 °C) | DEC-01, §10 |
| CON-5 | Output stays off through power-up, ROM bootloader and flashing | DEC-03 |
| CON-6 | Whole board, including the strip, survives a reversed 24 V input | DEC-09 |

### 1.2 Deliberate simplifications (each with an exit)

- **No hot-plug protection** (DEC-11): the board is never connected to a live 24 V lead. Exit:
  if that stops holding, add the input damping network and rerun a hot-plug test (VIN peak < 36 V).
- **No overcurrent protection for the output MOSFET itself** (DEC-12): the fuse limits the
  aftermath of a shorted output, not the MOSFET. Exit: a current-limited high-side switch.

## 2. Power architecture

```
24 V IN ─ fuse ─ +24 V ─┬─ TVS ─ (board GND)
                        ├─ LED+ (OUT connector)
                        ├─ 1 µF local bypass at the output stage
                        └─ TPS560430X3F FPWM buck ─ +3V3 ─ ESP32-C3 module, status LED
USB 5 V (debug header) ─ B5819W SL ─ buck VIN
24 V return ─ reverse-polarity MOSFET ─ board GND
```

- **Buck** (DEC-07): TI TPS560430X3F, fixed 3.3 V, forced PWM at 1.1 MHz (no PFM bursts, so no
  audible singing), 600 mA rating against ≈ 350 mA peak load (ESP32-C3 BLE/Wi-Fi peaks).
  Adjustable TPS560430XF (C523980) is the stock fallback.
- **Inductor** (DEC-08): 10 µH molded, Isat 2.2 A against the IC's 1.4 A maximum peak limit; TI's
  table suggests 12 µH. At 24 V in, ripple ≈ 0.26 A, peak ≈ 0.48 A, under the 0.8 A minimum
  current limit. Verify on the scope at bring-up.
- **USB 5 V** (DEC-14): via a Schottky into buck VIN, for flashing without the 24 V supply.
- **Supply requirement:** TBD — Karl (current limit of the intended 24 V supply; see §3 fuse).

## 3. Input and protection

- **Reverse polarity** (DEC-09): SI2356DS in the negative line. Drain to the input − terminal,
  source to board GND, gate pulled to +24 V through 100 kΩ, BZT52C10 zener gate → source (cathode
  at gate). Normal: gate ≈ 10 V (zener max 10.5 V, inside the ±12 V gate limit), ≤ 51 mΩ.
  Reversed: body diode blocks; the zener conducts forward and holds the gate ≈ −0.7 V, so the
  MOSFET stays off and its gate never sees −24 V. Board GND ≠ input − terminal.
- **TVS** (DEC-10): SMBJ26A across +24 V and board GND, after the reverse MOSFET. Standoff 26 V,
  breakdown 28.9–31.9 V, below the MOSFETs' 40 V and the buck's 38 V abs max. Its 42.1 V clamp
  figure applies only at the full 14 A pulse rating.
- **Fuse** (DEC-12): Littelfuse 0466002.NRHF, 2 A very fast-acting 1206, 63 V, in the +24 V
  input ahead of the TVS. Sized 1.0 A ÷ 0.75 (Littelfuse standard 25 % rerating) ÷ 0.9 for
  temperature, next standard size up. It limits sustained overcurrent after a shorted output or a
  TVS that fails short; it does not save the output MOSFET.
- **No bulk electrolytic** (DEC-11): it set the enclosure height.

## 4. Sensor / analogue front end

Not applicable: no sensors on the board. Triggers arrive over BLE.

## 5. ADC / measurement

Not applicable. Thermal fallback uses the ESP32-C3 internal temperature sensor (§8).

## 6. MCU and interfaces

| Signal | Pin | Notes |
|---|---|---|
| PWM to output MOSFET gate | GPIO7 (MTDO) | No internal pull at reset, 5 ns low glitch only (DEC-03) |
| Button | GPIO9 | Boot/recovery strap, pin-hole in the enclosure (DEC-15) |
| Status LED | GPIO10 | 5 ns low glitch at reset (harmless) |
| USB D− / D+ | GPIO18 / GPIO19 | Native USB-Serial-JTAG on the 1×4 debug header (DEC-14) |

Avoid for the PWM output: GPIO6 (pull-up at reset), GPIO18/19 (USB; GPIO18 has a 50 µs high
glitch at power-up), GPIO20/21 (UART, pulled up), GPIO2/8/9 (strapping pins).

### 6.1 Output stage

- **Switch** (DEC-02): Vishay SI2356DS low side, gate driven directly from GPIO7. ≤ 70 mΩ
  guaranteed at a 2.5 V gate, V_GS(th) 0.6–1.5 V, 40 V, V_GS ±12 V. Same part as the reverse
  MOSFET.
- **Gate network** (DEC-04): 100 Ω series (≈ 55–60 ns drain edges), 10 kΩ pull-down to source
  (holds the gate low even against an internal 45 kΩ pull-up), GPIO at maximum drive strength.
- **Freewheel** (DEC-05): SS34 Schottky across the strip, cathode to LED+ (DC-only output).
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
| Reverse-protection SI2356DS (≤ 51 mΩ at 10 V gate) | ≤ 0.015 W | ≤ 0.06 W |
| ESP32-C3 + buck | ≈ 0.35 W | ≈ 0.35 W |
| Air inside the tube | ≈ 43 °C | ≈ 45 °C |
| Output MOSFET junction (Tj max 150 °C) | ≈ 50 °C | ≈ 65 °C worst |

Worst case = datasheet maximum on-resistance at a 2.5 V gate (70 mΩ) × 1.2 for a warm junction;
switching loss from ≈ 55–60 ns per edge (100 Ω gate + ≈ 17 Ω GPIO driver, typical Qgd 0.81 nC
at a ≈ 1.6 V plateau, datasheet p.3), ≈ 80 ns with a hot junction. SI2356DS: RthJA 175 °C/W max
steady state on 1" × 1" FR4, junction-to-foot (drain) 75 °C/W max. SOT-23 has no exposed pad,
so the drain copper is the heatsink. The real limit is the PETG tube and the 85 °C connectors.

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
4. **Ground plane:** solid bottom-layer GND plane under everything except the module antenna
   keep-out; stitch top-layer GND pours to it. The plane is the main heat spreader.
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
8. **Input:** fuse directly at the IN connector's + pin, then the TVS; TVS after the
   reverse-protection MOSFET.
9. **Test access:** a bare copper test pad on the output drain pour for a thermocouple, and
   test points on drain and GND for measuring V_DS at full on during the soak test.

## 8. Firmware

ESPHome (see `ESPHome/` in this repo). Hardware/firmware contract:

- PWM on GPIO7, 19.5 kHz, 12-bit LEDC, `GPIO_DRIVE_CAP_3`; full brightness = static high.
- A minimum duty and a brightness lookup table, because the first few PWM codes give no light.
- Thermal fallback: read the internal temperature sensor; above a threshold set from the soak
  test, dim rather than switch off, so retriggering keeps working.
- Configuration: BLE GATT setup page (LE Secure Connections, 6-digit passkey on the label);
  button long-press opens a SoftAP serving the same page for 10 min; 10 s hold = factory reset.

## 9. Bill of materials and sourcing

JLCPCB PCBA. Extended (fee) lines: ESP32-C3 module, buck, inductor, connector, MOSFET, fuse = 6.
No no-fee power inductor exists at JLCPCB, so a zero-fee buck is not possible.

| Function | Part | LCSC |
|---|---|---|
| Output and reverse MOSFET (×2) | Vishay SI2356DS-T1-GE3 | C74127 |
| Gate series / pull-down | 100 Ω / 10 kΩ 0402 | C25076 / C25744 |
| Reverse-MOSFET gate pull-up / zener | 100 kΩ 0402 / BZT52C10 | C25741 / C19077408 |
| Freewheel Schottky | SS34 | C8678 |
| Output-stage bypass | Samsung CL21B105KBFNNNE 1 µF 50 V X7R 0805 | C28323 |
| Buck / inductor | TI TPS560430X3F / cjiang FXL0420-100-M | C2071721 / C177242 |
| TVS / fuse | SMBJ26A / Littelfuse 0466002.NRHF | C19077580 / C3105 |
| USB Schottky | B5819W SL | C8598 |
| IN and OUT connectors (×2) | HDGC4001SMD-S-2P push-in, 18–24 AWG | C5197184 |

## 10. Verification and bring-up plan

- **Test-point plan:** bare copper pad on the output drain pour for a thermocouple; test points
  on the output drain and GND for V_DS at full on. Further rail test points: TBD with the schematic.
- **Functional test:** TBD — Karl, once the test strategy (§1) is set. Candidates: strip
  on/off/fade, button, status LED, BLE trigger from a Shelly BLU device.
- **Soak test (3 m / 1 A only):** closed printed tube, 60 min at full brightness, then 60 min at
  90 % PWM; thermocouples on the output MOSFET drain copper and the tube's inner wall; measure
  V_DS at full on. Pass: tube wall < 65 °C (CON-4). Fail: move the output switch to AOS AON7264E
  (DFN 3×3, exposed pad, new footprint). 1.5 m needs no soak test.
- **Buck:** check ripple and inductor current on the scope at 24 V in (DEC-08).
- **Bring-up order:** TBD with the schematic.

## 11. Decision log

| ID | Date | Decision | Rationale |
|---|---|---|---|
| DEC-01 | 2026-10-01 | Product is a 24 V COB LED-strip controller, not a relay module; new product number `FEHA-LSC-001` | Change of role; relay board `FEHA-RM-001` Rev A stays as built |
| DEC-02 | 2026-10-02 | Output and reverse MOSFET: Vishay SI2356DS | Guaranteed ≤ 70 mΩ at a 2.5 V gate. Rejected: HL2310A (specified only at ≥ 4.5 V), AO3422 (≤ 200 mΩ at 2.5 V), AO3400A (30 V), photo-MOSFET SSRs (too slow for PWM) |
| DEC-03 | 2026-10-02 | PWM on GPIO7 | No pull at reset; continuity with Rev A. GPIO0/1/3/4/5 equally safe |
| DEC-04 | 2026-10-02 | 100 Ω gate series, 10 kΩ pull-down | Ample drive margin at a 1.6 V plateau; slower edges near the BLE radio for ≈ +15 mW. 33 Ω and 100 kΩ rejected |
| DEC-05 | 2026-10-01 | SS34 freewheel across the strip; 1 µF local bypass at the output stage | DC-only output; turn-on edge supplied locally |
| DEC-06 | 2026-10-01 | 19.5 kHz, 12-bit PWM; full brightness = static high | Inaudible, smooth fades; no switching loss in the commonest state |
| DEC-07 | 2026-10-01 | TPS560430X3F forced-PWM buck | No PFM singing. AP63201 FPWM rejected (no stock); AP63203 (Rev A) removed |
| DEC-08 | 2026-10-01 | 10 µH FXL0420-100-M inductor | Isat margin over the 1.4 A peak limit; verify on scope |
| DEC-09 | 2026-10-01 | Reverse-polarity N-MOSFET in the negative line | ≤ 0.06 W at 1 A against ≈ 0.45 W for the series SS34 it replaced |
| DEC-10 | 2026-10-01 | SMBJ26A TVS after the reverse MOSFET | Clamps supply overshoot below the buck's 38 V abs max |
| DEC-11 | 2026-10-02 | Hot-plug waived: no damping network, no hot-plug test, no bulk electrolytic | Board never connected to a live lead; electrolytic set the enclosure height |
| DEC-12 | 2026-10-02 | Littelfuse 0466002.NRHF 2 A fuse in the +24 V input | No overcurrent protection existed; very fast-acting is fine with no inrush |
| DEC-13 | 2026-10-01 | Push-in 2-pin connectors in and out (HDGC4001SMD-S-2P) | Tool-free field wiring, 18–24 AWG |
| DEC-14 | 2026-10-01 | USB-C and USBLC6 removed; 1×4 press-fit debug header, 5 V via B5819W SL | Bench-only access; same Schottky as Rev A D6 |
| DEC-15 | 2026-10-01 | GPIO9 button and one status LED; strip used as feedback. Red error and green power LEDs removed | Enclosure is closed; fewer parts |
| DEC-16 | 2026-10-01 | Inline slide-in tube enclosure (`FEHA-LSC-001-02`) | Sets CON-2 and CON-3 |
| DEC-17 | 2026-10-01 | Not doing: mmWave footprint, joining home Wi-Fi | Out of scope for a BLE-triggered light |

**Baseline rule:** until the first schematic gate closes, this table is a *baseline* — rows are
edited in place, not superseded. After that gate, a changed decision gets
`superseded-by: DEC-mm` appended to its row — never a strikethrough, never deletion.

## Appendix A — datasheet provenance

Vendor PDFs in `../datasheets/` are ground truth.

| Part | File | Note |
|---|---|---|
| SI2356DS | `Vishay-SI2356DS.pdf` | Doc 62893 Rev. A; gate charge p.3 |
| TPS560430 | `TI-TPS560430.pdf` | Current limits, Table 1 L/C values |
| ESP32-C3 | `Espressif-ESP32-C3.pdf` | v2.4; pin reset states Tables 2-1/2-2 |
| ESP32-C3-MINI-1 | `Espressif-ESP32-C3-MINI-1.pdf` | Module (Rev A used MINI-1-H4) |
| BZT52C10 | `hongjiacheng-BZT52C10.pdf` | |
| SMBJ26A | `hongjiacheng-SMBJ26A.pdf` | |
| SS34 | `MDD-SS34.pdf` | |
| B5819W SL | `CJ-B5819W-SL.pdf` | 40 V |
| 0466002.NRHF | `Littelfuse-0466.pdf` | 466 series, very fast-acting, 63 V |
| CL21B105KBFNNNE | `Samsung-CL21B105KBFNNNE.pdf` | |
| FXL0420-100-M | `cjiang-FXL0420-100-M.pdf` | Series catalogue |
| HDGC4001SMD-S-2P | `HDGC-HDGC4001SMD-S-2P.pdf` | |
