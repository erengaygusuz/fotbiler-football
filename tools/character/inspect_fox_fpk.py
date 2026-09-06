#!/usr/bin/env python3

import argparse
import struct
from pathlib import Path


HEADER = struct.Struct("<7s3sI18siii4s")
ENTRY = struct.Struct("<QQQQ16s")


def read_cstring(data, offset):
    if offset < 0 or offset >= len(data):
        raise ValueError(f"Invalid string offset: {offset}")

    end = data.find(b"\0", offset)

    if end == -1:
        raise ValueError(
            f"Unterminated string at offset {offset}"
        )

    return data[offset:end].decode(
        "utf-8",
        errors="replace"
    )


def parse_fpk(path):
    data = path.read_bytes()

    if len(data) < HEADER.size:
        raise RuntimeError("File too small.")

    (
        magic,
        platform,
        declared_size,
        _padding1,
        unknown,
        entry_count,
        ref_count,
        _padding2,
    ) = HEADER.unpack_from(data, 0)

    magic_text = magic.rstrip(b"\0").decode(
        "ascii",
        errors="replace"
    )

    platform_text = platform.rstrip(b"\0").decode(
        "ascii",
        errors="replace"
    )

    if magic_text not in ("foxfpk", "foxfpkd"):
        raise RuntimeError(
            f"Not a Fox FPK/FPKD: {magic!r}"
        )

    entries = []

    table_start = HEADER.size

    for i in range(entry_count):
        off = table_start + i * ENTRY.size

        if off + ENTRY.size > len(data):
            raise RuntimeError(
                f"Entry table exceeds file at entry {i}"
            )

        (
            data_offset,
            data_size,
            name_offset,
            name_length,
            md5,
        ) = ENTRY.unpack_from(data, off)

        name = read_cstring(
            data,
            name_offset
        )

        entries.append({
            "index": i,
            "name": name,
            "data_offset": data_offset,
            "data_size": data_size,
            "name_offset": name_offset,
            "name_length": name_length,
            "md5": md5.hex(),
        })

    return {
        "file_size": len(data),
        "declared_size": declared_size,
        "magic": magic_text,
        "platform": platform_text,
        "unknown": unknown,
        "entry_count": entry_count,
        "ref_count": ref_count,
        "entries": entries,
    }


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("input")
    args = ap.parse_args()

    path = Path(args.input)

    info = parse_fpk(path)

    print()
    print("=== FOX FPK ===")
    print("File          :", path)
    print("Magic         :", info["magic"])
    print("Platform      :", info["platform"])
    print("File size     :", info["file_size"])
    print("Declared size :", info["declared_size"])
    print("Entries       :", info["entry_count"])
    print("References    :", info["ref_count"])
    print("Unknown       :", info["unknown"])
    print()

    for e in info["entries"]:
        print(
            f'{e["index"]:3d}  '
            f'{e["data_size"]:10d}  '
            f'{e["name"]}'
        )


if __name__ == "__main__":
    main()
