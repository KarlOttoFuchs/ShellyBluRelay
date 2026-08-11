# ESP32-C3 Relay Module - Rev B Change Notes

Running list of changes to fold into the next board re-spin. Nothing here is applied to
Rev A; the Rev A schematic/PCB are unchanged.

---

## 1. L1 — downsize buck inductor (6×6×4.5 mm → 4×4×3.0 mm)

**Status**: Open — apply at Rev B
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
