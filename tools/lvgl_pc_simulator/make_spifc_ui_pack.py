#!/usr/bin/env python3
"""Pack converted LVGL .bin files for the raw U3 SPI-NOR UI partition.

The resulting u3_ui.bin is copied to SD:/UI/.  The one-time board installer
places it at U3 address 0x001B0000; LVGL then opens resources from this packed
index without a FAT filesystem.
"""

from __future__ import annotations

import argparse
import struct
import zlib
from pathlib import Path


MANIFEST_MAGIC = 0x55494D47  # UIMG
MANIFEST_HEADER = struct.Struct("<IIHH")
MANIFEST_ENTRY = struct.Struct("<64sII")
PACK_MAGIC = 0x49555053  # SPUI
PACK_HEADER = struct.Struct("<IIHHII")
PACK_ENTRY = struct.Struct("<64sIII")
PAGE_BYTES = 256
MAX_FILES = 256


def align(value: int, size: int = PAGE_BYTES) -> int:
    return (value + size - 1) & ~(size - 1)


def main() -> None:
    parser = argparse.ArgumentParser(description="build raw U3 LVGL UI pack")
    parser.add_argument("--ui-dir", type=Path, required=True)
    parser.add_argument("--output", type=Path, default=None)
    args = parser.parse_args()
    ui_dir = args.ui_dir
    manifest = ui_dir / "manifest.bin"
    if not manifest.is_file():
        parser.error(f"missing {manifest}; run make_nand_ui_manifest.py first")

    raw = manifest.read_bytes()
    if len(raw) < MANIFEST_HEADER.size:
        parser.error("manifest is truncated")
    magic, version, count, header_size = MANIFEST_HEADER.unpack_from(raw)
    if magic != MANIFEST_MAGIC or header_size != MANIFEST_HEADER.size or not (1 <= count <= MAX_FILES):
        parser.error("manifest header is invalid")
    expected_size = MANIFEST_HEADER.size + count * MANIFEST_ENTRY.size
    if len(raw) != expected_size:
        parser.error("manifest entry table is truncated")

    entries: list[tuple[bytes, bytes, int]] = []
    for index in range(count):
        name_raw, size, expected_crc = MANIFEST_ENTRY.unpack_from(
            raw, MANIFEST_HEADER.size + index * MANIFEST_ENTRY.size
        )
        name = name_raw.split(b"\0", 1)[0]
        if not name:
            parser.error(f"empty manifest entry {index}")
        asset = (ui_dir / name.decode("ascii")).read_bytes()
        if len(asset) != size or (zlib.crc32(asset) & 0xFFFFFFFF) != expected_crc:
            parser.error(f"asset validation failed: {name.decode('ascii')}")
        entries.append((name_raw, asset, expected_crc))

    table_offset = PACK_HEADER.size
    data_offset = align(table_offset + count * PACK_ENTRY.size)
    image = bytearray(b"\xFF" * data_offset)
    offsets: list[int] = []
    for _, asset, _ in entries:
        offsets.append(len(image))
        image.extend(asset)
        image.extend(b"\xFF" * (align(len(image)) - len(image)))

    if len(image) > 5 * 1024 * 1024:
        parser.error(f"packed UI is {len(image)} bytes; exceeds 5 MiB reserved U3 region")
    table = bytearray()
    for (name_raw, asset, crc32), offset in zip(entries, offsets):
        table.extend(PACK_ENTRY.pack(name_raw, offset, len(asset), crc32))
    table_crc = zlib.crc32(table) & 0xFFFFFFFF
    header = PACK_HEADER.pack(PACK_MAGIC, version, count, PACK_HEADER.size, len(image), table_crc)
    image[: PACK_HEADER.size] = header
    image[table_offset : table_offset + len(table)] = table

    output = args.output or (ui_dir / "u3_ui.bin")
    output.write_bytes(image)
    print(
        f"wrote {output}: version={version}, files={count}, "
        f"bytes={len(image)}, crc32={zlib.crc32(image) & 0xFFFFFFFF:08x}"
    )


if __name__ == "__main__":
    main()
