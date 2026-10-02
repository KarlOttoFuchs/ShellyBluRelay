# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this
repository.

## Session protocol — read this first, every session

Context is lost between sessions; the repository is the memory. At the start of every session,
in this order:

1. This file.
2. `Docs/design/led-strip-controller-design.md` **§0** — the pickup point: what is settled, open,
   blocked, next. Then the decision log **§11** (`DEC-nn`).

At the end of a session ("make a note for us"): update the spec §0 (and its `Last updated:`
stamp), propose a checkpoint commit, and update the session-resume memory. Anything learned about
*how* a tool behaves goes into the repo docs, not into chat.

## Project overview

This repository holds two products (see `Docs/design/part-numbering.md`):

- **LED Strip Controller** (`FEHA-LSC-001`, in design) — 24 V COB LED-strip controller, Shelly
  BLU / BLE triggered, ESP32-C3. PCB `FEHA-LSC-001-01` Rev A, enclosure `FEHA-LSC-001-02` Rev A.
  The design spec below is this product's.
- **Relay Module** (`FEHA-RM-001` Rev A, fabricated) — the original ESP32-C3 relay board. Its
  KiCad files are frozen; it predates this layout and keeps its directory name.

- `Hardware/FEHA-LSC-001-01-Controller-Rev-A/` — the controller's KiCad project (schematic, PCB, project-local library, scripts).
- `Hardware/ESP32C3-Relay-Module-Rev-A/` — the relay module (fabricated; uses machine-global libraries).
- `Enclosure/FEHA-LSC-001-02-Enclosure-Rev-A/` — the controller's printed enclosure (FreeCAD, parametric script, STLs).
- `Docs/design/` — conventions, design spec, `figures/`, `reference/`, `archive/`, `reviews/`.
- `Docs/datasheets/` — vendor PDFs for key parts. **Treat these PDFs as ground truth** for any
  electrical value, pinout or pad map — read them directly with the `Read` tool (no OCR or
  conversion step needed) rather than relying on scraped/OCR'd sources.
- `Firmware/` — ESP-IDF firmware; `ESPHome/` — the relay module's ESPHome configuration (both out of scope for hardware sessions).

## Working agreement (Karl ↔ Claude)

- **Karl draws and wires the schematics in KiCad himself.** Claude does design work: topology
  decisions, part selection, calculations, datasheet verification, documentation, and field/BOM
  hygiene. Do not add or rewire schematic symbols unless explicitly asked (field/property edits
  and no-connect flags via `kicad-mcp` are fine — that's the agreed exception).
- When unsure what Karl has in mind, ask before acting — he has said this explicitly.
- **Keep chat responses short.** Lead with the answer, the numbers, or the code. He is an
  experienced hardware engineer and will interrupt a derivation he did not ask for. Committed
  *documents* are the exception — design specs and review registers are written to be read later
  and should be complete. KiCad guidance is the other exception: give the full click path.
- **Design decisions live in the design spec under `Docs/design/`** — §0 is the session pickup
  point (stamped `Last updated:`, the **only** status location in the spec; artifact/build
  status stays in the revision README), §11 is the decision log (dated `DEC-nn` rows owning the
  rationale incl. rejected alternatives; sections state the live fact and cite `(DEC-nn)`).
  **Baseline rule:** until the first schematic gate closes, rows are edited in place; after it,
  supersede by `superseded-by:` pointer, never strikethrough. Read §0 and the decision log
  before proposing design changes; a decision recorded there is settled unless Karl reopens it.
  Record new decisions there (decisions only — keep process notes out).
- **Checkpoint commits at milestones.** After a verified schematic change, doc-decision update,
  or completed cleanup, propose a commit (Karl usually wants one; still wait for his go-ahead).

## Standing design constraints

- **Sourcing: JLCPCB assembly, prefer no-setup-fee (basic/preferred) parts.** An extended part
  with a setup fee needs Karl's explicit sign-off, recorded in the decision log. Check with
  `mcp__pcbparts__jlc_search` / `jlc_get_part`.
- **Hand-solderability:** for any part Karl may plausibly hand-rework (value-tuning resistors,
  DNP options), prefer **0603 or larger**. 0402 is fine for fixed passives.

## Definition of done — any schematic/library change

Run this checklist after **every** change, including single-component ones (a one-capacitor swap
has the same failure modes as a bulk import):

1. **New part imported to the project library** → run the `kicad-part-fields` skill (fields +
   pin-type normalisation + pin-number↔pad consistency check). Never skip the pin/pad check on
   multi-pin parts.
2. **Component added/changed in the schematic** (from JLCPCB/stock libraries) → run the
   `kicad-schematic-fields` skill on the touched references. Not optional, and not only for bulk
   audits — missed single parts slip through otherwise.
3. **Before any bulk `kicad-mcp` edit pass**: confirm no live `eeschema`/`kicad` process holds
   the file (`pgrep -fl kicad`, and check for a `~*.lck`) — a stale GUI save silently reverts
   on-disk edits. The `kicad-mcp` server's own Python processes match that `pgrep` and are
   **not** a GUI; check for `KiCad.app/Contents/MacOS/{kicad,eeschema,pcbnew}` specifically.
4. **Run ERC** (`mcp__kicad-mcp__run_erc`) and report the result honestly.
5. Propose a checkpoint commit.

For bulk (>~5 refs) schematic field audits use
`~/.claude/skills/kicad-schematic-fields/scripts/schematic_fields_audit.py` — one read pass
instead of per-component MCP round-trips. Writes still go via `kicad-mcp`.

## Design reviews

Use the **`hardware-design-review`** skill.

- One register file per pass: `Docs/design/reviews/<gate>-<YYYY-MM-DD>-<board>.md`.
- Profile and carried deferrals live in `Docs/design/review-profile.yml`.
- **Connectivity comes from a fresh `kicad-cli` netlist**, generated that session from the saved
  `.kicad_sch` with no live eeschema process — never from OCR of a schematic PDF. Values/pins
  from the vendor PDF in `Docs/datasheets/`.
- Findings graded **Blocker / Major / Minor / Advisory**, plus a coverage table and carried
  deferrals so settled items are not re-raised.
- Re-derive every finding from the fresh netlist. Never carry a reference designator across
  passes on trust — re-annotation shuffles them.

## Conventions

- **Part numbering and slugs:** `Docs/design/part-numbering.md` — `FEHA-LSC-001-01`/`-02` Rev A, `FEHA-RM-001` Rev A. Directories
  and KiCad file stems are `<PN>-<Slug>-Rev-<X>`. Same-copper populate options are KiCad design
  variants of one assembly; different copper is a new assembly number.
- **Documentation structure and cross-referencing:** `Docs/design/hardware-documentation.md`.
  Read it before writing into any `README.md`, `DESIGN-CHANGES.md` or design spec.
- **Change records:** each revision's `DESIGN-CHANGES.md`, append-only, `CHG-nn`, severities
  matching the review register. `Implemented` = drawn in the schematic; `Verified` = proven on
  fabricated hardware. Cite across assemblies as `<part-number> CHG-nn`.

## Tooling notes

- `kicad-cli` is **not on PATH**. Use the full path:
  `/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli`.
- The KiCad files here are **KiCad 10 format** (`version 20260306`). Don't copy library files in
  from older projects verbatim without letting KiCad upgrade them.
- `design_diff.py` (at `~/.claude/skills/kicad-revision/scripts/design_diff.py`) proves a
  refactor is electrically neutral. Use it for any library repoint, rename or bulk edit.
- New revisions are created with the `kicad-revision` skill, never by hand-copying.

## KiCad project layout

`Hardware/FEHA-LSC-001-01-Controller-Rev-A/` is the KiCad project directory:

- `FEHA-LSC-001-01-Controller-Rev-A.kicad_pro` / `.kicad_sch` / `.kicad_pcb`.
- `Library/_FEHA-LSC-001.kicad_sym` + `Library/_FEHA-LSC-001.pretty/` — the **project-local** symbol and
  footprint libraries (leading underscore is the convention), wired in via `sym-lib-table` /
  `fp-lib-table` pointing at `${KIPRJMOD}/Library/...`. **The nickname is the product part
  number, not the directory name** — it is embedded in every `lib_id`.
- `Library/3DModels/` — STEP models, referenced as `${KIPRJMOD}/Library/3DModels/<name>.step`.
- `Library/_FEHA-LSC-001.kicad_blocks/` — the project **design-block** library, wired in via
  `design-block-lib-table`, for reusable schematic/layout fragments. Reuse across boards goes
  through design blocks placed as hierarchical sheets with kept annotations — never through a
  sheet file shared by path between projects (KiCad advises against it; per-project annotation
  is written into the file).
- `scripts/lcsc_to_kicad.py` — imports LCSC/EasyEDA parts into the project library.
- `fabrication-toolkit-options.json` — config for the KiCad Fabrication Toolkit plugin.

**Library policy:** this repo is **self-contained**. Custom/vendor parts live in `_FEHA-LSC-001`.
Stock KiCad and JLCPCB PCM libraries (`Device:*`, `power:*`, `Connector:*`, `PCM_JLCPCB-*`) stay
as global references — only custom/vendor parts get localised.

Use the `kicad-mcp` MCP tools for any interaction with the project rather than hand-editing the
s-expression files, except for the specific scripted workflows below.

## Importing parts from LCSC/EasyEDA

Use `Hardware/FEHA-LSC-001-01-Controller-Rev-A/scripts/lcsc_to_kicad.py <LCSC_ID>` (e.g. `C3013946`) to pull a part's
symbol, footprint and 3D model into the project library — do not call the bare `easyeda2kicad`
CLI directly (EasyEDA's CDN intermittently blocks its TLS fingerprint; the script fetches via
`curl` and feeds the importer directly, then relocates the `.step` into `Library/3DModels/`).

**Pin-numbering caveat — always verify after import:** some EasyEDA source symbols carry
per-side sequential pin *numbers* that do not match the real footprint pad numbers even though
the pin *names* are correct. After importing a multi-pin part, confirm the set of symbol pin
numbers equals the set of footprint pad numbers; if not, renumber every named pin to the
manufacturer datasheet's pad map (vendor PDF in `Docs/datasheets/`, not LCSC's anti-bot-gated
page).

## Symbol metadata convention

Every project-library symbol carries: `Reference`, `Value` (=MPN), `Footprint`, `Datasheet`,
`Description`, `Manufacturer`, `Mfr. Part #`, `LCSC Part #`, `Package` — with only
Reference/Value visible. Use the `kicad-part-fields` skill rather than doing this by hand.

## Firmware

The LED Strip Controller firmware is **ESP-IDF v5.5** (`Firmware/`). The hardware/firmware
contract (PWM pin, drive strength, dimming curve, thermal cut-off) is the design spec §8. Use the
**`esp-idf` skill** for any `idf.py` command. `ESPHome/` is the relay module's configuration and
is not used for this product.
