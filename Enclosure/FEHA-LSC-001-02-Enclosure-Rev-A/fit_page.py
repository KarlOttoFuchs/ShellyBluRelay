"""
Generate enclosure-fit.html: true-scale sections of the enclosure with the board in place.

Every drawing is a slice of the FreeCAD model that build_enclosure.py writes (tube, both caps
and the board STEP), so run build_enclosure.py first, then:
    /Applications/FreeCAD.app/Contents/Resources/bin/freecadcmd -c \
        "exec(open('fit_page.py').read())"
Dimension labels are read from the model's Params sheet. The page text lives in
fit_page_template.html, where %%NAME%% markers take the drawings; the prose quotes numbers too,
so re-read it after changing a parameter. The claude.ai artifact "LSC Tube Fit" is published
from the same page.

Drawings use mm as SVG user units. Sections are seen from the OUT end (+Y on the right),
SVG y = -Z; the top view has SVG y = -Y.
"""

import os
import re

import FreeCAD as App
import Part
from FreeCAD import Vector

try:
    HERE = os.path.dirname(os.path.abspath(__file__))
except NameError:       # exec() and some macro runners do not set __file__
    HERE = os.getcwd()
if not os.path.isfile(os.path.join(HERE, "fit_page.py")):
    HERE = os.path.expanduser(
        "~/Documents/Development/Projects/ESP32C3 Relay Module/Enclosure/"
        "FEHA-LSC-001-02-Enclosure-Rev-A")
BOARD_FILE = os.path.normpath(os.path.join(
    HERE, "../../Hardware/FEHA-LSC-001-01-Controller-Rev-A/"
    "FEHA-LSC-001-01-Controller-Rev-A.kicad_pcb"))
MODEL = os.path.join(HERE, "FEHA-LSC-001-02-Enclosure-Rev-A.FCStd")

# Section planes and the parts labelled in the top view
X_CONN = -22.95         # through the IN connector's wire entries, inside the cap sleeve
Y_LONG = 2.0            # through one wire entry
TOP_LABELS = [("CN1", "IN"), ("CN2", "OUT"), ("U2", "ESP32-C3"), ("L1", ""), ("D1", ""),
              ("D2", ""), ("Q2", "")]


# ---------------------------------------------------------------- model ----------------
def load_model():
    doc = App.openDocument(MODEL)
    sheet = doc.getObject("Params")
    p = {}
    for row in range(1, 200):
        alias = sheet.getAlias(f"B{row}")
        if alias:
            p[alias] = float(sheet.get(alias))
    shapes = {"tube": doc.getObject("Tube").Shape,
              "caps": [doc.getObject("Cap").Shape, Part.getShape(doc.getObject("Cap_OUT"))]}
    pcb = next(o for o in doc.Objects if o.Label == "PCB")
    parts = []
    for label, sub in leaves(pcb):
        s = Part.getShape(pcb, sub, needSubElement=True)
        if s.Solids:
            parts.append((part_class(label), s))
    shapes["parts"] = parts
    return doc, p, shapes


def leaves(obj, prefix=""):
    """(label, subname) for every shape-carrying leaf under an App::Part tree."""
    for child in getattr(obj, "Group", []):
        sub = f"{prefix}{child.Name}."
        if child.TypeId == "App::Part":
            yield from leaves(child, sub)
        elif child.isDerivedFrom("Part::Feature") and not child.Shape.isNull():
            yield child.Label, sub


def part_class(label):
    if label.endswith("_PCB"):
        return "pcb"
    if label.startswith("CONN"):
        return "conn"
    if label.startswith("WIFIM"):
        return "mcu"
    return "part"


def board_refs():
    """Footprint centres in board-centre coordinates, Y up."""
    text = open(BOARD_FILE, encoding="utf-8").read()
    ox, oy = map(float, re.search(r"\(aux_axis_origin ([-\d.]+) ([-\d.]+)\)", text).groups())
    refs = {}
    for fp in text.split("\n\t(footprint ")[1:]:
        ref = re.search(r'\(property "Reference" "([^"]+)"', fp)
        at = re.search(r"\n\t\t\(at ([-\d.]+) ([-\d.]+)", fp)
        if ref and at:
            refs[ref.group(1)] = (float(at.group(1)) - ox, oy - float(at.group(2)))
    return refs


# ---------------------------------------------------------------- slicing --------------
def path(shape, normal, dist, i, j):
    """SVG path data for the section of shape by a plane: axis i across, axis j up."""
    try:
        wires = shape.slice(normal, dist)
    except Exception:       # plane misses the shape
        return ""
    out = []
    for w in wires:
        pts = w.discretize(Deflection=0.01)
        if len(pts) > 1:
            d = " L".join(f"{q[i]:.3f} {-q[j]:.3f}" for q in pts)
            out.append(f"M{d}{' Z' if w.isClosed() else ''}")
    return " ".join(out)


def section(shapes, normal, dist, i, j, caps=True):
    v = {"tube": path(shapes["tube"], normal, dist, i, j)}
    if caps:
        v["cap"] = " ".join(path(c, normal, dist, i, j) for c in shapes["caps"])
    for cls in ("pcb", "part", "conn", "mcu"):
        v[cls] = " ".join(path(s, normal, dist, i, j) for c, s in shapes["parts"] if c == cls)
    return v


def top_view(shapes, p):
    z = Vector(0, 0, 1)
    v = {"tube": path(shapes["tube"], z, p["TUBE_H"] - p["WALL"] / 2, 0, 1),
         "groove": path(shapes["tube"], z, p["BOARD_Z"] + p["PCB_T"] / 2, 0, 1),
         "cap": " ".join(path(c, z, 0.5, 0, 1) for c in shapes["caps"])}
    for cls, h in (("pcb", p["BOARD_Z"] + p["PCB_T"] / 2), ("conn", None), ("mcu", None),
                   ("part", None)):
        h = h or p["BOARD_Z"] + 1.75       # just above the lowest parts (0402, ~0.35 mm)
        v[cls] = " ".join(path(s, z, h, 0, 1) for c, s in shapes["parts"] if c == cls)
    return v


# ---------------------------------------------------------------- drawing helpers ------
def mm(x):
    return f"{x:.1f}"


def layers(v):
    out = []
    if v.get("cap", "").strip():
        out.append(f'<path class="cap" d="{v["cap"]}"/>')
    out.append(f'<path class="tube" d="{v["tube"]}"/>')
    for cls in ("pcb", "part", "conn", "mcu"):
        if v.get(cls, "").strip():
            out.append(f'<path class="{cls}" d="{v[cls]}"/>')
    return "\n".join(out)


def dimh(x0, x1, y, txt, fs, above=True):
    t = fs * 0.45
    ty = y - fs * 0.4 if above else y + fs * 1.05
    return (f'<path class="dim" d="M{x0} {y} H{x1} M{x0} {y - t} V{y + t} M{x1} {y - t} '
            f'V{y + t}"/><text class="dimt" x="{(x0 + x1) / 2}" y="{ty}" text-anchor="middle" '
            f'style="font-size:{fs}px">{txt}</text>')


def dimv(x, y0, y1, txt, fs):
    t = fs * 0.45
    return (f'<path class="dim" d="M{x} {y0} V{y1} M{x - t} {y0} H{x + t} M{x - t} {y1} '
            f'H{x + t}"/><text class="dimt" x="{x - fs * 0.5}" y="{(y0 + y1) / 2 + fs * 0.35}" '
            f'text-anchor="end" style="font-size:{fs}px">{txt}</text>')


def lead(pts, txt, fs, sub=None):
    d = " L".join(f"{x} {y}" for x, y in pts)
    x, y = pts[-1]
    s = (f'<path class="lead" d="M{d}"/><text x="{x + fs * 0.3}" y="{y + fs * 0.35}" '
         f'style="font-size:{fs}px">{txt}</text>')
    if sub:
        s += (f'<text class="ts" x="{x + fs * 0.3}" y="{y + fs * 1.5}" '
              f'style="font-size:{fs * 0.85}px">{sub}</text>')
    return s


def label(x, y, txt, fs, cls="ts", anchor="middle"):
    return (f'<text class="{cls}" x="{x}" y="{y}" text-anchor="{anchor}" '
            f'style="font-size:{fs}px">{txt}</text>')


# ---------------------------------------------------------------- drawings -------------
def draw_mid(v, p, roof_gap):
    f = 1.05
    hw, iw, bw = p["TUBE_W"] / 2, p["IN_W"] / 2, p["PCB_W"] / 2
    groove_mid = -(p["GROOVE_Z"] + p["GROOVE_H"] / 2)
    return f'''<svg viewBox="-23.5 -14.8 64.5 20" role="img" aria-label="Cross-section through the tube middle and the ESP32 module">
{layers(v)}
{dimh(-bw, bw, 1.4, f"board {mm(p['PCB_W'])}", f, above=False)}
{dimh(-hw, hw, 3.2, f"tube {mm(p['TUBE_W'])}", f, above=False)}
{dimh(-iw, iw, -p['TUBE_H'] - 1.0, f"inside {mm(p['IN_W'])}", f)}
{dimv(-hw - 1.2, 0, -p['TUBE_H'], mm(p['TUBE_H']), f)}
{lead([(10, -7.39), (10, -11.4), (hw + 1.5, -11.4)], f"module, {roof_gap['mcu']:.1f} mm under the roof", f, "antenna over the board notch")}
{lead([(iw + 1.3, groove_mid), (hw + 1.5, groove_mid)], f"groove {mm(p['GROOVE_H'])} × {mm(p['GROOVE_D'])}", f, "full length, both walls")}
{lead([(hw - 1.1, -1), (hw + 1.5, -1)], f"side wall {mm(p['WALL_SIDE'])}", f, f"top and bottom {mm(p['WALL'])}")}
</svg>'''


def draw_groove(v, p):
    f = 0.13
    iw, bw = p["IN_W"] / 2, p["PCB_W"] / 2
    floor, outer = iw + p["GROOVE_D"], p["TUBE_W"] / 2
    z0, z1 = -p["GROOVE_Z"], -(p["GROOVE_Z"] + p["GROOVE_H"])
    return f'''<svg viewBox="{-outer - 0.9} {z1 - 1.35} 9.6 4.2" role="img" aria-label="Detail of the board edge in the wall groove">
{layers(v)}
{dimv(-outer - 0.3, z0, z1, mm(p['GROOVE_H']), f)}
{dimh(-floor, -iw, z1 - 0.5, f"{mm(p['GROOVE_D'])} deep", f)}
{dimh(-floor, -bw, z1 - 0.8, mm(p['SIDE_CLR']), f)}
{dimh(-outer, -floor, z0 + 0.45, f"{mm(outer - floor)} wall behind", f, above=False)}
{dimh(-bw, -iw, z0 + 0.2, mm(p['GROOVE_OVER']), f, above=False)}
{lead([(-iw + 0.3, (z0 + z1) / 2), (-iw + 1.3, z0 - 0.05), (-iw + 1.7, z0 - 0.05)], f"board edge overlaps {mm(p['GROOVE_OVER'])}", f, f"{mm(p['SIDE_CLR'])} clear to the groove floor")}
{lead([(-iw, z1 - 0.35), (-iw + 1.7, z1 - 0.35)], "wall face above the groove", f, "nothing on the board top within 1.0 mm (CON-3)")}
</svg>'''


def draw_conn(v, p):
    f = 1.05
    hw, ow = p["TUBE_W"] / 2, p["TUBE_W"] / 2 + p["CAP_O"]
    wz, r = -p["WIRE_Z"], p["CABLE_D"] / 2
    behind = p["CHAMBER_L"] + p["PLATE_T"]
    return f'''<svg viewBox="-23.5 -16.2 64.5 20.4" role="img" aria-label="Cross-section near the IN end through the push-in connector and the cap sleeve">
{layers(v)}
<circle class="ghost" cx="0" cy="{wz}" r="{r}"/>
<path class="lead" d="M-2 {wz} H2 M0 {wz - 2} V{wz + 2}"/>
{dimv(-ow - 1.3, p['CAP_O'], -(p['TUBE_H'] + p['CAP_O']), mm(p['OVERALL_H']), f)}
{dimh(-ow, ow, 2.4, f"cap {mm(p['OVERALL_W'])}", f, above=False)}
{lead([(2, wz - 0.9), (9, -13.3), (ow + 1.7, -13.3)], f"wire entries y ±2.0, {mm(p['WIRE_Z'])} up", f, "centred in the cable exit")}
{lead([(r, wz + 1.2), (9, -9.6), (ow + 1.7, -9.6)], f"cable exit Ø{mm(p['CABLE_D'])} (dashed)", f, f"in the end plate, {mm(behind)} mm behind")}
{lead([(ow - 0.5, -6.1), (ow + 1.7, -6.1)], f"cap sleeve {mm(p['CAP_WALL'])} wall", f, f"{mm(p['CAP_CLR'])} clear around the tube")}
{lead([(hw, -2.5), (ow + 1.7, -2.5)], "tube end", f, f"sleeve overlaps it {mm(p['SLEEVE_L'])} mm")}
</svg>'''


def draw_long(v, p, roof_gap):
    f = 1.5
    ol, tl = p["OVERALL_L"] / 2, p["TUBE_L"] / 2
    tab, chamber = -p["EAR_HX"], -(p["X_END"] - p["CHAMBER_L"] / 2)
    return f'''<svg viewBox="-54 -19 108 25" role="img" aria-label="Lengthwise section of the assembled enclosure through one wire entry">
{layers(v)}
{dimh(-ol, ol, 3.6, f"overall {mm(p['OVERALL_L'])} incl. screw tabs", f, above=False)}
{dimh(-tl, tl, -16.2, f"tube {mm(p['TUBE_L'])}", f)}
{label(-tab, -12.2, "IN", f * 0.9)}
{label(tab, -12.2, "OUT", f * 0.9)}
{label(-tab, 2.4, "screw tab", f * 0.75)}
{label(tab, 2.4, "screw tab", f * 0.75)}
{label(-chamber, -7.9, "cable chamber", f * 0.75)}
{label(0, -p['TUBE_H'] - 0.5, f"roof {roof_gap['conn']:.2f} mm over the connectors", f * 0.75)}
</svg>'''


def draw_top(v, p, refs):
    f = 1.45
    tags = ""
    for ref, sub in TOP_LABELS:
        x, y = refs[ref][0], -refs[ref][1]
        if sub:
            tags += label(x, y - 0.3, ref, f, "fl") + label(x, y + f * 1.1, sub, f * 0.8, "fl")
        else:
            tags += label(x, y + f * 0.35, ref, f * 0.8, "fl")
    bx, by, lx, ly = p["BTN_X"], -p["BTN_Y"], p["LED_X"], -p["LED_Y"]
    hl, hw = p["PCB_L"] / 2, p["PCB_W"] / 2
    return f'''<svg viewBox="-54 -24 108 49" role="img" aria-label="Top view of the board in the tube with the caps, pinhole and LED window">
<path class="capflat" d="{v["cap"]}"/>
<path class="pcbflat" d="{v["pcb"]}"/>
<rect class="keep" x="{-hl}" y="{-hw}" width="{p['PCB_L']}" height="1"/><rect class="keep" x="{-hl}" y="{hw - 1}" width="{p['PCB_L']}" height="1"/>
<path class="partflat" d="{v["part"]}"/>
<path class="connflat" d="{v["conn"]}"/>
<path class="mcuflat" d="{v["mcu"]}"/>
<path class="hidden" d="{v["groove"]}"/>
<path class="tubeline" d="{v["tube"]}"/>
{tags}
<circle class="hole" cx="{bx}" cy="{by}" r="{p['PINHOLE_D'] / 2}"/><circle class="hole" cx="{lx}" cy="{ly}" r="{p['LED_HOLE_D'] / 2}"/>
<path class="lead" d="M{bx + 0.8} {by} L26.8 8.5"/>{label(27.3, 9.0, "pinhole over S1", f, "t", "start")}
<path class="lead" d="M{lx + 1.6} {ly} L26.8 12.4"/>{label(27.3, 12.9, "LED window over D3", f, "t", "start")}
{label(-hl, -19.4, "antenna side (+Y)", f * 0.85, anchor="start")}
{label(-36, -22.0, "stop ribs block the board corners", f * 0.85, anchor="start")}
{dimh(-hl, hl, 22.2, f"board {mm(p['PCB_L'])} · {mm(p['END_PLAY'])} play each end", f, above=False)}
</svg>'''


def roof_gaps(shapes, p):
    """Gap from the tallest connector and from the module to the tube roof."""
    roof = p["WALL"] + p["IN_H"]
    top = {c: max(s.BoundBox.ZMax for k, s in shapes["parts"] if k == c) for c in ("conn", "mcu")}
    return {c: roof - z for c, z in top.items()}


# ---------------------------------------------------------------- main -----------------
def main():
    doc, p, shapes = load_model()
    gaps = roof_gaps(shapes, p)
    mid = section(shapes, Vector(1, 0, 0), 0.0, 1, 2, caps=False)
    svgs = {
        "mid": draw_mid(mid, p, gaps),
        "groove": draw_groove(mid, p),
        "conn": draw_conn(section(shapes, Vector(1, 0, 0), X_CONN, 1, 2), p),
        "long": draw_long(section(shapes, Vector(0, 1, 0), Y_LONG, 0, 2), p, gaps),
        "top": draw_top(top_view(shapes, p), p, board_refs()),
    }
    page = open(os.path.join(HERE, "fit_page_template.html"), encoding="utf-8").read()
    for key, svg in svgs.items():
        page = page.replace(f"%%{key.upper()}%%", svg)
    if "%%" in page:
        raise SystemExit("fit_page_template.html has an unfilled %%MARKER%%")
    # The artifact publish wraps the page in a document skeleton; the local file needs its own.
    html = ('<!doctype html>\n<html lang="en"><head><meta charset="utf-8">'
            '<meta name="viewport" content="width=device-width,initial-scale=1"></head>\n'
            f'<body>\n{page}\n</body></html>\n')
    open(os.path.join(HERE, "enclosure-fit.html"), "w", encoding="utf-8").write(html)
    App.closeDocument(doc.Name)
    print(f"enclosure-fit.html written; roof gap {gaps['conn']:.2f} mm over the connectors, "
          f"{gaps['mcu']:.2f} mm over the module")


main()
