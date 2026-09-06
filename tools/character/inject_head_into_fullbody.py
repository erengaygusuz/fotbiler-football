#!/usr/bin/env python3

import argparse
import json
import math
import re
from pathlib import Path


def find_balanced_block(text, start):
    """
    start must point to the beginning of something containing an opening {.
    Returns (start, end_exclusive).
    """
    open_pos = text.find("{", start)

    if open_pos < 0:
        raise RuntimeError("Opening brace bulunamadı.")

    depth = 0

    for i in range(open_pos, len(text)):
        ch = text[i]

        if ch == "{":
            depth += 1

        elif ch == "}":
            depth -= 1

            if depth == 0:
                return start, i + 1

    raise RuntimeError("Balanced block kapanışı bulunamadı.")


def find_head_geomobject(text):
    pos = 0

    while True:
        start = text.find("*GEOMOBJECT", pos)

        if start < 0:
            break

        block_start, block_end = find_balanced_block(
            text,
            start
        )

        block = text[block_start:block_end]

        # Direct GEOMOBJECT name occurs before NODE_TM.
        head_match = re.search(
            r'^\s*\*NODE_NAME\s+"head"\s*$',
            block,
            re.M
        )

        if head_match:
            return block_start, block_end, block

        pos = block_end

    raise RuntimeError(
        'GEOMOBJECT NODE_NAME "head" bulunamadı.'
    )


def find_mesh_block(geom_block):
    match = re.search(
        r'^\s*\*MESH\s*\{',
        geom_block,
        re.M
    )

    if not match:
        raise RuntimeError(
            "Head GEOMOBJECT içinde MESH bulunamadı."
        )

    start = match.start()

    _, end = find_balanced_block(
        geom_block,
        start
    )

    return start, end


def sub(a, b):
    return (
        a[0] - b[0],
        a[1] - b[1],
        a[2] - b[2],
    )


def cross(a, b):
    return (
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    )


def normalize(v):
    length = math.sqrt(
        v[0] * v[0]
        + v[1] * v[1]
        + v[2] * v[2]
    )

    if length < 1e-12:
        return (0.0, 0.0, 1.0)

    return (
        v[0] / length,
        v[1] / length,
        v[2] / length,
    )


def add(a, b):
    return (
        a[0] + b[0],
        a[1] + b[1],
        a[2] + b[2],
    )


def calculate_normals(vertices, faces):
    face_normals = []
    accum = [
        (0.0, 0.0, 0.0)
        for _ in vertices
    ]

    for a, b, c in faces:
        ab = sub(vertices[b], vertices[a])
        ac = sub(vertices[c], vertices[a])

        n = normalize(cross(ab, ac))

        face_normals.append(n)

        accum[a] = add(accum[a], n)
        accum[b] = add(accum[b], n)
        accum[c] = add(accum[c], n)

    vertex_normals = [
        normalize(v)
        for v in accum
    ]

    return face_normals, vertex_normals


def make_planar_uv(vertices):
    """
    Temporary technical-PoC UV.

    U = X across head width
    V = Z across head height

    This is NOT our final face UV.
    """

    xs = [v[0] for v in vertices]
    zs = [v[2] for v in vertices]

    min_x = min(xs)
    max_x = max(xs)

    min_z = min(zs)
    max_z = max(zs)

    dx = max_x - min_x
    dz = max_z - min_z

    if dx <= 0 or dz <= 0:
        raise RuntimeError("Geçersiz head bounds.")

    result = []

    for x, _y, z in vertices:
        u = (x - min_x) / dx
        v = (z - min_z) / dz

        result.append((u, v, 0.0))

    return result


def fmt(v):
    return f"{v:.6f}"


def build_mesh_block(
    vertices,
    faces,
    color,
    material_id,
):
    face_normals, vertex_normals = (
        calculate_normals(
            vertices,
            faces
        )
    )

    uvs = make_planar_uv(vertices)

    lines = []

    lines.append("\t*MESH {")
    lines.append("\t\t*TIMEVALUE 0")
    lines.append(
        f"\t\t*MESH_NUMVERTEX {len(vertices)}"
    )
    lines.append(
        f"\t\t*MESH_NUMFACES {len(faces)}"
    )

    # --------------------------------------------------------
    # Vertices
    # --------------------------------------------------------

    lines.append("\t\t*MESH_VERTEX_LIST {")

    for i, (x, y, z) in enumerate(vertices):
        lines.append(
            f"\t\t\t*MESH_VERTEX {i:4d}"
            f"\t{fmt(x)}"
            f"\t{fmt(y)}"
            f"\t{fmt(z)}"
        )

    lines.append("\t\t}")

    # --------------------------------------------------------
    # Faces
    # --------------------------------------------------------

    lines.append("\t\t*MESH_FACE_LIST {")

    for i, (a, b, c) in enumerate(faces):
        lines.append(
            f"\t\t\t*MESH_FACE {i:4d}:"
            f"    A: {a:4d}"
            f" B: {b:4d}"
            f" C: {c:4d}"
            f" AB:    1"
            f" BC:    1"
            f" CA:    1"
            f"\t*MESH_SMOOTHING 1"
            f"\t*MESH_MTLID {material_id}"
        )

    lines.append("\t\t}")

    # --------------------------------------------------------
    # Temporary UV
    # --------------------------------------------------------

    lines.append(
        f"\t\t*MESH_NUMTVERTEX {len(uvs)}"
    )

    lines.append("\t\t*MESH_TVERTLIST {")

    for i, (u, v, w) in enumerate(uvs):
        lines.append(
            f"\t\t\t*MESH_TVERT {i:4d}"
            f"\t{fmt(u)}"
            f"\t{fmt(v)}"
            f"\t{fmt(w)}"
        )

    lines.append("\t\t}")

    lines.append(
        f"\t\t*MESH_NUMTVFACES {len(faces)}"
    )

    lines.append("\t\t*MESH_TFACELIST {")

    # One UV per geometric vertex for this PoC.
    for i, (a, b, c) in enumerate(faces):
        lines.append(
            f"\t\t\t*MESH_TFACE {i}"
            f"\t{a}"
            f"\t{b}"
            f"\t{c}"
        )

    lines.append("\t\t}")

    # --------------------------------------------------------
    # Vertex colors = CPU skin binding
    # --------------------------------------------------------

    lines.append(
        f"\t\t*MESH_NUMCVERTEX {len(vertices)}"
    )

    lines.append("\t\t*MESH_CVERTLIST {")

    r, g, b = color

    for i in range(len(vertices)):
        lines.append(
            f"\t\t\t*MESH_VERTCOL {i}"
            f"\t{fmt(r)}"
            f"\t{fmt(g)}"
            f"\t{fmt(b)}"
        )

    lines.append("\t\t}")

    lines.append(
        f"\t\t*MESH_NUMCVFACES {len(faces)}"
    )

    lines.append("\t\t*MESH_CFACELIST {")

    for i, (a, b, c) in enumerate(faces):
        lines.append(
            f"\t\t\t*MESH_CFACE {i}"
            f"\t{a}"
            f"\t{b}"
            f"\t{c}"
        )

    lines.append("\t\t}")

    # --------------------------------------------------------
    # Normals
    #
    # Important: GetVertexColors() treats MESH_NORMALS as
    # the end of the useful vertex-color block.
    # --------------------------------------------------------

    lines.append("\t\t*MESH_NORMALS {")

    for i, (a, b, c) in enumerate(faces):
        fn = face_normals[i]

        lines.append(
            f"\t\t\t*MESH_FACENORMAL {i}"
            f"\t{fmt(fn[0])}"
            f"\t{fmt(fn[1])}"
            f"\t{fmt(fn[2])}"
        )

        for vertex_index in (a, b, c):
            vn = vertex_normals[vertex_index]

            lines.append(
                f"\t\t\t\t*MESH_VERTEXNORMAL "
                f"{vertex_index}"
                f"\t{fmt(vn[0])}"
                f"\t{fmt(vn[1])}"
                f"\t{fmt(vn[2])}"
            )

    lines.append("\t\t}")

    lines.append("\t}")

    return "\n".join(lines)


def validate_data(data):
    vertices = [
        tuple(map(float, v))
        for v in data["vertices"]
    ]

    faces = [
        tuple(map(int, f))
        for f in data["faces"]
    ]

    if not vertices:
        raise RuntimeError("Vertex listesi boş.")

    if not faces:
        raise RuntimeError("Face listesi boş.")

    n = len(vertices)

    for i, face in enumerate(faces):
        if len(face) != 3:
            raise RuntimeError(
                f"Face {i} triangle değil."
            )

        for index in face:
            if index < 0 or index >= n:
                raise RuntimeError(
                    f"Face {i}: geçersiz vertex index {index}"
                )

    return vertices, faces


def main():
    parser = argparse.ArgumentParser()

    parser.add_argument(
        "fullbody",
        type=Path
    )

    parser.add_argument(
        "head_json",
        type=Path
    )

    parser.add_argument(
        "output",
        type=Path
    )

    args = parser.parse_args()

    source_text = args.fullbody.read_text(
        encoding="utf-8",
        errors="replace"
    )

    data = json.loads(
        args.head_json.read_text(
            encoding="utf-8"
        )
    )

    vertices, faces = validate_data(data)

    binding = data["skin_binding"]

    color = tuple(
        float(v)
        for v in binding["ase_vertex_color"]
    )

    material_id = int(
        data.get("material_id", 1)
    )

    # Locate original head GEOMOBJECT.
    geom_start, geom_end, geom = (
        find_head_geomobject(source_text)
    )

    mesh_start, mesh_end = (
        find_mesh_block(geom)
    )

    # Preserve original NODE_TM, properties and MATERIAL_REF.
    new_mesh = build_mesh_block(
        vertices,
        faces,
        color,
        material_id,
    )

    new_geom = (
        geom[:mesh_start]
        + new_mesh
        + geom[mesh_end:]
    )

    output_text = (
        source_text[:geom_start]
        + new_geom
        + source_text[geom_end:]
    )

    args.output.parent.mkdir(
        parents=True,
        exist_ok=True
    )

    args.output.write_text(
        output_text,
        encoding="utf-8"
    )

    print()
    print("=== FOTBILER ASE HEAD INJECTOR ===")
    print("Source       :", args.fullbody)
    print("Head JSON    :", args.head_json)
    print("Output       :", args.output)
    print("Vertices     :", len(vertices))
    print("Faces        :", len(faces))
    print("Joint        :", binding["joint_id"])
    print("Weight       :", binding["weight"])
    print("Vertex color :", color)
    print("Material ID  :", material_id)
    print("SUCCESS")


if __name__ == "__main__":
    main()
