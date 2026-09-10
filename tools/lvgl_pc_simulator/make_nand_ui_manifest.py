#!/usr/bin/env python3
"""Create UI/manifest.bin for the GPCV2178 one-time NAND UI installer.

Run this after LVGL's Image Converter has produced the .bin resources:
    python make_nand_ui_manifest.py --ui-dir E:\\UI --version 1
"""

from __future__ import annotations

import argparse
import struct
import zlib
from pathlib import Path


MAGIC = 0x55494D47  # "UIMG" in the target's little-endian UINT32 format
HEADER = struct.Struct("<IIHH")
ENTRY = struct.Struct("<64sII")
MAX_FILES = 256
ALLOWED_NAME_CHARS = set("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.-")


def read_entry(path: Path) -> tuple[str, int, int]:
    name = path.name
    if (
        not name.endswith(".bin")
        or name == "manifest.bin"
        or len(name.encode("ascii", "ignore")) != len(name)
        or len(name) >= 64
        or any(ch not in ALLOWED_NAME_CHARS for ch in name)
        or name.startswith(".")
    ):
        raise ValueError(f"invalid UI filename: {name}")

    data = path.read_bytes()
    if not data:
        raise ValueError(f"empty image file: {path}")
    return name, len(data), zlib.crc32(data) & 0xFFFFFFFF


def main() -> None:
    parser = argparse.ArgumentParser(description="create manifest.bin for the NAND UI installer")
    parser.add_argument("--ui-dir", type=Path, required=True, help="SD card's UI directory")
    parser.add_argument("--version", type=int, required=True, help="positive monotonically increasing UI version")
    args = parser.parse_args()

    if args.version < 1 or args.version > 0xFFFFFFFF:
        parser.error("--version must be in range 1..4294967295")
    if not args.ui_dir.is_dir():
        parser.error(f"not a directory: {args.ui_dir}")

    files = sorted(path for path in args.ui_dir.glob("*.bin")
                   if path.name not in {"manifest.bin", "u3_ui.bin"})
    if not files:
        parser.error("no .bin image files found directly in UI directory")
    if len(files) > MAX_FILES:
        parser.error(f"too many image files ({len(files)} > {MAX_FILES})")

    entries = [read_entry(path) for path in files]
    output = bytearray(HEADER.pack(MAGIC, args.version, len(entries), HEADER.size))
    for name, size, crc32 in entries:
        output.extend(ENTRY.pack(name.encode("ascii").ljust(64, b"\0"), size, crc32))

    manifest = args.ui_dir / "manifest.bin"
    temporary = args.ui_dir / "manifest.bin.tmp"
    temporary.write_bytes(output)
    temporary.replace(manifest)
    print(f"wrote {manifest}: version={args.version}, files={len(entries)}, bytes={len(output)}")


if __name__ == "__main__":
    main()
