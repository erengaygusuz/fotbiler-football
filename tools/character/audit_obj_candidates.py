#!/usr/bin/env python3

from pathlib import Path
import struct

ROOT = Path(
    "character_work/asset_packs/work/fotbiler_candidates"
)


def png_size(path):
    try:
        with path.open("rb") as f:
            sig = f.read(24)

        if sig[:8] != b"\x89PNG\r\n\x1a\n":
            return None

        return struct.unpack(">II", sig[16:24])
    except Exception:
        return None


print()
print("=== FOTBILER CHARACTER CANDIDATES ===")

for obj in sorted(ROOT.rglob("*.obj")):
    verts = 0
    texcoords = 0
    normals = 0
    faces = 0
    tris = 0

    with obj.open(
        "r",
        encoding="utf-8",
        errors="replace"
    ) as f:
        for line in f:
            if line.startswith("v "):
                verts += 1
            elif line.startswith("vt "):
                texcoords += 1
            elif line.startswith("vn "):
                normals += 1
            elif line.startswith("f "):
                parts = line.split()[1:]
                faces += 1

                if len(parts) >= 3:
                    tris += len(parts) - 2

    print()
    print(obj)
    print("  vertices :", verts)
    print("  faces    :", faces)
    print("  triangles:", tris)
    print("  UVs      :", texcoords)
    print("  normals  :", normals)

    textures = sorted(obj.parent.glob("*.png"))

    for tex in textures:
        size = png_size(tex)

        print(
            "  texture  :",
            tex.name,
            f"{size[0]}x{size[1]}" if size else ""
        )

print()
print("SUCCESS")
