"""FEHA-LSC-001-01 board setup (design spec §0 layout step 2; DEC-29..DEC-33).

Run on the placed board, with KiCad closed:
    /Applications/KiCad/KiCad.app/Contents/Frameworks/Python.framework/Versions/Current/bin/python3 \
        scripts/setup_board.py SRC.kicad_pcb DST.kicad_pcb [W]
W is the board width in mm (default 30, DEC-35). Re-runnable: it replaces the outline, the
rule areas and its own GND pours (by zone name); footprints, tracks and other pours stay.
Saving also rewrites .kicad_pro/.kicad_prl beside DST: use a scratch DST and copy only the
.kicad_pcb into the project. Board rules and netclasses live in .kicad_pro and are set there.

- 4 copper layers, KiCad default 1.6 mm stack-up (DEC-30, DEC-34)
- outline redrawn: 50 x W with the 15.2 x 6.6 antenna notch, 0.5 mm radii in the notch's inner
  corners (1.0 mm router bit)
- aux (drill/place) and grid origin at the board centre (DEC-33)
- rule areas: 1.0 mm top-side part keep-out along both long edges (CON-3); antenna keep-out
  over the notch on all 4 layers (DEC-31)
- GND pour on L1..L4 over the whole board (DEC-30)

Board frame: X along the board, Y across (+Y = antenna edge), drawn at
KiCad (100, 100): kx = 100 + X, ky = 100 + W - Y.
"""
import math
import sys
import pcbnew

SRC, DST = sys.argv[1], sys.argv[2]
L, W = 50.0, float(sys.argv[3]) if len(sys.argv) > 3 else 30.0
NCX, NOTCH_W, NOTCH_D, R = 25.0, 15.2, 6.6, 0.5
EDGE_KO = 1.0
mm = pcbnew.FromMM


def pt(x, y):
    return pcbnew.VECTOR2I(mm(100 + x), mm(100 + W - y))


board = pcbnew.LoadBoard(SRC)
ds = board.GetDesignSettings()

# stack-up: layer count here; the stack-up block itself is not exposed to Python and is
# rewritten in the saved file below
board.SetCopperLayerCount(4)

# outline: drop the old Edge.Cuts and User.1 picture shapes, redraw with notch fillets
for d in list(board.GetDrawings()):
    if d.GetLayer() in (pcbnew.Edge_Cuts, pcbnew.User_1):
        board.Remove(d)
# remove only this script's own zones (by name); hand-drawn pours stay
OWN = {"CON-3 edge -Y", "CON-3 edge +Y", "Antenna keep-out",
       "GND F.Cu", "GND In1.Cu", "GND In2.Cu", "GND B.Cu"}
for z in list(board.Zones()):
    if z.GetZoneName() in OWN:
        board.Remove(z)

a, b, yb = NCX - NOTCH_W / 2, NCX + NOTCH_W / 2, W - NOTCH_D
k = R * (1 - math.sqrt(0.5))
# ("L", start, end) segments and ("A", start, mid, end) arcs, in the board frame
outline = [
    ("L", (0, 0), (L, 0)), ("L", (L, 0), (L, W)), ("L", (L, W), (b, W)),
    ("L", (b, W), (b, yb + R)),
    ("A", (b, yb + R), (b - k, yb + k), (b - R, yb)),
    ("L", (b - R, yb), (a + R, yb)),
    ("A", (a + R, yb), (a + k, yb + k), (a, yb + R)),
    ("L", (a, yb + R), (a, W)), ("L", (a, W), (0, W)), ("L", (0, W), (0, 0)),
]
for seg in outline:
    s = pcbnew.PCB_SHAPE(board)
    s.SetLayer(pcbnew.Edge_Cuts)
    s.SetWidth(mm(0.05))
    if seg[0] == "L":
        s.SetShape(pcbnew.SHAPE_T_SEGMENT)
        s.SetStart(pt(*seg[1]))
        s.SetEnd(pt(*seg[2]))
    else:
        s.SetShape(pcbnew.SHAPE_T_ARC)
        s.SetArcGeometry(pt(*seg[1]), pt(*seg[2]), pt(*seg[3]))
    board.Add(s)

# origins at the board centre
ds.SetAuxOrigin(pt(L / 2, W / 2))
ds.SetGridOrigin(pt(L / 2, W / 2))


def zone(name, layers, corners, rule=False):
    z = pcbnew.ZONE(board)
    ls = pcbnew.LSET()
    for layer in layers:
        ls.AddLayer(layer)
    z.SetLayerSet(ls)
    z.SetZoneName(name)
    for x, y in corners:
        z.AppendCorner(pt(x, y), -1)
    z.SetIsRuleArea(rule)
    board.Add(z)
    return z


CU = (pcbnew.F_Cu, pcbnew.In1_Cu, pcbnew.In2_Cu, pcbnew.B_Cu)

for name, y0 in (("CON-3 edge -Y", 0.0), ("CON-3 edge +Y", W - EDGE_KO)):
    z = zone(name, (pcbnew.F_Cu,), [(0, y0), (L, y0), (L, y0 + EDGE_KO), (0, y0 + EDGE_KO)], True)
    z.SetDoNotAllowFootprints(True)
    z.SetDoNotAllowTracks(False)
    z.SetDoNotAllowVias(False)
    z.SetDoNotAllowPads(False)
    z.SetDoNotAllowZoneFills(False)

z = zone("Antenna keep-out", CU, [(a, yb), (b, yb), (b, W), (a, W)], True)
z.SetDoNotAllowFootprints(False)
z.SetDoNotAllowTracks(True)
z.SetDoNotAllowVias(True)
z.SetDoNotAllowPads(True)
z.SetDoNotAllowZoneFills(True)

gnd = board.FindNet("GND")
for layer in CU:
    z = zone("GND " + board.GetLayerName(layer), (layer,), [(0, 0), (L, 0), (L, W), (0, W)])
    z.SetNet(gnd)
    z.SetLocalClearance(mm(0.2))
    z.SetMinThickness(mm(0.2))

filler = pcbnew.ZONE_FILLER(board)
filler.Fill(board.Zones())
board.Save(DST)

# 4-layer stack-up, 1.6 mm nominal (no impedance-controlled nets, §7; JLCPCB builds its own
# standard 1.6 mm 4-layer stack-up)
def cu(name):
    return f'\t\t\t(layer "{name}"\n\t\t\t\t(type "copper")\n\t\t\t\t(thickness 0.035)\n\t\t\t)\n'


def diel(n, kind, t):
    return (f'\t\t\t(layer "dielectric {n}"\n\t\t\t\t(type "{kind}")\n\t\t\t\t(thickness {t})\n'
            f'\t\t\t\t(material "FR4")\n\t\t\t\t(epsilon_r 4.5)\n\t\t\t\t(loss_tangent 0.02)\n\t\t\t)\n')


text = open(DST).read()
start = text.index('\t\t\t(layer "F.Cu"\n\t\t\t\t(type "copper")')
end = text.index('\t\t\t(layer "B.Mask"', start)
stack = (cu("F.Cu") + diel(1, "prepreg", 0.1) + cu("In1.Cu") + diel(2, "core", 1.24)
         + cu("In2.Cu") + diel(3, "prepreg", 0.1) + cu("B.Cu"))
open(DST, "w").write(text[:start] + stack + text[end:])
print("layers:", board.GetCopperLayerCount(), "zones:", len(board.Zones()))
