#!/usr/bin/env python3
"""
Descarga los .proto oficiales de Meshtastic en third_party/meshtastic_protos.

Uso:
  python3 scripts/interop/regen_protos.py [--commit <sha>] [--force]

Si no se especifica commit, intenta usar el sha de protos.lock. Si no existe,
lee el sha del submódulo 'protobufs' referenciado por meshtastic/firmware@master.

Nota: Este script no genera código C/C++ (nanopb). Solo trae los .proto.
"""
import argparse
import pathlib
import sys
import urllib.request
import json

ROOT = pathlib.Path(__file__).resolve().parents[2]
DEST = ROOT / "third_party" / "meshtastic_protos" / "meshtastic"
LOCK = ROOT / "third_party" / "meshtastic_protos" / "protos.lock"

PROTO_FILES = [
    # Núcleo de interoperabilidad mínima
    "mesh.proto",
    "portnums.proto",
    "config.proto",
    "channel.proto",
    # Soporte complementario habitual
    "module_config.proto",
    "telemetry.proto",
    "clientonly.proto",
    "deviceonly.proto",
    "interdevice.proto",
]

NANOPB_PROTO = "nanopb.proto"


def http_json(url: str):
    with urllib.request.urlopen(url) as r:
        return json.loads(r.read().decode("utf-8"))


def fetch_submodule_sha_from_firmware_master() -> str:
    # Lee el sha del submódulo 'protobufs' en meshtastic/firmware@master
    url = "https://api.github.com/repos/meshtastic/firmware/contents/protobufs?ref=master"
    data = http_json(url)
    sha = data.get("sha")
    if not sha:
        raise RuntimeError("No se pudo obtener SHA de submódulo protobufs")
    return sha


from typing import Optional


def read_lock_sha() -> Optional[str]:
    if LOCK.exists():
        return LOCK.read_text(encoding="utf-8").strip()
    return None


def write_lock_sha(sha: str):
    LOCK.parent.mkdir(parents=True, exist_ok=True)
    LOCK.write_text(sha + "\n", encoding="utf-8")


def download(url: str, dest: pathlib.Path):
    dest.parent.mkdir(parents=True, exist_ok=True)
    with urllib.request.urlopen(url) as r, open(dest, "wb") as f:
        f.write(r.read())


def ensure_nanopb_proto():
    # nanopb.proto vive en la raíz del repo meshtastic/protobufs@master
    url = f"https://raw.githubusercontent.com/meshtastic/protobufs/master/{NANOPB_PROTO}"
    dest = ROOT / "third_party" / "meshtastic_protos" / NANOPB_PROTO
    download(url, dest)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--commit", help="SHA de meshtastic/protobufs a usar")
    ap.add_argument("--force", action="store_true", help="Sobrescribe archivos existentes")
    args = ap.parse_args()

    sha = args.commit or read_lock_sha()
    if not sha:
        sha = fetch_submodule_sha_from_firmware_master()
        print(f"[info] sha desde firmware submodule: {sha}")
        write_lock_sha(sha)
    else:
        print(f"[info] usando sha lock: {sha}")

    base = f"https://raw.githubusercontent.com/meshtastic/protobufs/{sha}/meshtastic"
    for name in PROTO_FILES:
        url = f"{base}/{name}"
        dest = DEST / name
        if dest.exists() and not args.force:
            print(f"[skip] {dest} (existe)")
            continue
        print(f"[get] {url} -> {dest}")
        download(url, dest)

    ensure_nanopb_proto()
    print("[ok] .proto actualizados")


if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print(f"[error] {e}")
        sys.exit(1)
