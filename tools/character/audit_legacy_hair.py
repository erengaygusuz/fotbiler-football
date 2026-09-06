#!/usr/bin/env python3

import re
from pathlib import Path

ROOT = Path(
    "data/media/objects/players/hairstyles"
)

vertex_re = re.compile(
    r"^\s*\*MESH_VERTEX\s+"
    r"(\d+)\s+"
    r"([-+0-9.eE]+)\s+"
    r"([-+0-9.eE]+)\s+"
    r"([-+0-9.eE]+)"
)

face_re = re.compile(
    r"^\s*\*MESH_FACE\s+"
)

node_re = re.compile(
    r'\*NODE_NAME\s+"([^"]+)"'
)


print()
print("=== LEGACY HAIR AUDIT ===")
print()

for path in sorted(ROOT.glob("*.ase")):

    text = path.read_text(
        encoding="utf-8",
        errors="replace"
    )

    vertices = []

    for line in text.splitlines():
        m = vertex_re.match(line)

        if m:
            vertices.append((
                float(m.group(2)),
                float(m.group(3)),
                float(m.group(4)),
            ))

    faces = sum(
        1
        for line in text.splitlines()
        if face_re.match(line)
    )

    nodes = node_re.findall(text)

    print(path.name)
    print("  nodes    :", nodes)
    print("  vertices :", len(vertices))
    print("  faces    :", faces)

    if vertices:
        xs = [v[0] for v in vertices]
        ys = [v[1] for v in vertices]
        zs = [v[2] for v in vertices]

        mins = (
            min(xs),
            min(ys),
            min(zs),
        )

        maxs = (
            max(xs),
            max(ys),
            max(zs),
        )

        size = (
            maxs[0] - mins[0],
            maxs[1] - mins[1],
            maxs[2] - mins[2],
        )

        center = (
            (mins[0] + maxs[0]) * 0.5,
            (mins[1] + maxs[1]) * 0.5,
            (mins[2] + maxs[2]) * 0.5,
        )

        print(
            "  min      :",
            tuple(round(v, 6) for v in mins)
        )

        print(
            "  max      :",
            tuple(round(v, 6) for v in maxs)
        )

        print(
            "  size     :",
            tuple(round(v, 6) for v in size)
        )

        print(
            "  center   :",
            tuple(round(v, 6) for v in center)
        )

    print()

print("=== MPFB FITTED HEAD REFERENCE ===")
print("min  : (-0.083, -0.447, 1.523)")
print("max  : ( 0.083, -0.213, 1.818)")
print("size : ( 0.166,  0.234, 0.295)")
print()
print("SUCCESS")
