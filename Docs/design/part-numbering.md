# Part numbering

## The scheme

```
<PREFIX>-<FAMILY>-<NNN>[-<AA>]

  PREFIX   company or brand           FEHA
  FAMILY   product family             RM (relay module), LSC (LED-strip controller)
  NNN      product serial, 3 digits   001
  AA       assembly within the product, 2 digits — only once there is more than one
```

The part number **never encodes the revision**. `Revision` is a separate KiCad text variable,
starting at `A`.

## Allocation table

Every assembly is allocated a part number **and a slug** — a 1–2-word descriptive name, fixed at
allocation and immutable thereafter. The number is for uniqueness; the slug is for a human
scanning a directory listing. Directories and KiCad file stems are named
`<PN>-<Slug>-Rev-<X>`.

| PN | Slug | What |
|---|---|---|
| `FEHA-RM-001` | Relay | ESP32-C3 relay module, 5–24 V input (fabricated Rev A; legacy directory name) |
| `FEHA-LSC-001-01` | Controller | 24 V COB LED-strip controller PCB, Shelly BLU / BLE triggered, ESP32-C3 |
| `FEHA-LSC-001-02` | Enclosure | Printed slide-in tube with two identical snap-on end caps |

If an assembly's role changes enough that its slug reads wrong, that is a **new assembly
number**, not a rename — renaming a slug is the directory-renaming failure this table prevents.
Slugs name the hardware, not the use case (`Analogue`, not `Level`; `Thermistor`, not `Geyser`).

## This product

This repository holds two products. `FEHA-RM-001` is a single-assembly product, so it has no
`-<AA>` suffix. `FEHA-LSC-001` has two assemblies from the start, the PCB (`-01`) and the
enclosure (`-02`), so both carry the suffix. Non-PCB assemblies — an enclosure, a wiring
harness — get assembly numbers exactly like PCBs do; their revision directory carries the same
README/DESIGN-CHANGES pair, with CAD exports in place of KiCad files. Because firmware board
names target the *product*, the suffix migration needs no firmware rename.

## The failure this prevents

A single-board product takes the product number. It looks fine for a year. Then a second board
appears, there is nowhere to put it, and someone invents a fourth segment with a different
meaning — `FEHA-LSC-BAT-001` — so `-001` now means "product serial" in one number
and "board serial" in another. Keeping the product number and adding a dash suffix avoids this.

## Two rules that matter

**Never renumber a fabricated revision.** Its part number and revision are printed on the board;
changing the files would make the repo disagree with hardware in hand. A repo mid-migration
legitimately shows both shapes.

**Same assembly, or a new one?** Advance the revision when the assembly keeps its role in the
product. Allocate a new assembly number when it does not. Two variants that must both be
orderable at once need two assembly numbers **when their copper differs**. Same copper with a
different population is a **KiCad design variant** of that one assembly, named `<PN>` plus the
variant code in the BOM and the production archive — the board and its silkscreen are the same.

## Silkscreen

The board carries the number via KiCad text variables, so it re-renders from project settings and
never needs a manual silkscreen edit on a revision bump:

```
${Part Number}-${Revision}
```

## KiCad text variables

Set in `Hardware/FEHA-LSC-001-01-Controller-Rev-A/FEHA-LSC-001-01-Controller-Rev-A.kicad_pro`:

| Variable | Value |
|---|---|
| `Company Name` | FEHA |
| `Designed By` | Karl Fuchs |
| `Part Number` | `FEHA-LSC-001-01` |
| `Project Title` | LED Strip Controller |
| `Revision` | `A` |

## Relationship to the project-local library

The KiCad library nickname is derived from the **product** part number with any assembly suffix
stripped — `_FEHA-LSC-001`. It is deliberately *not* the directory name, because the nickname is
embedded in every `lib_id` in the schematic and PCB: a name carrying the revision would need
every `lib_id` rewritten on each revision, for no benefit.

## History

- `FEHA-RM-001` Rev A (relay module) was designed and fabricated before this scheme was
  adopted. Its directory `Hardware/ESP32C3-Relay-Module-Rev-A/` keeps its original name, because
  renaming fabricated history would make the repo disagree with boards in hand.
- 2026-10-02: the board first planned as "Rev B" of the relay module became a 24 V LED-strip
  controller. That is a change of role, so it was allocated a new product number,
  `FEHA-LSC-001`, instead of a revision of `FEHA-RM-001`. Its planning notes are archived in
  [`archive/rev-b-notes-2026-10-02.md`](archive/rev-b-notes-2026-10-02.md), and its enclosure,
  first drawn as `Enclosure/Rev-B/`, became `FEHA-LSC-001-02` Rev A.
- `FEHA-LSC-001-01` Rev A scaffolded 2026-10-02 with `hardware-init`.
