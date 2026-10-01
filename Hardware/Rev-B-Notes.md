# ESP32-C3 Relay Module - Rev B Change Notes

Running list of changes to fold into the next board re-spin. Nothing here is applied to
Rev A; the Rev A schematic/PCB are unchanged.

---

## 0. Rev B direction — settled 2026-10-01

Rev B is redirected from a relay module to a **24 V LED-strip controller** for under-cabinet
COB strips and cupboard lights, triggered by Shelly BLU sensors. Settled in design discussion:

| Area | Decision | Part (LCSC) |
|---|---|---|
| Load | 24 V COB strip, 8 W/m. Main use 1.5 m = 12 W, 0.5 A. Rated up to 3 m = 24 W, 1.0 A continuous, subject to the soak test below | — |
| Output | Low-side N-MOSFET, gate driven directly from GPIO (valid to ≈1 A; on-resistance at 3.3 V gate is typical-curve only, check V_DS at full on during bring-up) | hongjiacheng HL2310A, 60 V / 3 A (C7420347) |
| | 33 Ω gate series (faster edges, lower switching loss), GPIO at maximum drive strength (`GPIO_DRIVE_CAP_3`), 100 kΩ gate pull-down (off through reset/flash) | C25105, C25741 |
| | Freewheel Schottky across the strip (DC-only output) | SS34 (C8678) |
| | Local +24 V → GND bypass at the output stage: supplies the fast current edge at each turn-on locally instead of from the buck's input capacitors at the far end of the board. 1 µF 50 V X7R 0805; at 24 V DC bias expect roughly half that, which is enough for edge current | Samsung CL21B105KBFNNNE (C28323) |
| | PWM 19.5 kHz, 12-bit (LEDC) — inaudible, smooth fades | firmware |
| Firmware rules | (1) Full brightness = output held permanently high (LEDC at full-scale duty), never 99.x % PWM, so the commonest case has no switching loss. (2) Thermal fallback: read the ESP32-C3 internal temperature sensor; above a threshold set from the soak test, dim gently (e.g. to 70 %) rather than switch off, so retriggering keeps working | firmware |
| Buck | Forced-PWM 3.3 V buck (no PFM bursts → no singing; AP63201 FPWM part had 1 in stock) | TI TPS560430X3F (C2071721); adjustable TPS560430XF (C523980) as fallback |
| | 10 µH molded inductor, 2.2 A Isat vs 1.4 A IC peak limit (TI table suggests 12 µH; verify on scope) | cjiang FXL0420-100-M (C177242) |
| Input protection | Reverse polarity for the whole board incl. strip: N-MOSFET in the negative line — drain to the input − terminal, source to board GND, gate from +24 V through 100 kΩ, 10 V zener gate→source (cathode at gate). Normal polarity: fully on at ~10 V gate, ≤ 105 mΩ → ≤ 0.03 W at 0.5 A, ≤ 0.1 W at 1 A. Reversed: gate negative, body diode reverse-biased, blocks. Same part as the output switch (one part number). Replaces the series SS34, which dissipated ~0.2 W at 0.5 A and ~0.45 W at 1 A. Note board GND ≠ input − terminal. TVS after the MOSFET clamps hot-plug ringing (ceramic-only input can ring towards 2 × 24 V vs the buck's 38 V abs max). No bulk electrolytic (dropped 2026-10-01: TVS covers it, and it set the enclosure height). Unfitted footprint for a series R + ceramic C damping network across the input, fitted only if the hot-plug test fails | HL2310A (C7420347); BZT52C10 (C19077408); 100 kΩ (C25741); SMBJ26A (C19077580); damping R/C basic parts, DNP |
| Bring-up tests | (1) Hot-plug: plug the live 24 V lead in ~10 times while scoping VIN at the buck. Pass: peak < 36 V. Fail: fit the damping network and repeat. (2) Soak, 3 m / 1 A: closed printed tube, 60 min at full brightness, then 60 min at 90 % PWM; thermocouples on the output MOSFET drain copper and the tube's inner wall; measure V_DS at full on. Pass: tube wall < 65 °C (PETG softens ~80 °C, connectors rated 85 °C). Fail: move the output switch to AOS AON7264E (DFN 3×3, exposed pad, extended part, new footprint). 1.5 m needs no soak test (< 0.1 W per MOSFET) | — |
| Connectors | Push-in, 2-pin, in and out (18–24 AWG) | HDGC4001SMD-S-2P ×2 (C5197184) |
| USB | USB-C and USBLC6 removed. 1×4 offset-hole press-fit header (5V via Schottky → VIN, D+, D−, GND) for native USB-Serial-JTAG flash/console/debug | footprint only |
| UI | GPIO9 button kept (boot/recovery, pin-hole in enclosure); one status LED (GPIO10); strip itself used as user feedback | — |
| Configuration | Day to day: BLE GATT setup page (Web Bluetooth; Chrome on Mac, Bluefy on iPhone), LE Secure Connections with a 6-digit passkey on the label. Fallback: button long-press → on-device SoftAP serving the same page (works in Safari, no app, no internet) for 10 min. Factory reset: 10 s hold | firmware |
| Removed | Relay + driver, AP63203 + VLS6045, SMBJ24A, B5819W, series SS34 reverse diode, USB-C, USBLC6, red error LED, green power LED | — |
| Not doing | Photo-MOSFET SSR on the output (GAQY252G3S, Letex LT218: no PWM at ~0.4–0.8 ms turn-on, LT218 only 40 V), mmWave footprint, joining home Wi-Fi, AO3400A for reverse protection (lower loss, but 30 V and a second part number; the saving is ~0.02 W at 1.5 m) | — |

Extended (fee) lines: ESP32-C3 module, buck, inductor, connector = 4. No no-fee inductor exists
at JLCPCB, so a zero-fee buck is not possible.

**Enclosure:** inline slide-in tube with two identical end caps (one STL, printed twice), each
with a screw tab centred under its cable exit; caps click on with snap detents, no screws or
inserts (once mounted, the two tab screws fix the caps and trap the tube); overall
119.4 × 27.8 × 15.8 mm —
concept and dimensions in [`../Enclosure/Rev-B/enclosure-concept.html`](../Enclosure/Rev-B/enclosure-concept.html),
parametric model `../Enclosure/Rev-B/enclosure_rev_b.py`. Board constraints it imposes:
66 × 20 mm placeholder outline; top side clear within 1.5 mm of both long edges over the last
10 mm at each end (hold-down ribs); no component taller than the 4.5 mm connectors (5.5 mm
headroom, tube 12.2 mm tall); module antenna flush with the +Y edge.

### 0.1 Thermal budget and PCB layout for heat

Steady state is reached in 20–30 min (MOSFET in 1–2 min per HL2310A Fig. 9), so long
retriggering = permanently on; design for 100 % on with no time limit. Estimates, closed tube
in 35 °C ambient, tube shedding ≈ 20 °C/W:

| | 1.5 m / 0.5 A | 3 m / 1.0 A |
|---|---|---|
| Output HL2310A, full on (typ / worst) | 0.03 / 0.08 W | 0.13 / 0.30 W |
| Output HL2310A, ~90 % PWM, 33 Ω gate (worst) | ≈ 0.1 W | ≈ 0.35 W |
| Reverse-protection HL2310A (≤ 105 mΩ at 10 V gate) | ≤ 0.03 W | ≤ 0.1 W |
| ESP32-C3 + buck | ≈ 0.35 W | ≈ 0.35 W |
| Air inside the tube | ≈ 45 °C | ≈ 48–50 °C |
| Output MOSFET junction (Tj max 150 °C) | ≈ 50 °C | ≈ 65 °C typ, 90–105 °C worst |

HL2310A datasheet (Rev 2.1): Tj max 150 °C, RθJA 104 °C/W (mounting not stated; on this board
expect 130–180 °C/W), PD 1.2 W at 25 °C. SOT-23 has no exposed pad: heat leaves through the
leads, mainly drain pin 3, so the copper on the drain net is the heatsink. The real limit is
the PETG tube and the 85 °C connectors, not the MOSFET.

**Layout rules**

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
7. **Switching loop:** at every PWM edge up to 1 A moves between two paths in ~100 ns. With
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
8. **Input:** TVS next to the IN connector, after the reverse-protection MOSFET; damping R/C
   footprint (DNP) beside it.
9. **Test access:** a bare copper test pad on the output drain pour for a thermocouple, and
   test points on drain and GND for measuring V_DS at full on during the soak test.

---

## 1. L1 — downsize buck inductor (6×6×4.5 mm → 4×4×3.0 mm)

**Status**: Superseded 2026-10-01 by §0 (TPS560430X3F + FXL0420-100-M)
**Raised**: 2026-08-11
**Affects**: `L1`, BOM, PCB placement near U3

### Current part (Rev A)

| | |
|---|---|
| MPN | TDK **VLS6045EX-6R8M** |
| LCSC | C415364 |
| Value | 6.8 µH ±20% |
| Package | 6.0 × 6.0 × 4.5 mm |
| Ratings | 3.6 A rms / 4.7 A sat, 36 mΩ |
| Footprint | `_Inductor_SMD:IND-SMD_L6.0-W6.0` |

### Why it's oversized

Rail load on +3V3:

| Load | Current |
|------|---------|
| K1 relay coil (SRD-03VDC-SL-C, 360 mW @ 3 V) | 120 mA |
| U2 ESP32-C3-MINI-1, WiFi TX peak | ~350 mA |
| **Worst case total** | **~0.5 A** |

Ripple current, worst case at 24 V DC input (README specifies USB-C 5 V or external
12–24 V), with U3 = AP63203 at f_SW = 1.1 MHz:

    ΔI_L = V_OUT·(V_IN − V_OUT) / (V_IN · L · f_SW)

| L | ΔI_L @ V_IN = 24 V |
|---|---|
| 6.8 µH | 380 mA |
| 4.7 µH | 550 mA |
| 3.9 µH | 660 mA |

Peak inductor current in normal operation is therefore **~0.85 A** (0.5 A load + half
ripple, taking L at its −20% tolerance corner). Rev A carries a 4.7 A part for that.

### Sizing basis for the replacement

From the AP63203 datasheet (Diodes DS41326 Rev. 3-2, Nov 2024):

- **Table 2** recommends **L = 3.9 µH** for the AP63203 at 3.3 V out. Rev A's 6.8 µH is
  legal (§10 allows 2.2–10 µH) but above the reference design — moving to 4.7 µH moves
  *toward* the datasheet, not away from it.
- **§10** asks for DCR < 100 mΩ for best efficiency, and I_rms ≥ 1.35 × max load.
- **HS peak current limit = 2.5 / 2.8 / 3.1 A** (min/typ/max). This is the real sizing
  driver: on an output short the IC clamps at up to 3.1 A, so choose **Isat ≥ 3.2 A** so
  the core cannot saturate during a fault.

### Selected replacement

| | |
|---|---|
| MPN | cjiang **FNR4030S4R7MT** |
| LCSC | **C167874** (Extended; ~230 k stock, $0.050 @1, $0.041 @100) |
| Value | 4.7 µH ±20% |
| Package | 4.0 × 4.0 × 3.0 mm, magnetically shielded |
| Ratings | 2.3 A rms / **3.2 A sat**, **78 mΩ** |

Clears the 3.1 A max current limit, DCR under the 100 mΩ guidance, cheapest of the
candidate set, deepest stock. Board area 36 mm² → 16 mm², height 4.5 → 3.0 mm.

### Alternates considered

| Part | LCSC | Size (mm) | Isat | Irms | DCR | Price | Note |
|---|---|---|---|---|---|---|---|
| FHD4020S-4R7MT | C602031 | 4×4×**2.0** | 4.9 A | 3.3 A | 95 mΩ | $0.067 | Pick this if enclosure height binds. **Verify 4.9 A Isat against the manufacturer PDF** — aggressive for a 4020. |
| FNR4020S4R7MT | C167828 | 4×4×2.0 | 2.5 A | 2.0 A | 98 mΩ | $0.053 | Isat below the IC's 3.1 A limit — can saturate on a short. |
| FTC303018D4R7MBCA | C5832388 | **3×3×1.8** | 4.7 A | 3.4 A | 72 mΩ | $0.207 | Smallest, but 4× price and needs a new land pattern. |
| VLS3012HBX-4R7M (TDK) | C404496 | 3×3×1.2 | 2.5 A | 2.0 A | 175 mΩ | $0.153 | DCR well over the 100 mΩ guidance. |

JLCPCB stocks no Basic/Preferred power inductors in this class — all candidates are
Extended parts.

### Implementation notes

- Footprint **already exists** in the shared library:
  `_Inductor_SMD:IND-SMD_L4.0-W4.0` (pads 1.6 × 3.5 mm on 3.2 mm pitch — the standard
  NR40xx land pattern). No new footprint needed for any 4×4 option.
- The library's 3D model is `IND-SMD_L4.0-W4.0-H1.9.step` (a 4020 body). It will render
  short against a 3.0 mm-tall FNR4030 — cosmetic only, or source an H3.0 STEP.
- Update `L1` symbol fields: Value, Mfr. Part #, Manufacturer, LCSC Part #, Package,
  Description, Datasheet.
- Re-check C_OUT against datasheet Eq. 10 after the L change (Table 2 reference design
  for the AP63203 is C1 = 10 µF in, C2 = 2 × 22 µF out, C3 = 100 nF boot).
- Reclaimed area near U3 is a good place to tighten the SW-node loop during layout.
