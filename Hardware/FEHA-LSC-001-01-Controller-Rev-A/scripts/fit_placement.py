"""FEHA-LSC-001-01 placement and courtyard fit check (design spec DEC-29..DEC-33).

Places all 40 footprints at the agreed positions, draws the outline with the antenna notch and
the 1.0 mm edge keep-out strips (User.1), saves the result to DST, and reports part overlaps,
keep-out and notch hits. Writes DST with .json beside it (part boxes, for drawings).

Run with KiCad's bundled Python (pcbnew module), with KiCad closed:
    /Applications/KiCad/KiCad.app/Contents/Frameworks/Python.framework/Versions/Current/bin/python3 \
        scripts/fit_placement.py SRC.kicad_pcb DST.kicad_pcb 50 25
Use a scratch DST first; write the real board only after a clean run.

Board frame: X along the board (IN end X = 0), Y across (+Y = antenna edge), mm.
KiCad: kx = 100 + X, ky = 100 + W - Y (board drawn at 100, 100). The agreed board origin is the
board centre (DEC-33): set it in KiCad after placement.

Part extents in EXT are pads plus courtyard or body (connector bodies at their full 7.9 mm), as
measured from the project footprints on 2026-10-06. It checks placement only, not routing.
"""
import sys
import pcbnew

SRC, DST, L, W = sys.argv[1], sys.argv[2], float(sys.argv[3]), float(sys.argv[4])
GAP = 0.2       # minimum part-to-part gap counted as OK
EDGE_KO = 1.0   # CON-3 top-side edge keep-out (DEC-29)
NOTCH_W, NOTCH_D = 15.2, 6.6

# extents (xmin, xmax, ymin, ymax) in footprint-local KiCad coords (pads + courtyard/body)
EXT = {
    "C_0402": (-0.9, 0.9, -0.4, 0.4), "R_0402": (-0.9, 0.9, -0.4, 0.4),
    "C_0603": (-1.4, 1.4, -0.6, 0.6), "C_0805": (-1.7, 1.7, -1.0, 1.0),
    "C_1206": (-2.3, 2.3, -1.15, 1.15), "D_SMA": (-3.51, 3.5, -1.75, 1.75),
    "D_SMB": (-3.66, 3.65, -2.25, 2.25), "D_0805": (-1.69, 1.68, -0.96, 0.96),
    "D_SOD-123": (-2.36, 2.35, -1.15, 1.15), "IND": (-2.3, 2.3, -2.2, 2.2),
    "Q1": (-1.92, 1.92, -1.84, 1.70), "Q2": (-1.65, 1.65, -1.35, 1.35),
    "U1": (-1.9, 1.9, -1.25, 1.25), "S1": (-3.0, 3.0, -1.7, 1.7),
    "TP": (-0.95, 0.95, -0.95, 0.95), "CN3": (-1.77, 1.77, -1.77, 9.39),
    "CONN": (-3.95, 3.95, -6.38, 7.62), "U2": (-6.85, 6.85, -11.25, 5.85),
}


def kind(ref, fpname):
    for k in ("C_0402", "R_0402", "C_0603", "C_0805", "C_1206", "D_SMA", "D_SMB", "D_0805",
              "D_SOD-123"):
        if fpname.endswith(k):
            return k
    if ref.startswith("TP"):
        return "TP"
    if ref in ("CN1", "CN2"):
        return "CONN"
    return {"L1": "IND"}.get(ref, ref)


# (X, Y, rot) in the board frame; rot is KiCad orientation in degrees
CY = W / 2      # connector centre line (cable exits centred in the caps)
MY0 = W - 1.3 - 11.0   # module origin Y: body 1.3 mm in from the +Y edge
P = {
    # IN end: connector; TVS + bulk above it; reverse protection, status LED below it
    "CN1": (7.92, CY, 270), "D2": (4.3, W - 5.0, 0), "C3": (10.8, W - 3.5, 0),
    "C8": (10.8, W - 6.2, 0),
    "Q1": (2.6, 4.0, 0), "D5": (7.4, 5.4, 0), "R3": (7.4, 2.6, 0), "TP1": (12.4, 3.5, 0),
    "D3": (3.0, 7.3, 0), "R7": (12.4, 6.5, 0),
    # module centred; 3V3/EN/IO2 parts on its left
    "U2": (25.0, MY0, 0),
    "C4": (15.5, MY0 + 2.4, 90), "C6": (17.1, MY0 + 2.6, 90), "C2": (17.1, MY0 - 1.4, 90),
    "R8": (15.5, MY0 - 1.4, 90), "R4": (17.1, MY0 + 0.6, 90), "TP3": (15.9, MY0 - 4.2, 0),
    # strip under the module: buck in one row, then the button
    "L1": (17.3, 3.6, 0), "C5": (21.0, 3.6, 90), "U1": (23.9, 3.6, 90), "C9": (25.9, 3.6, 90),
    "R5": (27.2, 3.6, 90), "R6": (28.4, 3.6, 90), "C7": (23.9, 6.1, 0),
    "S1": (32.4, 3.4, 0),
    # right of the module: strap pull-ups, test points
    "R1": (33.0, 9.0, 90), "C1": (34.4, 9.0, 90), "R2": (33.0, 11.4, 90),
    "TP4": (33.4, 14.0, 0), "TP5": (34.0, 17.4, 0),
    # OUT end: switching corner below CN2; debug header + VBUS diode above it
    "CN2": (L - 7.92, CY, 90), "D1": (L - 8.4, 6.4, 0), "Q2": (L - 2.8, 6.4, 0),
    "C10": (L - 9.8, 2.4, 0), "R9": (L - 6.9, 2.5, 0), "R10": (L - 4.7, 2.5, 0),
    "TP6": (L - 2.4, 3.4, 0), "TP7": (L - 13.3, 3.0, 0),
    "CN3": (L - 2.1, W - 3.0, 270), "D4": (L - 8.0, W - 6.8, 0),
}


def bbox(ref, k, x, y, rot):
    x0, x1, y0, y1 = EXT[k]
    r = rot % 360
    # local (u, v) -> screen offset for KiCad rotation r (CCW on screen, y down)
    if r == 0:
        dx, dy = (x0, x1), (y0, y1)
    elif r == 90:
        dx, dy = (y0, y1), (-x1, -x0)
    elif r == 180:
        dx, dy = (-x1, -x0), (-y1, -y0)
    else:
        dx, dy = (-y1, -y0), (x0, x1)
    # screen dy is down; board Y is up
    return (x + dx[0], x + dx[1], y - dy[1], y - dy[0])


board = pcbnew.LoadBoard(SRC)
mm = pcbnew.FromMM
boxes = {}
for fp in board.GetFootprints():
    ref = fp.GetReference()
    if ref not in P:
        print("UNPLACED", ref)
        continue
    x, y, rot = P[ref]
    fp.SetOrientationDegrees(rot)
    fp.SetPosition(pcbnew.VECTOR2I(mm(100 + x), mm(100 + W - y)))
    boxes[ref] = bbox(ref, kind(ref, fp.GetFPIDAsString()), x, y, rot)

# outline with antenna notch centred on the module
ncx = P["U2"][0]
pts = [(0, 0), (L, 0), (L, W), (ncx + NOTCH_W / 2, W), (ncx + NOTCH_W / 2, W - NOTCH_D),
       (ncx - NOTCH_W / 2, W - NOTCH_D), (ncx - NOTCH_W / 2, W), (0, W)]
for (ax, ay), (bx, by) in zip(pts, pts[1:] + pts[:1]):
    seg = pcbnew.PCB_SHAPE(board)
    seg.SetShape(pcbnew.SHAPE_T_SEGMENT)
    seg.SetLayer(pcbnew.Edge_Cuts)
    seg.SetWidth(mm(0.05))
    seg.SetStart(pcbnew.VECTOR2I(mm(100 + ax), mm(100 + W - ay)))
    seg.SetEnd(pcbnew.VECTOR2I(mm(100 + bx), mm(100 + W - by)))
    board.Add(seg)
# edge keep-out strips drawn on User.1 for the picture
for y0 in (0, W - EDGE_KO):
    r = pcbnew.PCB_SHAPE(board)
    r.SetShape(pcbnew.SHAPE_T_RECT)
    r.SetLayer(pcbnew.User_1)
    r.SetWidth(mm(0.05))
    r.SetStart(pcbnew.VECTOR2I(mm(100), mm(100 + W - y0)))
    r.SetEnd(pcbnew.VECTOR2I(mm(100 + L), mm(100 + W - y0 - EDGE_KO)))
    board.Add(r)
board.Save(DST)

# checks
problems = []
refs = sorted(boxes)
for i, a in enumerate(refs):
    ax0, ax1, ay0, ay1 = boxes[a]
    if ax0 < 0 or ax1 > L or ay0 < 0 or ay1 > W:
        problems.append(f"{a} off board")
    if ay0 < EDGE_KO - 1e-6 or (ay1 > W - EDGE_KO + 1e-6 and a != "U2"):
        problems.append(f"{a} in edge keep-out")
    # parts (other than the module) inside the notch
    if a != "U2" and ay1 > W - NOTCH_D and ax1 > ncx - NOTCH_W / 2 and ax0 < ncx + NOTCH_W / 2:
        problems.append(f"{a} in antenna notch")
    for b in refs[i + 1:]:
        bx0, bx1, by0, by1 = boxes[b]
        ox = min(ax1, bx1) - max(ax0, bx0)
        oy = min(ay1, by1) - max(ay0, by0)
        if ox > -GAP + 1e-6 and oy > -GAP + 1e-6:
            problems.append(f"{a} x {b}: overlap {max(ox, 0):.2f} x {max(oy, 0):.2f}")
used = sum((b[1] - b[0]) * (b[3] - b[2]) for b in boxes.values())
print(f"L={L}: {len(boxes)} placed, part area {used:.0f} mm2 of {L * W - NOTCH_W * NOTCH_D:.0f}"
      f" ({used / (L * W - NOTCH_W * NOTCH_D) * 100:.0f} %)")
print("\n".join(problems) if problems else "no overlaps, no keep-out hits")

import json

json.dump({"L": L, "boxes": boxes, "W": W, "notch": [ncx - NOTCH_W / 2, ncx + NOTCH_W / 2, W - NOTCH_D]},
          open(DST.replace(".kicad_pcb", ".json"), "w"))
