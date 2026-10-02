#!/Library/Frameworks/Python.framework/Versions/3.12/bin/python3.12
"""
lcsc_to_kicad.py — Import LCSC/EasyEDA components into the project KiCad library.

Usage:
    ./scripts/lcsc_to_kicad.py C3013946

Why this script exists:
    easyeda2kicad (v0.8.0) uses Python's `requests` library, which is blocked by
    EasyEDA's CDN via TLS fingerprinting (JA3). curl is not blocked. This script
    fetches the EasyEDA API data via a subprocess curl call and feeds it directly
    into the easyeda2kicad conversion pipeline, bypassing the network issue.

    Pin-numbering caveat: some EasyEDA source symbols (e.g. ESP32-S3-WROOM-1U,
    C3013946) carry per-side sequential pin numbers (1..N repeated per edge) that
    do NOT match the footprint pad numbers. This is bad upstream data, not a tool
    bug — neither the CLI nor this pipeline can fix it. After importing such a
    part, verify the symbol pin numbers against the datasheet pad map and renumber
    if they duplicate.

3D model convention:
    Step files go in Library/3DModels/ and are referenced as ${KIPRJMOD}/Library/3DModels/
    The easyeda2kicad exporter writes an absolute .wrl path by default — this script
    corrects it automatically.
"""

import json
import os
import re
import subprocess
import sys

sys.path.insert(0, '/Users/karlfuchs/Library/Python/3.12/lib/python/site-packages')

from easyeda2kicad.easyeda.easyeda_importer import (
    Easyeda3dModelImporter,
    EasyedaFootprintImporter,
    EasyedaSymbolImporter,
)
from easyeda2kicad.helpers import (
    add_component_in_symbol_lib_file,
    id_already_in_symbol_lib,
    update_component_in_symbol_lib_file,
)
from easyeda2kicad.kicad.export_kicad_3d_model import Exporter3dModelKicad
from easyeda2kicad.kicad.export_kicad_footprint import ExporterFootprintKicad
from easyeda2kicad.kicad.export_kicad_symbol import ExporterSymbolKicad
from easyeda2kicad.kicad.parameters_kicad_symbol import KicadVersion

LIB_ROOT = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))  # the dir holding Library/
LIB_NAME = "_FEHA-LSC-001"
LIB_BASE = os.path.join(LIB_ROOT, "Library", LIB_NAME)
MODELS_DIR = os.path.join(LIB_ROOT, "Library", "3DModels")

EASYEDA_API = "https://easyeda.com/api/products/{lcsc_id}/components?version=6.4.19.5"


def fetch_easyeda_data(lcsc_id: str) -> dict:
    url = EASYEDA_API.format(lcsc_id=lcsc_id)
    result = subprocess.run(["curl", "-s", url], capture_output=True, text=True)
    if result.returncode != 0:
        raise RuntimeError(f"curl failed: {result.stderr}")
    data = json.loads(result.stdout)
    if not data.get("success"):
        raise RuntimeError(f"EasyEDA API error: {data}")
    return data["result"]


def fix_footprint_3d_path(kicad_mod_path: str, model_name: str):
    """Replace the absolute .wrl path easyeda2kicad writes with the project-relative .step path."""
    with open(kicad_mod_path) as f:
        content = f.read()
    fixed = re.sub(
        r'\(model "[^"]*"(\s+\(offset)',
        f'(model "${{KIPRJMOD}}/Library/3DModels/{model_name}.step"\\1',
        content,
    )
    with open(kicad_mod_path, "w") as f:
        f.write(fixed)


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <LCSC_ID>  (e.g. C3013946)")
        sys.exit(1)

    lcsc_id = sys.argv[1]
    print(f"Fetching EasyEDA data for {lcsc_id} via curl...")
    cad_data = fetch_easyeda_data(lcsc_id)

    # --- Symbol ---
    importer = EasyedaSymbolImporter(easyeda_cp_cad_data=cad_data)
    easyeda_symbol = importer.get_symbol()
    component_name = easyeda_symbol.info.name
    lib_path = f"{LIB_BASE}.kicad_sym"
    kicad_version = KicadVersion.v6

    already_in_lib = id_already_in_symbol_lib(lib_path=lib_path, component_name=component_name, kicad_version=kicad_version)
    exporter = ExporterSymbolKicad(symbol=easyeda_symbol, kicad_version=kicad_version)
    kicad_symbol_lib = exporter.export(footprint_lib_name=LIB_NAME)

    if already_in_lib:
        update_component_in_symbol_lib_file(lib_path=lib_path, component_name=component_name, component_content=kicad_symbol_lib, kicad_version=kicad_version)
        print(f"Updated symbol '{component_name}' in {lib_path}")
    else:
        add_component_in_symbol_lib_file(lib_path=lib_path, component_content=kicad_symbol_lib, kicad_version=kicad_version)
        print(f"Added symbol '{component_name}' to {lib_path}")

    # --- Footprint ---
    fp_importer = EasyedaFootprintImporter(easyeda_cp_cad_data=cad_data)
    easyeda_footprint = fp_importer.get_footprint()
    footprint_name = easyeda_footprint.info.name
    footprint_dir = f"{LIB_BASE}.pretty"
    footprint_path = os.path.join(footprint_dir, f"{footprint_name}.kicad_mod")

    os.makedirs(footprint_dir, exist_ok=True)
    # Temp 3dshapes dir — easyeda2kicad requires a path, we'll clean it up after
    temp_3dshapes = f"{LIB_BASE}.3dshapes"
    ExporterFootprintKicad(footprint=easyeda_footprint).export(
        footprint_full_path=footprint_path,
        model_3d_path=temp_3dshapes,
    )
    print(f"Added footprint '{footprint_name}' to {footprint_dir}/")

    # --- 3D Model ---
    os.makedirs(MODELS_DIR, exist_ok=True)
    os.makedirs(temp_3dshapes, exist_ok=True)
    model_name = None
    try:
        model_importer = Easyeda3dModelImporter(easyeda_cp_cad_data=cad_data, download_raw_3d_model=True)
        if model_importer.output:
            exporter_3d = Exporter3dModelKicad(model_3d=model_importer.output)
            exporter_3d.export(lib_path=LIB_BASE)
            model_name = exporter_3d.output.name if exporter_3d.output else None

            # Move step file to 3DModels/, discard .wrl
            if model_name:
                step_src = os.path.join(temp_3dshapes, f"{model_name}.step")
                step_dst = os.path.join(MODELS_DIR, f"{model_name}.step")
                if os.path.exists(step_src):
                    os.rename(step_src, step_dst)
                    print(f"Saved 3D model to Library/3DModels/{model_name}.step")
        else:
            print("No 3D model available.")
    except Exception as e:
        print(f"3D model skipped: {e}")
    finally:
        # Clean up temp .3dshapes dir
        import shutil
        if os.path.isdir(temp_3dshapes):
            shutil.rmtree(temp_3dshapes)

    # Fix the 3D model path in the footprint (easyeda2kicad writes absolute .wrl path)
    if model_name and os.path.exists(footprint_path):
        fix_footprint_3d_path(footprint_path, model_name)
        print(f"Fixed 3D model path in footprint -> ${{KIPRJMOD}}/Library/3DModels/{model_name}.step")


if __name__ == "__main__":
    main()
