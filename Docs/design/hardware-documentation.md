# Hardware documentation structure

## Layout

```
Docs/design/          product-level, survives any single revision
    part-numbering.md              the scheme and the allocation table (PN + slug)
    hardware-documentation.md      (this file)
    led-strip-controller-design.md     the design spec — §0 is the session pickup point
    review-profile.yml             hardware-design-review config; deferrals persist here
    reviews/                       one register file per review pass
    figures/
    reference/                     vendor app notes, worked reference designs
    archive/                       verbose discussion superseded by the spec
Docs/datasheets/      vendor PDFs — ground truth for any electrical value
Hardware/
    README.md         the assembly index — one row per assembly, with revision lineage
    <PN>-<Slug>-Rev-<X>/           one directory per revision of each assembly
        README.md                  what this revision is
        DESIGN-CHANGES.md          what it changes vs its baseline
        <PN>-<Slug>-Rev-<X>.kicad_pro / .kicad_sch / .kicad_pcb
        sym-lib-table  fp-lib-table  design-block-lib-table   → ${KIPRJMOD}/Library/
        Library/                   project-local symbols, footprints, 3D models, design blocks
        scripts/                   project-scoped tooling (lcsc_to_kicad.py)
        production/                fab outputs — tracked, they are deliverables
Firmware/
```

Directory casing is capitalised (`Hardware/` / `Docs/` / `Firmware/`). The revision directory
and the KiCad file stem inside it are the **same string**, `<PN>-<Slug>-Rev-<X>`, so a board is
self-identifying in backups, exports and fab uploads.

**Placement test:** if it stops being true when a revision is superseded, it belongs with the
revision. Adapter wiring for one board's mistake lives with that board; the numbering scheme
lives in `Docs/design/`.

## Fab outputs

Fab exports live in the revision's `production/` directory. The upload archive is named from the
same stem plus an optional KiCad variant code, the fab house and the date:

```
<PN>-<Slug>-Rev-<X>[-<VARIANT>]-<fab>-<YYYYMMDD>.zip      e.g. FEHA-LSC-001-01-Controller-Rev-A-JLCPCB-20260901.zip
```

Never inherit a previous revision's `production/` — regenerate outputs for each revision.

## README vs DESIGN-CHANGES

Their lifecycles differ, which is why they are separate files:

- `README.md` — what this revision *is*. Rewritten as the revision matures.
- `DESIGN-CHANGES.md` — what it *changes* against its baseline. Append-only; becomes history.

Neither restates the other, and neither carries design *reasoning* — that lives in the spec.

`Implemented` means drawn in the schematic. `Verified` means proven on the fabricated board.
Keeping those apart is what stops a change being assumed done. The record freezes at fab;
anything later opens the next revision.

Severities (`Blocker` / `Major` / `Minor` / `Advisory`) match the `hardware-design-review`
findings register, so its output drops in untranslated.

## Cross-referencing between assemblies

- **Name the part number, link the path.** Part numbers survive directory renames; paths do not.
- **Relative links only** — repos get cloned to different paths.
- **Cross-assembly coupling goes in the originating change's `Impact` field, recorded once.**
  No mirrored entries on both boards; they drift.
- **Cite a change as `<part-number> CHG-nn`.** Bare `CHG-01` is ambiguous when every assembly
  numbers from 1.
- **Put the change ID in commit subjects** so `git log --grep=CHG-01` reconstructs its history.
  Detail lives in history; intent lives in the document.

## Design reviews

Use the `hardware-design-review` skill. One register file per pass:
`Docs/design/reviews/<gate>-<YYYY-MM-DD>-<board>.md`. Deferrals and settled findings are
recorded in `review-profile.yml` so they are carried, not re-raised.

**Connectivity comes from a fresh `kicad-cli` netlist**, generated that session from the saved
`.kicad_sch` with no live eeschema process — never from OCR of a schematic PDF. Values and pins
come from the vendor PDF in `Docs/datasheets/`.

Re-derive every finding from the fresh netlist. Never carry a reference designator across passes
on trust — re-annotation shuffles them.
