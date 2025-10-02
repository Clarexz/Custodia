Import("env")

"""
Extra script de PlatformIO (opcional).

Objetivo en Fase 0: solo informar si existe el material de protos y si hay
herramientas para generar nanopb. No ejecuta nada por defecto para no romper builds.

Para habilitar generación automática en el futuro:
  - Instala protoc y nanopb
  - Descomenta en platformio.ini:
        extra_scripts = extra_scripts/meshtastic_codegen.py
  - Exporta MESHTASTIC_INTEROP_GEN=1 si quieres que ejecute codegen
"""

import os
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PROTO_DIR = ROOT / "third_party" / "meshtastic_protos"
OUT_DIR = ROOT / "lib" / "meshtastic_protos"


def _log(msg):
    print("[meshtastic_codegen]", msg)


def exists(cmd):
    return shutil.which(cmd) is not None


def run():
    if not PROTO_DIR.exists():
        _log("No se encontraron protos. Ejecuta scripts/interop/regen_protos.py")
        return

    gen_flag = os.environ.get("MESHTASTIC_INTEROP_GEN", "0") == "1"
    protoc_ok = exists("protoc")
    nanopb_ok = exists("protoc-gen-nanopb") or exists("nanopb_generator.py")

    _log(f"protos: {PROTO_DIR}")
    _log(f"protoc: {'ok' if protoc_ok else 'faltante'} | nanopb: {'ok' if nanopb_ok else 'faltante'}")

    if not gen_flag:
        _log("Generación deshabilitada (MESHTASTIC_INTEROP_GEN!=1). Solo informativo.")
        return

    if not (protoc_ok and nanopb_ok):
        _log("Herramientas faltantes. Aborta codegen.")
        return

    # Aquí iría el codegen real en Fase 1 (cuando se habilite)
    _log("Nada que generar en Fase 0 (placeholder)")


run()

