"""
FEHA-LSC-001-02 Rev A - inline tube enclosure for the LED Strip Controller FEHA-LSC-001-01.

Builds a fully parametric FreeCAD document with separate parts that fit together:

    Params    spreadsheet - every dimension, by alias; derived values are formulas
    Tube      PartDesign Body - slide-in tube with the full-length board groove (DEC-29)
    Cap       PartDesign Body - IN-end cap; this is the one printed part, printed twice
    Cap_OUT   App::Link to Cap, turned 180 deg about Z: the OUT cap IS the IN cap
    PCB       the populated controller board, exported from KiCad as STEP (DNP parts left
              out), sitting in the groove

Every feature dimension and position is an expression on Params, so editing a cell in the
FreeCAD GUI and recomputing updates the tube, both caps and the board position together.
The PCB is a reference solid: if the board changes, re-run this script to re-export it.

Board-derived values (outline size, button and LED positions) are read from the KiCad board
file on every run, so the pinhole and LED window follow the placement of record.

Run headless (writes the .FCStd, the STLs and the STEPs, prints the fit check):
    /Applications/FreeCAD.app/Contents/Resources/bin/freecadcmd -c \
        "exec(open('build_enclosure.py').read())"
Or inside FreeCAD (Macro > Macros... > Execute) to also get colours, see-through parts and a
fitted view saved into the .FCStd.

The script is the source of truth: values settled in the GUI belong in PARAMS below,
because a re-run rebuilds the document from scratch.

Coordinate system (same as the board, DEC-33): origin at the tube centre in X and Y, Z up from
the tube's bottom face. X along the tube, IN end at -X. +Y is the antenna side. +Z is the
board's component side; screwed under a cabinet it faces down into the room, so the pinhole
and the LED window are visible. Units: mm.
"""

import os
import re
import subprocess
import tempfile

import FreeCAD as App
import MeshPart
import Part
from FreeCAD import Rotation, Vector

try:
    HERE = os.path.dirname(os.path.abspath(__file__))
except NameError:       # exec() and some macro runners do not set __file__
    HERE = os.getcwd()
if not os.path.isfile(os.path.join(HERE, "build_enclosure.py")):
    HERE = os.path.expanduser(
        "~/Documents/Development/Projects/ESP32C3 Relay Module/Enclosure/"
        "FEHA-LSC-001-02-Enclosure-Rev-A")

DOC_NAME = "FEHA-LSC-001-02-Enclosure-Rev-A"
BOARD_FILE = os.path.normpath(os.path.join(
    HERE, "../../Hardware/FEHA-LSC-001-01-Controller-Rev-A/"
    "FEHA-LSC-001-01-Controller-Rev-A.kicad_pcb"))
KICAD_CLI = os.environ.get("KICAD_CLI", "/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli")
BUTTON_REF = "S1"       # GPIO9 button, top-actuated: pinhole above it
LED_REF = "D3"          # status LED: window above it

# ---------------------------------------------------------------- parameters ----------
# (alias, value, note). Sections become headings in the spreadsheet. "@board" values are
# replaced by what the KiCad board file says.
PARAMS = [
    ("PCB", None, None),
    ("PCB_L", "@board", "board length along X (Edge.Cuts)"),
    ("PCB_W", "@board", "board width along Y (Edge.Cuts)"),
    ("PCB_T", 1.6, "board thickness, nominal (JLCPCB 4-layer)"),
    ("BTN_X", "@board", "button S1 centre X, board origin"),
    ("BTN_Y", "@board", "button S1 centre Y, board origin"),
    ("LED_X", "@board", "status LED D3 centre X, board origin"),
    ("LED_Y", "@board", "status LED D3 centre Y, board origin"),
    ("CONN_WIRE_Z", 3.4, "push-in wire-hole centre above board bottom (measured, STEP)"),

    ("Tube", None, None),
    ("WALL", 1.8, "top and bottom wall"),
    ("WALL_SIDE", 2.8, "side wall, thickened to carry the groove (DEC-29)"),
    ("GROOVE_OVER", 0.7, "groove lip overlap onto the board edge (DEC-29)"),
    ("SIDE_CLR", 0.3, "board edge to groove bottom, each side"),
    ("GROOVE_CLR", 0.3, "groove height over PCB_T (1.9 mm slot)"),
    ("UNDER_GAP", 1.5, "air gap under the board (no parts on the bottom, CON-3)"),
    ("OVER_GAP", 5.5, "headroom above the board top (tallest part 4.44 mm, CON-2)"),
    ("END_PLAY", 0.2, "axial play each end, board to tube end"),
    ("PINHOLE_D", 1.6, "paper-clip hole over the button"),
    ("LED_HOLE_D", 3.2, "LED window (clear resin or a light pipe)"),

    ("Snap detents", None, None),
    ("DET_X", 4.0, "detent from the tube end, inside the cap sleeve"),
    ("DET_R", 1.0, "ridge radius"),
    ("DET_ENGAGE", 0.4, "ridge reach into the tube wall"),
    ("DET_LEN", 6.0, "ridge length along Z, centred on the tube height"),
    ("DET_CLR", 0.05, "groove radius clearance over the ridge"),

    ("Cap", None, None),
    ("CAP_WALL", 1.6, "cap wall thickness"),
    ("CAP_CLR", 0.2, "sleeve clearance around the tube"),
    ("SLEEVE_L", 8.0, "length the cap slides over the tube"),
    ("CHAMBER_L", 12.0, "cable chamber (jacket strip, fan-out, tie stop)"),
    ("PLATE_T", 2.5, "end plate thickness"),
    ("CABLE_D", 6.5, "cable exit hole (twin cable up to ~6 mm)"),
    ("STOP_L", 1.5, "stop ribs at the tube end plane, along X"),
    ("STOP_IN", 4.0, "stop-rib reach inward from the tube's inner side face"),

    ("Screw tab", None, None),
    ("EAR_W", 12.0, "tab width; the end is a full radius of EAR_W / 2"),
    ("EAR_HOLE_X", 6.0, "screw-hole centre beyond the cap end face"),
    ("EAR_T", 3.0, "tab thickness"),
    ("EAR_HOLE_D", 3.6, "clearance for a 3.5 mm wood screw"),
    ("EAR_CSK_D", 7.2, "countersink top diameter"),
    ("GUSSET", 3.0, "gusset leg length, tab to end plate"),
    ("GUSSET_W", 2.0, "gusset thickness along Y, at the tab's outer edges"),
]

# (alias, formula, note): spreadsheet formulas, evaluated by FreeCAD
DERIVED = [
    ("IN_W", "PCB_W - 2 * GROOVE_OVER", "interior width above/below the groove"),
    ("GROOVE_D", "GROOVE_OVER + SIDE_CLR", "groove depth into the side wall"),
    ("GROOVE_H", "PCB_T + GROOVE_CLR", "groove height"),
    ("IN_H", "UNDER_GAP + PCB_T + OVER_GAP", "interior height"),
    ("TUBE_L", "PCB_L + 2 * END_PLAY", "tube length"),
    ("TUBE_W", "IN_W + 2 * WALL_SIDE", "tube width"),
    ("TUBE_H", "IN_H + 2 * WALL", "tube height"),
    ("BOARD_Z", "WALL + UNDER_GAP", "board bottom face"),
    ("GROOVE_Z", "BOARD_Z - GROOVE_CLR / 2", "groove floor"),
    ("WIRE_Z", "BOARD_Z + CONN_WIRE_Z", "cable exit centre height"),
    ("DET_OFF", "DET_R - DET_ENGAGE", "detent axis outside the tube face"),
    ("DET_Z", "(TUBE_H - DET_LEN) / 2", "detent bottom"),
    ("CAP_O", "CAP_CLR + CAP_WALL", "cap outside over the tube face"),
    ("X_END", "-TUBE_L / 2", "IN tube end plane"),
    ("X_OUT", "X_END - CHAMBER_L - PLATE_T", "cap outer end face"),
    ("EAR_HX", "X_OUT - EAR_HOLE_X", "screw-hole centre X"),
    ("EAR_Z1", "-CAP_O + EAR_T", "screw-tab top face"),
    ("CSK_H", "(EAR_CSK_D - EAR_HOLE_D) / 2", "countersink depth (90 deg)"),
    ("OVERALL_L", "TUBE_L + 2 * (CHAMBER_L + PLATE_T + EAR_HOLE_X + EAR_W / 2)",
     "overall length incl. screw tabs"),
    ("OVERALL_W", "TUBE_W + 2 * CAP_O", "overall width (caps)"),
    ("OVERALL_H", "TUBE_H + 2 * CAP_O", "overall height (caps)"),
]

# Display styles in the FreeCAD GUI: (RGB, transparency %)
STYLES = {
    "Tube": ((0.86, 0.82, 0.72), 55),
    "Cap": ((0.45, 0.62, 0.82), 35),
    "Cap_OUT": ((0.45, 0.62, 0.82), 35),
}


# ---------------------------------------------------------------- KiCad board ---------
def read_board(path):
    """Board outline size and the button / LED centres, in board-centre coordinates.

    KiCad Y points down; the board and this model use Y up (antenna at +Y).
    """
    text = open(path, encoding="utf-8").read()
    ox, oy = map(float, re.search(r"\(aux_axis_origin ([-\d.]+) ([-\d.]+)\)", text).groups())

    xs, ys = [], []
    for block in re.findall(r"\(gr_(?:line|arc|rect)\b.*?\(layer \"Edge\.Cuts\"\)", text, re.S):
        for x, y in re.findall(r"\((?:start|end|mid) ([-\d.]+) ([-\d.]+)\)", block):
            xs.append(float(x))
            ys.append(float(y))
    pcb_l, pcb_w = max(xs) - min(xs), max(ys) - min(ys)
    cx, cy = (max(xs) + min(xs)) / 2, (max(ys) + min(ys)) / 2
    if abs(cx - ox) > 0.01 or abs(cy - oy) > 0.01:
        raise SystemExit(f"Board origin ({ox}, {oy}) is not the outline centre ({cx}, {cy}); "
                         "the tube is built about the board centre (DEC-33)")

    def centre(ref):
        for fp in text.split("\n\t(footprint ")[1:]:
            if re.search(r'\(property "Reference" "%s"' % re.escape(ref), fp):
                x, y = map(float, re.search(r"\n\t\t\(at ([-\d.]+) ([-\d.]+)", fp).groups())
                return round(x - ox, 3), round(oy - y, 3)
        raise SystemExit(f"{ref} not found in {path}")

    bx, by = centre(BUTTON_REF)
    lx, ly = centre(LED_REF)
    return {"PCB_L": round(pcb_l, 3), "PCB_W": round(pcb_w, 3),
            "BTN_X": bx, "BTN_Y": by, "LED_X": lx, "LED_Y": ly}


def export_board_step(path):
    out = os.path.join(tempfile.mkdtemp(prefix="lsc_step_"), "pcb.step")
    subprocess.run([KICAD_CLI, "pcb", "export", "step", "--drill-origin", "--subst-models",
                    "--no-dnp", "--force", "-o", out, path],
                   check=True, capture_output=True)
    return out


# ---------------------------------------------------------------- document helpers ----
def make_params(doc, board):
    sheet = doc.addObject("Spreadsheet::Sheet", "Params")
    sheet.set("A1", "Parameter")
    sheet.set("B1", "Value (mm)")
    sheet.set("C1", "Note")
    sheet.setStyle("A1:C1", "bold")
    row = 2
    for alias, value, note in PARAMS + [("Derived (formulas - do not edit)", None, None)] + \
            [(a, "=" + f, n) for a, f, n in DERIVED]:
        if value is None:
            row += 1
            sheet.set(f"A{row}", alias)
            sheet.setStyle(f"A{row}", "bold")
        else:
            if value == "@board":
                value = board[alias]
                note = f"{note} - from KiCad"
            sheet.set(f"A{row}", alias)
            sheet.set(f"B{row}", str(value))
            sheet.setAlias(f"B{row}", alias)
            sheet.set(f"C{row}", note)
        row += 1
    sheet.setColumnWidth("A", 150)
    sheet.setColumnWidth("C", 420)
    return sheet


def feature(body, kind, name, dims, at, rot=None):
    """Add a PartDesign primitive. dims and at hold expressions on Params (strings)."""
    f = body.newObject(f"PartDesign::{kind}", name)
    for prop, expr in dims.items():
        f.setExpression(prop, expr)
    if rot is not None:
        f.Placement = App.Placement(Vector(), rot)
    for axis, expr in zip("xyz", at):
        f.setExpression(f"Placement.Base.{axis}", expr)
    f.Refine = True
    return f


def box(body, sub, name, length, width, height, x, y, z):
    kind = "SubtractiveBox" if sub else "AdditiveBox"
    return feature(body, kind, name, {"Length": length, "Width": width, "Height": height},
                   (x, y, z))


def cyl(body, sub, name, radius, height, x, y, z, rot=None):
    kind = "SubtractiveCylinder" if sub else "AdditiveCylinder"
    return feature(body, kind, name, {"Radius": radius, "Height": height}, (x, y, z), rot)


# ---------------------------------------------------------------- parts ---------------
def make_tube(doc):
    b = doc.addObject("PartDesign::Body", "Tube")
    P = "Params."
    box(b, False, "Shell", P + "TUBE_L", P + "TUBE_W", P + "TUBE_H",
        f"-{P}TUBE_L / 2", f"-{P}TUBE_W / 2", "0")
    box(b, True, "Bore", f"{P}TUBE_L + 2", P + "IN_W", P + "IN_H",
        f"-{P}TUBE_L / 2 - 1", f"-{P}IN_W / 2", P + "WALL")
    # Full-length board groove in both side walls (DEC-29)
    box(b, True, "BoardGroove", f"{P}TUBE_L + 2", f"{P}IN_W + 2 * {P}GROOVE_D", P + "GROOVE_H",
        f"-{P}TUBE_L / 2 - 1", f"-({P}IN_W / 2 + {P}GROOVE_D)", P + "GROOVE_Z")
    # Snap-detent grooves on the +/-Y faces, both ends: the cap ridges click into these
    for xs, xn in (("-", "IN"), ("", "OUT")):
        for ys, yn in (("-", "Neg"), ("", "Pos")):
            cyl(b, True, f"Detent_{xn}_{yn}Y", f"{P}DET_R + {P}DET_CLR", P + "DET_LEN",
                f"{xs}({P}TUBE_L / 2 - {P}DET_X)", f"{ys}({P}TUBE_W / 2 + {P}DET_OFF)",
                P + "DET_Z")
    cyl(b, True, "Pinhole", f"{P}PINHOLE_D / 2", f"{P}WALL + 0.2",
        P + "BTN_X", P + "BTN_Y", f"{P}TUBE_H - {P}WALL - 0.1")
    cyl(b, True, "LedWindow", f"{P}LED_HOLE_D / 2", f"{P}WALL + 0.2",
        P + "LED_X", P + "LED_Y", f"{P}TUBE_H - {P}WALL - 0.1")
    return b


def make_cap(doc):
    """IN-end cap, built in place at the -X end of the tube."""
    b = doc.addObject("PartDesign::Body", "Cap")
    P = "Params."
    box(b, False, "Shell", f"{P}CHAMBER_L + {P}PLATE_T + {P}SLEEVE_L",
        f"{P}TUBE_W + 2 * {P}CAP_O", f"{P}TUBE_H + 2 * {P}CAP_O",
        P + "X_OUT", f"-({P}TUBE_W / 2 + {P}CAP_O)", f"-{P}CAP_O")
    box(b, True, "Socket", f"{P}CHAMBER_L + {P}SLEEVE_L + 0.1",
        f"{P}TUBE_W + 2 * {P}CAP_CLR", f"{P}TUBE_H + 2 * {P}CAP_CLR",
        f"{P}X_END - {P}CHAMBER_L", f"-({P}TUBE_W / 2 + {P}CAP_CLR)", f"-{P}CAP_CLR")
    # Stop ribs at the tube end plane: the tube butts against them and they block the board
    # corners, leaving the centre open for the wires.
    rib_w = f"{P}CAP_CLR + {P}WALL_SIDE + {P}STOP_IN"
    rib_h = f"{P}TUBE_H + 2 * {P}CAP_CLR"
    box(b, False, "StopRib_NegY", P + "STOP_L", rib_w, rib_h,
        f"{P}X_END - {P}STOP_L", f"-({P}TUBE_W / 2 + {P}CAP_CLR)", f"-{P}CAP_CLR")
    box(b, False, "StopRib_PosY", P + "STOP_L", rib_w, rib_h,
        f"{P}X_END - {P}STOP_L", f"{P}TUBE_W / 2 - {P}WALL_SIDE - {P}STOP_IN", f"-{P}CAP_CLR")
    cyl(b, True, "CableExit", f"{P}CABLE_D / 2", f"{P}PLATE_T + 0.2",
        f"{P}X_OUT - 0.1", "0", P + "WIRE_Z", Rotation(Vector(0, 1, 0), 90))
    for ys, yn in (("-", "Neg"), ("", "Pos")):
        cyl(b, False, f"SnapRidge_{yn}Y", P + "DET_R", P + "DET_LEN",
            f"{P}X_END + {P}DET_X", f"{ys}({P}TUBE_W / 2 + {P}DET_OFF)", P + "DET_Z")

    # Screw tab, centred under the cable exit and flush with the mounting face. Centring
    # makes the cap symmetric in Y, so the IN and OUT caps are the same printed part.
    box(b, False, "Tab", f"{P}EAR_HOLE_X + 0.5", P + "EAR_W", P + "EAR_T",
        P + "EAR_HX", f"-{P}EAR_W / 2", f"-{P}CAP_O")
    cyl(b, False, "TabEnd", f"{P}EAR_W / 2", P + "EAR_T", P + "EAR_HX", "0", f"-{P}CAP_O")
    # Gussets: right-angle wedges, legs along -X (on the tab) and +Z (on the end plate);
    # wedge local X -> -X, local Y -> +Z, local Z (thickness) -> +Y.
    m = App.Matrix(-1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0)
    for yn, y in (("NegY", f"-{P}EAR_W / 2"), ("PosY", f"{P}EAR_W / 2 - {P}GUSSET_W")):
        feature(b, "AdditiveWedge", f"Gusset_{yn}",
                {"Xmin": "0", "Xmax": P + "GUSSET", "Ymin": "0", "Ymax": P + "GUSSET",
                 "Zmin": "0", "Zmax": P + "GUSSET_W", "X2min": "0", "X2max": "0",
                 "Z2min": "0", "Z2max": P + "GUSSET_W"},
                (P + "X_OUT", y, P + "EAR_Z1"), Rotation(m))
    cyl(b, True, "ScrewHole", f"{P}EAR_HOLE_D / 2", f"{P}EAR_T + 0.2",
        P + "EAR_HX", "0", f"-{P}CAP_O - 0.1")
    feature(b, "SubtractiveCone", "Countersink",
            {"Radius1": f"{P}EAR_HOLE_D / 2", "Radius2": f"{P}EAR_CSK_D / 2",
             "Height": f"{P}CSK_H + 0.01"},
            (P + "EAR_HX", "0", f"{P}EAR_Z1 - {P}CSK_H"))
    return b


def make_cap_out(doc, cap):
    link = doc.addObject("App::Link", "Cap_OUT")
    link.LinkedObject = cap
    link.Placement = App.Placement(Vector(), Rotation(Vector(0, 0, 1), 180))
    return link


def import_pcb(doc, step):
    before = set(doc.Objects)
    if App.GuiUp:
        import ImportGui
        ImportGui.insert(step, doc.Name)
    else:
        import Import
        Import.insert(step, doc.Name)
    new = [o for o in doc.Objects if o not in before]
    top = [o for o in new if o.TypeId == "App::Part" and not any(p in new for p in o.InList)]
    if len(top) != 1:
        raise SystemExit(f"Expected one top-level part in the board STEP, got {len(top)}")
    pcb = top[0]
    pcb.Label = "PCB"
    pcb.setExpression("Placement.Base.z", "Params.BOARD_Z")
    return pcb


# ---------------------------------------------------------------- checks / export ----
def leaves(obj, prefix=""):
    """(label, subname) for every shape-carrying leaf under an App::Part tree."""
    for child in getattr(obj, "Group", []):
        sub = f"{prefix}{child.Name}."
        if child.TypeId == "App::Part":
            yield from leaves(child, sub)
        elif child.isDerivedFrom("Part::Feature") and not child.Shape.isNull():
            yield child.Label, sub


def fit_check(doc, tube, caps, pcb, tube_l):
    """The board must slide the whole tube length without touching it, and sit clear of the
    caps. Each part's Y/Z bounding box is swept along X (conservative)."""
    tube_s = Part.getShape(tube)
    problems = []
    for label, sub in leaves(pcb):
        s = Part.getShape(pcb, sub, needSubElement=True)
        if not s.Solids:
            continue
        bb = s.BoundBox
        sweep = Part.makeBox(tube_l + 2, bb.YLength, bb.ZLength,
                             Vector(-tube_l / 2 - 1, bb.YMin, bb.ZMin))
        v = tube_s.common(sweep).Volume
        if v > 1e-3:
            problems.append(f"{label}: insertion path hits the tube ({v:.3f} mm3)")
        for cap in caps:
            v = Part.getShape(cap).common(s).Volume
            if v > 1e-3:
                problems.append(f"{label}: hits {cap.Name} ({v:.3f} mm3)")
    return problems


def write_stl(shape, path):
    mesh = MeshPart.meshFromShape(Shape=shape, LinearDeflection=0.01, AngularDeflection=0.1)
    mesh.write(path)


def style_in_gui(doc):
    import FreeCADGui as Gui

    for name, (color, transparency) in STYLES.items():
        vo = doc.getObject(name).ViewObject
        vo.ShapeColor = color
        vo.Transparency = transparency
    view = Gui.getDocument(doc.Name).activeView()
    view.viewIsometric()
    view.fitAll()


def main():
    board = read_board(BOARD_FILE)
    step = export_board_step(BOARD_FILE)

    if DOC_NAME in App.listDocuments():
        App.closeDocument(DOC_NAME)
    doc = App.newDocument(DOC_NAME)
    params = make_params(doc, board)
    doc.recompute()
    tube = make_tube(doc)
    cap = make_cap(doc)
    cap_out = make_cap_out(doc, cap)
    pcb = import_pcb(doc, step)
    doc.recompute()

    bad = [o.Name for o in doc.Objects if "Invalid" in o.State or "Error" in o.State]
    if bad:
        raise SystemExit(f"Recompute failed: {bad}")

    g = {alias: float(params.get(alias)) for alias, *_ in DERIVED}
    if App.GuiUp:
        style_in_gui(doc)
    doc.saveAs(os.path.join(HERE, DOC_NAME + ".FCStd"))

    tube_s, cap_s = tube.Shape.copy(), cap.Shape.copy()
    cap_s.exportStep(os.path.join(HERE, f"{DOC_NAME}_Cap.step"))
    tube_s.exportStep(os.path.join(HERE, f"{DOC_NAME}_Tube.step"))
    # STLs in print orientation. Tube standing on its end (DEC-29: no overhangs, slot height
    # from XY accuracy). Cap as modelled, mounting face down; print it twice.
    standing = tube_s.copy()
    standing.rotate(Vector(), Vector(0, 1, 0), -90)
    standing.translate(Vector(0, 0, -standing.BoundBox.ZMin))
    write_stl(standing, os.path.join(HERE, f"{DOC_NAME}_Tube.stl"))
    write_stl(cap_s, os.path.join(HERE, f"{DOC_NAME}_Cap.stl"))

    print(f"Board  : {board['PCB_L']} x {board['PCB_W']} mm; button {BUTTON_REF} "
          f"({board['BTN_X']}, {board['BTN_Y']}), LED {LED_REF} ({board['LED_X']}, "
          f"{board['LED_Y']})")
    print(f"Tube   : {g['TUBE_L']:.1f} x {g['TUBE_W']:.1f} x {g['TUBE_H']:.1f} mm "
          f"(interior {g['IN_W']:.1f} wide, groove {g['GROOVE_H']:.1f} x {g['GROOVE_D']:.1f})")
    print(f"Overall: {g['OVERALL_L']:.1f} (incl. tabs) x {g['OVERALL_W']:.1f} x "
          f"{g['OVERALL_H']:.1f} mm")
    for name, s in (("Tube", tube_s), ("Cap", cap_s)):
        print(f"{name:7s}: valid={s.isValid()} solids={len(s.Solids)} "
              f"volume={s.Volume / 1000:.2f} cm3")
    problems = fit_check(doc, tube, (cap, cap_out), pcb, g["TUBE_L"])
    print("Fit    : " + ("OK - board slides the full tube and clears both caps" if not problems
                         else "\n         ".join(problems)))


main()
