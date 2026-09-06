#!/usr/bin/env python3

from pathlib import Path
import re

ROOT = Path("/run/media/ereng/files/projects/fotbiler-football")

REFERENCE = (
    ROOT
    / "data/media/objects/players/hairstyles/short01.ase"
)

SOURCE = (
    ROOT
    / "character_work/asset_packs/work/runtime_short01"
    / "fotbiler_short01.ase"
)

OUTPUT = (
    ROOT
    / "character_work/asset_packs/work/runtime_short01"
    / "fotbiler_short01_aligned.ase"
)


VERTEX_RE = re.compile(
    r"^(\s*\*MESH_VERTEX\s+\d+\s+)"
    r"([-+0-9.eE]+)\s+"
    r"([-+0-9.eE]+)\s+"
    r"([-+0-9.eE]+)"
    r"(.*)$"
)


def read_vertices(path):
    vertices = []

    for line in path.read_text(
        encoding="utf-8",
        errors="replace",
    ).splitlines():

        m = VERTEX_RE.match(line)

        if not m:
            continue

        vertices.append((
            float(m.group(2)),
            float(m.group(3)),
            float(m.group(4)),
        ))

    if not vertices:
        raise RuntimeError(
            f"No MESH_VERTEX entries in {path}"
        )

    return vertices


def bounds(vertices):
    mn = tuple(
        min(v[i] for v in vertices)
        for i in range(3)
    )

    mx = tuple(
        max(v[i] for v in vertices)
        for i in range(3)
    )

    center = tuple(
        (mn[i] + mx[i]) * 0.5
        for i in range(3)
    )

    size = tuple(
        mx[i] - mn[i]
        for i in range(3)
    )

    return mn, mx, center, size


reference_vertices = read_vertices(REFERENCE)
source_vertices = read_vertices(SOURCE)

ref_min, ref_max, ref_center, ref_size = bounds(
    reference_vertices
)

src_min, src_max, src_center, src_size = bounds(
    source_vertices
)

offset = tuple(
    ref_center[i] - src_center[i]
    for i in range(3)
)

print("=== HAIR LOCAL-SPACE ALIGNMENT ===")
print()
print("Reference:", REFERENCE)
print("Source   :", SOURCE)
print()
print(
    "Reference center:",
    tuple(round(x, 6) for x in ref_center),
)
print(
    "Source center   :",
    tuple(round(x, 6) for x in src_center),
)
print(
    "Translation     :",
    tuple(round(x, 6) for x in offset),
)

source_lines = SOURCE.read_text(
    encoding="utf-8",
    errors="replace",
).splitlines()

output_lines = []

changed = 0

for line in source_lines:
    m = VERTEX_RE.match(line)

    if not m:
        output_lines.append(line)
        continue

    x = float(m.group(2)) + offset[0]
    y = float(m.group(3)) + offset[1]
    z = float(m.group(4)) + offset[2]

    output_lines.append(
        f"{m.group(1)}"
        f"{x:.9f} "
        f"{y:.9f} "
        f"{z:.9f}"
        f"{m.group(5)}"
    )

    changed += 1

if changed != len(source_vertices):
    raise RuntimeError(
        f"Changed {changed}, expected "
        f"{len(source_vertices)} vertices."
    )

OUTPUT.write_text(
    "\n".join(output_lines) + "\n",
    encoding="utf-8",
)

aligned_vertices = read_vertices(OUTPUT)

a_min, a_max, a_center, a_size = bounds(
    aligned_vertices
)

print()
print("ALIGNED")
print(
    " min   :",
    tuple(round(x, 6) for x in a_min),
)
print(
    " max   :",
    tuple(round(x, 6) for x in a_max),
)
print(
    " center:",
    tuple(round(x, 6) for x in a_center),
)
print(
    " size  :",
    tuple(round(x, 6) for x in a_size),
)

for i in range(3):
    if abs(a_center[i] - ref_center[i]) > 1e-6:
        raise RuntimeError(
            "Center alignment failed."
        )

print()
print("Vertices changed:", changed)
print("Output:", OUTPUT)
print("SUCCESS")
