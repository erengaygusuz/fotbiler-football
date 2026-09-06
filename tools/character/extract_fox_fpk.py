#!/usr/bin/env python3

import argparse
import struct
from pathlib import Path


HEADER = struct.Struct("<7s3sI18siii4s")
ENTRY = struct.Struct("<QQQQ16s")


def read_cstring(data, offset):
    if offset < 0 or offset >= len(data):
        raise RuntimeError(
            f"Invalid string offset: {offset}"
        )

    end = data.find(b"\0", offset)

    if end == -1:
        raise RuntimeError(
            f"Unterminated string at {offset}"
        )

    return data[offset:end].decode(
        "utf-8",
        errors="replace"
    )


def parse(path):
    data = path.read_bytes()

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

    magic = magic.rstrip(b"\0").decode(
        "ascii",
        errors="replace"
    )

    if magic not in ("foxfpk", "foxfpkd"):
        raise RuntimeError(
            f"Unsupported FPK magic: {magic}"
        )

    entries = []

    for i in range(entry_count):
        table_offset = (
            HEADER.size
            + i * ENTRY.size
        )

        (
            data_offset,
            data_size,
            name_offset,
            name_length,
            md5,
        ) = ENTRY.unpack_from(
            data,
            table_offset
        )

        name = read_cstring(
            data,
            name_offset
        )

        if data_offset + data_size > len(data):
            raise RuntimeError(
                f"Entry {i} exceeds file bounds."
            )

        entries.append({
            "index": i,
            "name": name,
            "offset": data_offset,
            "size": data_size,
            "md5": md5.hex(),
        })

    return data, entries


def safe_output(root, name):
    root = root.resolve()

    # FPK entry may theoretically contain directories.
    candidate = (
        root / Path(name)
    ).resolve()

    if (
        candidate != root
        and root not in candidate.parents
    ):
        raise RuntimeError(
            f"Unsafe entry path: {name}"
        )

    return candidate


def main():
    ap = argparse.ArgumentParser()

    ap.add_argument("input")
    ap.add_argument("output")

    ap.add_argument(
        "--index",
        type=int,
        action="append",
        default=[]
    )

    args = ap.parse_args()

    src = Path(args.input)
    dst = Path(args.output)

    data, entries = parse(src)

    selected = entries

    if args.index:
        wanted = set(args.index)

        selected = [
            e for e in entries
            if e["index"] in wanted
        ]

    dst.mkdir(
        parents=True,
        exist_ok=True
    )

    print()
    print("=== FOX FPK EXTRACT ===")

    for e in selected:
        out = safe_output(
            dst,
            e["name"]
        )

        out.parent.mkdir(
            parents=True,
            exist_ok=True
        )

        start = e["offset"]
        end = start + e["size"]

        out.write_bytes(
            data[start:end]
        )

        print(
            f'{e["index"]:3d}  '
            f'{e["size"]:10d}  '
            f'{out}'
        )

    print()
    print(
        "Extracted:",
        len(selected)
    )

    print("SUCCESS")


if __name__ == "__main__":
    main()
