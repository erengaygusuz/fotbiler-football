#!/usr/bin/env python3

from pathlib import Path
import argparse
import re
import shutil


def extract_geomobject(text: str, node_name: str) -> str:
    pattern = re.compile(
        rf'\*GEOMOBJECT\s*\{{\s*'
        rf'\*NODE_NAME\s+"{re.escape(node_name)}".*?'
        rf'(?=\n\*GEOMOBJECT\s*\{{|\Z)',
        re.S
    )

    match = pattern.search(text)

    if not match:
        raise RuntimeError(
            f'GEOMOBJECT "{node_name}" bulunamadı.'
        )

    return match.group(0)


def parse_vertices(block: str):
    vertices = {}

    pattern = re.compile(
        r'\*MESH_VERTEX\s+(\d+)\s+'
        r'([-+0-9.eE]+)\s+'
        r'([-+0-9.eE]+)\s+'
        r'([-+0-9.eE]+)'
    )

    for m in pattern.finditer(block):
        index = int(m.group(1))
        vertices[index] = (
            float(m.group(2)),
            float(m.group(3)),
            float(m.group(4)),
        )

    return vertices


def parse_faces(block: str):
    faces = {}

    pattern = re.compile(
        r'\*MESH_FACE\s+(\d+):\s+'
        r'A:\s*(\d+)\s+'
        r'B:\s*(\d+)\s+'
        r'C:\s*(\d+)'
    )

    for m in pattern.finditer(block):
        face_index = int(m.group(1))
        faces[face_index] = (
            int(m.group(2)),
            int(m.group(3)),
            int(m.group(4)),
        )

    return faces


def parse_tvertices(block: str):
    tvertices = {}

    pattern = re.compile(
        r'\*MESH_TVERT\s+(\d+)\s+'
        r'([-+0-9.eE]+)\s+'
        r'([-+0-9.eE]+)\s+'
        r'([-+0-9.eE]+)'
    )

    for m in pattern.finditer(block):
        index = int(m.group(1))
        tvertices[index] = (
            float(m.group(2)),
            float(m.group(3)),
            float(m.group(4)),
        )

    return tvertices


def parse_tfaces(block: str):
    tfaces = {}

    pattern = re.compile(
        r'\*MESH_TFACE\s+(\d+)\s+'
        r'(\d+)\s+'
        r'(\d+)\s+'
        r'(\d+)'
    )

    for m in pattern.finditer(block):
        face_index = int(m.group(1))
        tfaces[face_index] = (
            int(m.group(2)),
            int(m.group(3)),
            int(m.group(4)),
        )

    return tfaces


def write_obj(
    output: Path,
    vertices,
    faces,
    tvertices,
    tfaces,
):
    mtl_name = output.with_suffix(".mtl").name

    lines = [
        "# Fotbiler Football legacy runtime head",
        "# Extracted from fullbody.ase",
        "",
        f"mtllib {mtl_name}",
        "o legacy_runtime_head",
        "",
    ]

    for index in sorted(vertices):
        x, y, z = vertices[index]
        lines.append(f"v {x:.9f} {y:.9f} {z:.9f}")

    lines.append("")

    for index in sorted(tvertices):
        u, v, _w = tvertices[index]
        lines.append(f"vt {u:.9f} {v:.9f}")

    lines.extend([
        "",
        "usemtl skin",
        "s 1",
        "",
    ])

    for face_index in sorted(faces):
        a, b, c = faces[face_index]

        if face_index in tfaces:
            ta, tb, tc = tfaces[face_index]

            lines.append(
                "f "
                f"{a + 1}/{ta + 1} "
                f"{b + 1}/{tb + 1} "
                f"{c + 1}/{tc + 1}"
            )
        else:
            lines.append(
                f"f {a + 1} {b + 1} {c + 1}"
            )

    output.write_text(
        "\n".join(lines) + "\n",
        encoding="utf-8"
    )


def write_mtl(output: Path, texture_filename: str | None):
    lines = [
        "newmtl skin",
        "Ka 0.200000 0.200000 0.200000",
        "Kd 1.000000 1.000000 1.000000",
        "Ks 0.020000 0.020000 0.020000",
        "Ns 10.000000",
    ]

    if texture_filename:
        lines.append(f"map_Kd {texture_filename}")

    output.write_text(
        "\n".join(lines) + "\n",
        encoding="utf-8"
    )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "input",
        type=Path,
        help="Input ASE file"
    )
    parser.add_argument(
        "output",
        type=Path,
        help="Output OBJ file"
    )

    args = parser.parse_args()

    text = args.input.read_text(
        encoding="utf-8",
        errors="replace"
    )

    block = extract_geomobject(text, "head")

    vertices = parse_vertices(block)
    faces = parse_faces(block)
    tvertices = parse_tvertices(block)
    tfaces = parse_tfaces(block)

    if not vertices:
        raise RuntimeError("Head vertices bulunamadı.")

    if not faces:
        raise RuntimeError("Head faces bulunamadı.")

    args.output.parent.mkdir(
        parents=True,
        exist_ok=True
    )

    write_obj(
        args.output,
        vertices,
        faces,
        tvertices,
        tfaces,
    )

    source_texture = (
        args.input.parent.parent
        / "textures"
        / "skin.jpg"
    )

    copied_texture = None

    if source_texture.exists():
        copied_texture = (
            args.output.parent
            / "legacy_skin.jpg"
        )

        shutil.copy2(
            source_texture,
            copied_texture
        )

    write_mtl(
        args.output.with_suffix(".mtl"),
        copied_texture.name if copied_texture else None
    )

    print("=== Fotbiler ASE Head Extract ===")
    print(f"Input       : {args.input}")
    print(f"Output      : {args.output}")
    print(f"Vertices    : {len(vertices)}")
    print(f"Faces       : {len(faces)}")
    print(f"UV vertices : {len(tvertices)}")
    print(f"UV faces    : {len(tfaces)}")

    if copied_texture:
        print(f"Texture     : {copied_texture}")
    else:
        print("Texture     : not copied")


if __name__ == "__main__":
    main()
