"""
ESP32-C3 LED Controller Rev B - inline tube enclosure (parametric).

Builds the slide-in tube, the two end caps and placeholder bodies for the PCB and its
tallest parts, then saves an .FCStd and exports one STL per printed part.

Run it inside FreeCAD (Macro > Macros... > point "User macros location" at this folder >
select enclosure_rev_b.py > Execute) to get coloured, see-through parts and a fitted view
saved into the .FCStd. Or run headless (no colours, no saved view):
    /Applications/FreeCAD.app/Contents/Resources/bin/freecadcmd enclosure_rev_b.py
Every dimension is a parameter in the block below; change it there and re-run. Units: mm.

Coordinate system: X along the tube (IN end at X = 0), Y across, Z up from the tube's
bottom face. "Top" (+Z) is the board's component side; when the unit is screwed under a
cabinet the top faces down into the room, so the pin-hole and LED window are visible.
The ESP32 antenna is on the +Y side.

Concept and rationale: enclosure-concept.html (same folder).
"""

import os

import FreeCAD as App
import Part
from FreeCAD import Vector

# ---------------------------------------------------------------- parameters ----------
# PCB (placeholder until the KiCad outline exists - update from the board, then re-run)
PCB_L = 66.0            # board length along X
PCB_W = 20.0            # board width along Y
PCB_T = 1.6             # board thickness

# Tube
WALL = 1.8              # tube wall thickness
SIDE_CLR = 0.3          # clearance board edge <-> tube wall, each side
END_PLAY = 0.2          # axial play each end (board shorter than tube by 2 x this)
UNDER_GAP = 1.5         # air gap under the board = height of the support rails
OVER_GAP = 5.5          # headroom above the board top (tallest part: 4.5 mm connector)
RAIL_W = 1.2            # support-rail width, measured from the wall
LIFT_CLR = 0.2          # vertical clearance board top <-> corner bosses

# Hold-down ribs (stop the board lifting off the rails at both ends)
HOLD_L = 10.0           # rib length along X from each tube end
HOLD_W = 1.0            # rib width inward from the wall (overlaps the board edge by 0.7)

# Snap detents (no screws): a half-round ridge inside each cap sleeve clicks into a
# matching groove on the tube's +/-Y faces
DET_X = 4.0             # detent position from the tube end (inside the sleeve)
DET_R = 1.0             # ridge radius
DET_ENGAGE = 0.4        # how far the ridge reaches into the tube wall
DET_LEN = 6.0           # ridge length along Z, centred on the tube height
DET_CLR = 0.05          # groove radius clearance over the ridge

# End caps
CAP_WALL = 1.6          # cap wall thickness
CAP_CLR = 0.2           # sleeve clearance around the tube
SLEEVE_L = 8.0          # length the cap slides over the tube
CHAMBER_L = 12.0        # cable chamber (jacket strip, fan-out, tie-stop)
PLATE_T = 2.5           # end plate thickness
CABLE_D = 6.5           # cable exit hole (round twin cable up to ~6 mm)
STOP_L = 1.5            # board/tube stop ribs at the tube end plane
STOP_W = 4.0            # stop-rib width from the cap wall (blocks the board corners)

# Screw ears (wood screws into the cabinet): one round-ended tab per cap, centred under
# the cable exit and flush with the mounting face. Centring makes the cap symmetric in Y,
# so the IN and OUT caps are the same printed part.
EAR_W = 12.0            # tab width (Y); the tab end is a full radius of EAR_W / 2
EAR_HOLE_X = 6.0        # screw-hole centre beyond the cap's end face
EAR_T = 3.0             # tab thickness
EAR_HOLE_D = 3.6        # clearance for a 3.5 mm wood screw
EAR_CSK_D = 7.2         # countersink top diameter
GUSSET = 3.0            # triangular gussets tab <-> end plate (each side of the cable)
GUSSET_W = 2.0          # gusset thickness along Y, at the outer edges of the tab

# Board features that the tube wall must line up with (board coordinates, origin at the
# board's IN-end, -Y corner, X along the board)
BUTTON_XY = (51.0, 4.0)     # GPIO9 button actuator centre
PINHOLE_D = 1.6             # paper-clip hole over the button
LED_XY = (51.0, 16.0)       # status LED centre
LED_HOLE_D = 3.2            # window for the LED (fill with clear resin or a light pipe)

# Placeholder parts (for fit checks only - not printed)
CONN_L, CONN_W, CONN_H = 11.5, 7.9, 4.5     # HDGC4001SMD-S-2P (datasheet, 2P)
CONN_WIRE_Z = 2.25                          # wire-entry centre above board top (est.)
MODULE_X = 34.0                             # ESP32-C3-MINI-1 X position on the board
MODULE_L, MODULE_W, MODULE_H = 13.2, 16.6, 2.4

try:
    OUT_DIR = os.path.dirname(os.path.abspath(__file__))
except NameError:       # some FreeCAD macro runners do not set __file__
    OUT_DIR = os.path.expanduser("~")
DOC_NAME = "Enclosure_Rev_B"

# Display styles used when the script runs inside the FreeCAD GUI: (RGB, transparency %)
STYLES = {
    "Tube": ((0.86, 0.82, 0.72), 55),
    "Cap_IN": ((0.45, 0.62, 0.82), 35),
    "Cap_OUT": ((0.45, 0.62, 0.82), 35),
    "PCB": ((0.18, 0.55, 0.30), 0),
    "Conn_IN": ((0.80, 0.80, 0.80), 0),
    "Conn_OUT": ((0.80, 0.80, 0.80), 0),
    "ESP32_module": ((0.35, 0.37, 0.40), 0),
    "Button": ((0.15, 0.15, 0.15), 0),
}

# ---------------------------------------------------------------- derived -------------
IN_W = PCB_W + 2 * SIDE_CLR                     # tube interior width
IN_H = UNDER_GAP + PCB_T + OVER_GAP             # tube interior height
TUBE_L = PCB_L + 2 * END_PLAY
TUBE_W = IN_W + 2 * WALL
TUBE_H = IN_H + 2 * WALL

BOARD_Z = WALL + UNDER_GAP                      # board bottom face
BOARD_TOP = BOARD_Z + PCB_T
HOLD_Z0 = BOARD_TOP + LIFT_CLR                  # hold-down rib underside
CEIL_Z = WALL + IN_H                            # tube ceiling
WIRE_Z = BOARD_TOP + CONN_WIRE_Z


def board_to_tube(x, y):
    """Board coordinates -> tube coordinates."""
    return END_PLAY + x, WALL + SIDE_CLR + y


def box(x0, y0, z0, x1, y1, z1):
    return Part.makeBox(x1 - x0, y1 - y0, z1 - z0, Vector(x0, y0, z0))


def mirror_x(shape):
    """Mirror about the tube's mid-plane, so IN-end features become OUT-end features."""
    return shape.mirror(Vector(TUBE_L / 2, 0, 0), Vector(1, 0, 0))


def detent(x, grow):
    """Half-round detent along Z at X = x, on both +/-Y tube faces.

    grow = 0 gives the cap's ridge; grow = DET_CLR gives the tube's groove.
    The axis sits DET_R - DET_ENGAGE outside each tube face, so the ridge reaches
    DET_ENGAGE into the tube wall.
    """
    z0 = (TUBE_H - DET_LEN) / 2
    off = DET_R - DET_ENGAGE
    r = DET_R + grow
    lo = Part.makeCylinder(r, DET_LEN, Vector(x, -off, z0))
    hi = Part.makeCylinder(r, DET_LEN, Vector(x, TUBE_W + off, z0))
    return lo.fuse(hi)


def make_tube():
    tube = box(0, 0, 0, TUBE_L, TUBE_W, TUBE_H)
    tube = tube.cut(box(-1, WALL, WALL, TUBE_L + 1, WALL + IN_W, WALL + IN_H))

    rails = box(0, WALL, WALL, TUBE_L, WALL + RAIL_W, BOARD_Z)
    rails = rails.fuse(box(0, WALL + IN_W - RAIL_W, WALL, TUBE_L, WALL + IN_W, BOARD_Z))
    tube = tube.fuse(rails)

    holds = box(0, WALL, HOLD_Z0, HOLD_L, WALL + HOLD_W, CEIL_Z)
    holds = holds.fuse(box(0, WALL + IN_W - HOLD_W, HOLD_Z0, HOLD_L, WALL + IN_W, CEIL_Z))
    holds = holds.fuse(mirror_x(holds))
    tube = tube.fuse(holds)

    grooves = detent(DET_X, DET_CLR)
    tube = tube.cut(grooves).cut(mirror_x(grooves))

    bx, by = board_to_tube(*BUTTON_XY)
    lx, ly = board_to_tube(*LED_XY)
    z_hole = WALL + IN_H - 0.1
    tube = tube.cut(Part.makeCylinder(PINHOLE_D / 2, WALL + 0.2, Vector(bx, by, z_hole)))
    tube = tube.cut(Part.makeCylinder(LED_HOLE_D / 2, WALL + 0.2, Vector(lx, ly, z_hole)))
    return tube.removeSplitter()


def make_cap_in():
    """IN-end cap. The OUT cap is its mirror image."""
    x_out = -(CHAMBER_L + PLATE_T)
    o = CAP_CLR + CAP_WALL
    cap = box(x_out, -o, -o, SLEEVE_L, TUBE_W + o, TUBE_H + o)
    cap = cap.cut(box(-CHAMBER_L, -CAP_CLR, -CAP_CLR,
                      SLEEVE_L + 0.1, TUBE_W + CAP_CLR, TUBE_H + CAP_CLR))

    # Stop ribs at the tube end plane: the tube butts against them and they block the
    # board's corners, leaving the centre open for the wires.
    stop = box(-STOP_L, -CAP_CLR, -CAP_CLR, 0, WALL + STOP_W, TUBE_H + CAP_CLR)
    stop = stop.fuse(box(-STOP_L, WALL + IN_W - STOP_W, -CAP_CLR,
                         0, TUBE_W + CAP_CLR, TUBE_H + CAP_CLR))
    cap = cap.fuse(stop)

    cable = Part.makeCylinder(CABLE_D / 2, PLATE_T + 0.2,
                              Vector(x_out - 0.1, TUBE_W / 2, WIRE_Z), Vector(1, 0, 0))
    cap = cap.cut(cable)

    # Snap ridges: keep only the part of each half-round that lies in the sleeve gap and
    # the tube wall zone, i.e. outside the tube's outer box shrunk by DET_ENGAGE.
    ridges = detent(DET_X, 0)
    inner = box(-1, DET_ENGAGE, -1, SLEEVE_L + 1, TUBE_W - DET_ENGAGE, TUBE_H + 1)
    cap = cap.fuse(ridges.cut(inner))

    cy = TUBE_W / 2
    hx = x_out - EAR_HOLE_X
    z0, z1 = -o, -o + EAR_T
    ear = box(hx, cy - EAR_W / 2, z0, x_out + 0.01, cy + EAR_W / 2, z1)
    ear = ear.fuse(Part.makeCylinder(EAR_W / 2, EAR_T, Vector(hx, cy, z0)))
    for y0 in (cy - EAR_W / 2, cy + EAR_W / 2 - GUSSET_W):
        tri = Part.makePolygon([Vector(x_out + 0.01, y0, z1), Vector(x_out - GUSSET, y0, z1),
                                Vector(x_out + 0.01, y0, z1 + GUSSET),
                                Vector(x_out + 0.01, y0, z1)])
        ear = ear.fuse(Part.Face(tri).extrude(Vector(0, GUSSET_W, 0)))
    hole = Part.makeCylinder(EAR_HOLE_D / 2, EAR_T + 0.2, Vector(hx, cy, z0 - 0.1))
    csk_h = (EAR_CSK_D - EAR_HOLE_D) / 2
    csk = Part.makeCone(EAR_HOLE_D / 2, EAR_CSK_D / 2, csk_h + 0.01,
                        Vector(hx, cy, z1 - csk_h))
    cap = cap.fuse(ear.cut(hole).cut(csk))
    return cap.removeSplitter()


def make_placeholders():
    parts = {}
    x0, y0 = board_to_tube(0, 0)
    parts["PCB"] = box(x0, y0, BOARD_Z, x0 + PCB_L, y0 + PCB_W, BOARD_TOP)

    cy = y0 + PCB_W / 2 - CONN_W / 2
    parts["Conn_IN"] = box(x0, cy, BOARD_TOP, x0 + CONN_L, cy + CONN_W, BOARD_TOP + CONN_H)
    parts["Conn_OUT"] = box(x0 + PCB_L - CONN_L, cy, BOARD_TOP,
                            x0 + PCB_L, cy + CONN_W, BOARD_TOP + CONN_H)

    mx, my = board_to_tube(MODULE_X, PCB_W - MODULE_W)
    parts["ESP32_module"] = box(mx, my, BOARD_TOP,
                                mx + MODULE_L, my + MODULE_W, BOARD_TOP + MODULE_H)


    bx, by = board_to_tube(*BUTTON_XY)
    parts["Button"] = box(bx - 1.5, by - 2.0, BOARD_TOP, bx + 1.5, by + 2.0, BOARD_TOP + 1.6)
    return parts


def check_fit(tube, placeholders):
    """Report any interference between the printed tube and the placeholder parts."""
    problems = []
    for name, shape in placeholders.items():
        vol = tube.common(shape).Volume
        if vol > 1e-3:
            problems.append(f"{name} intersects tube ({vol:.2f} mm3)")
    return problems


def style_in_gui(doc):
    """Colour the parts, make tube and caps see-through and fit the view (GUI only)."""
    import FreeCADGui as Gui

    for obj in doc.Objects:
        color, transparency = STYLES.get(obj.Name, ((0.6, 0.6, 0.6), 0))
        obj.ViewObject.ShapeColor = color
        obj.ViewObject.Transparency = transparency
    view = Gui.getDocument(doc.Name).activeView()
    view.viewIsometric()
    view.fitAll()


def main():
    if DOC_NAME in App.listDocuments():
        App.closeDocument(DOC_NAME)
    doc = App.newDocument(DOC_NAME)

    tube = make_tube()
    cap_in = make_cap_in()
    cap_out = mirror_x(cap_in)
    placeholders = make_placeholders()

    printed = {"Tube": tube, "Cap_IN": cap_in, "Cap_OUT": cap_out}
    for name, shape in {**printed, **placeholders}.items():
        obj = doc.addObject("Part::Feature", name)
        obj.Shape = shape

    doc.recompute()
    if App.GuiUp:
        style_in_gui(doc)
    doc.saveAs(os.path.join(OUT_DIR, DOC_NAME + ".FCStd"))
    # The caps are one part: print Cap.stl twice. The OUT cap is the IN cap turned 180 deg.
    tube.exportStl(os.path.join(OUT_DIR, f"{DOC_NAME}_Tube.stl"))
    cap_in.exportStl(os.path.join(OUT_DIR, f"{DOC_NAME}_Cap.stl"))

    turned = cap_in.copy()
    turned.rotate(Vector(TUBE_L / 2, TUBE_W / 2, 0), Vector(0, 0, 1), 180)
    mismatch = turned.cut(cap_out).Volume + cap_out.cut(turned).Volume

    overall_l = TUBE_L + 2 * (CHAMBER_L + PLATE_T + EAR_HOLE_X + EAR_W / 2)
    overall_w = TUBE_W + 2 * (CAP_CLR + CAP_WALL)
    overall_h = TUBE_H + 2 * (CAP_CLR + CAP_WALL)
    print(f"Tube   : {TUBE_L:.1f} x {TUBE_W:.1f} x {TUBE_H:.1f} mm")
    print(f"Overall: {overall_l:.1f} (incl. ears) x {overall_w:.1f} x {overall_h:.1f} mm")
    print(f"Caps identical (OUT = IN turned 180 deg): mismatch {mismatch:.4f} mm3")
    for name, shape in printed.items():
        print(f"{name:8s} valid={shape.isValid()} solids={len(shape.Solids)} "
              f"volume={shape.Volume / 1000:.2f} cm3")
    problems = check_fit(tube, placeholders)
    print("Fit check: " + ("OK" if not problems else "; ".join(problems)))


main()
