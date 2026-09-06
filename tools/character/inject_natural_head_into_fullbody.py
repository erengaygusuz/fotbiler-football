#!/usr/bin/env python3

import json
import math
import re
import sys
from pathlib import Path


# ------------------------------------------------------------
# Balanced ASE blocks
# ------------------------------------------------------------

def find_balanced_block(text, start):
    open_pos = text.find("{", start)

    if open_pos == -1:
        raise ValueError("Opening brace bulunamadı.")

    depth = 0

    for i in range(open_pos, len(text)):

        if text[i] == "{":
            depth += 1

        elif text[i] == "}":
            depth -= 1

            if depth == 0:
                return start, i + 1

    raise ValueError("Balanced block kapanmadı.")


def find_head_geomobject(text):
    for match in re.finditer(r"\*GEOMOBJECT\b", text):

        start, end = find_balanced_block(
            text,
            match.start()
        )

        block = text[start:end]

        if re.search(
            r'\*NODE_NAME\s+"head"',
            block
        ):
            return start, end, block

    raise RuntimeError(
        'GEOMOBJECT NODE_NAME "head" bulunamadı.'
    )


def find_mesh_block(geom):
    match = re.search(r"\*MESH\s*\{", geom)

    if not match:
        raise RuntimeError(
            "Head GEOMOBJECT içinde MESH bulunamadı."
        )

    return find_balanced_block(
        geom,
        match.start()
    )


# ------------------------------------------------------------
# Vector math
# ------------------------------------------------------------

def sub(a, b):
    return (
        a[0] - b[0],
        a[1] - b[1],
        a[2] - b[2],
    )


def add(a, b):
    return (
        a[0] + b[0],
        a[1] + b[1],
        a[2] + b[2],
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


def calculate_normals(vertices, faces):
    face_normals = []
    accum = [
        (0.0, 0.0, 0.0)
        for _ in vertices
    ]

    for a, b, c in faces:
        va = vertices[a]
        vb = vertices[b]
        vc = vertices[c]

        ab = sub(vb, va)
        ac = sub(vc, va)

        normal = normalize(
            cross(ab, ac)
        )

        face_normals.append(normal)

        accum[a] = add(accum[a], normal)
        accum[b] = add(accum[b], normal)
        accum[c] = add(accum[c], normal)

    vertex_normals = [
        normalize(v)
        for v in accum
    ]

    return face_normals, vertex_normals


def fmt(value):
    if abs(value) < 0.0000005:
        value = 0.0

    return f"{value:.6f}"


# ------------------------------------------------------------
# Validation
# ------------------------------------------------------------

def validate_data(data):
    if data.get("format") != "fotbiler-runtime-head-v2":
        raise RuntimeError(
            "Beklenmeyen JSON formatı."
        )

    vertices = data["vertices"]
    faces = data["faces"]

    uv_vertices = data["uv_vertices"]
    uv_faces = data["uv_faces"]

    if not vertices:
        raise RuntimeError("Vertex listesi boş.")

    if not faces:
        raise RuntimeError("Face listesi boş.")

    if len(faces) != len(uv_faces):
        raise RuntimeError(
            "Geometry/UV face count eşleşmiyor."
        )

    for face in faces:
        if len(face) != 3:
            raise RuntimeError(
                "Geometry face triangle değil."
            )

        for index in face:
            if index < 0 or index >= len(vertices):
                raise RuntimeError(
                    f"Geçersiz vertex index: {index}"
                )

    for face in uv_faces:
        if len(face) != 3:
            raise RuntimeError(
                "UV face triangle değil."
            )

        for index in face:
            if index < 0 or index >= len(uv_vertices):
                raise RuntimeError(
                    f"Geçersiz UV index: {index}"
                )

    binding = data["skin_binding"]

    if binding["joint_id"] != 2:
        raise RuntimeError(
            "İlk runtime test için joint_id 2 bekleniyor."
        )

    if abs(binding["weight"] - 1.0) > 1e-6:
        raise RuntimeError(
            "İlk runtime test için weight 1.0 bekleniyor."
        )


# ------------------------------------------------------------
# ASE MESH writer
# ------------------------------------------------------------

def build_mesh_block(data):
    vertices = [
        tuple(map(float, v))
        for v in data["vertices"]
    ]

    faces = [
        tuple(map(int, f))
        for f in data["faces"]
    ]

    uv_vertices = [
        tuple(map(float, uv))
        for uv in data["uv_vertices"]
    ]

    uv_faces = [
        tuple(map(int, f))
        for f in data["uv_faces"]
    ]

    material_id = int(
        data.get("material_id", 1)
    )

    color = tuple(
        map(
            float,
            data["skin_binding"]["ase_vertex_color"]
        )
    )

    face_normals, vertex_normals = (
        calculate_normals(
            vertices,
            faces
        )
    )

    out = []

    out.append("\t*MESH {")
    out.append("\t\t*TIMEVALUE 0")

    # --------------------------------------------------------
    # Geometry
    # --------------------------------------------------------

    out.append(
        f"\t\t*MESH_NUMVERTEX {len(vertices)}"
    )

    out.append(
        f"\t\t*MESH_NUMFACES {len(faces)}"
    )

    out.append("\t\t*MESH_VERTEX_LIST {")

    for i, (x, y, z) in enumerate(vertices):
        out.append(
            "\t\t\t*MESH_VERTEX "
            f"{i} "
            f"{fmt(x)} "
            f"{fmt(y)} "
            f"{fmt(z)}"
        )

    out.append("\t\t}")

    out.append("\t\t*MESH_FACE_LIST {")

    for i, (a, b, c) in enumerate(faces):
        out.append(
            "\t\t\t*MESH_FACE "
            f"{i}: "
            f"A: {a} "
            f"B: {b} "
            f"C: {c} "
            "AB: 1 "
            "BC: 1 "
            "CA: 1 "
            "*MESH_SMOOTHING 1 "
            f"*MESH_MTLID {material_id}"
        )

    out.append("\t\t}")

    # --------------------------------------------------------
    # UV
    # --------------------------------------------------------

    out.append(
        f"\t\t*MESH_NUMTVERTEX {len(uv_vertices)}"
    )

    out.append("\t\t*MESH_TVERTLIST {")

    for i, (u, v, w) in enumerate(uv_vertices):
        out.append(
            "\t\t\t*MESH_TVERT "
            f"{i} "
            f"{fmt(u)} "
            f"{fmt(v)} "
            f"{fmt(w)}"
        )

    out.append("\t\t}")

    out.append(
        f"\t\t*MESH_NUMTVFACES {len(uv_faces)}"
    )

    out.append("\t\t*MESH_TFACELIST {")

    for i, (a, b, c) in enumerate(uv_faces):
        out.append(
            "\t\t\t*MESH_TFACE "
            f"{i} {a} {b} {c}"
        )

    out.append("\t\t}")

    # --------------------------------------------------------
    # Vertex colors
    #
    # Existing Fotbiler CPU skinning decodes these:
    # 0.114 * 255 ~= 29
    # joint = floor(29 * 0.1) = 2
    # weight = (29 - 20) / 9 = 1
    # --------------------------------------------------------

    out.append(
        f"\t\t*MESH_NUMCVERTEX {len(vertices)}"
    )

    out.append("\t\t*MESH_CVERTLIST {")

    for i in range(len(vertices)):
        out.append(
            "\t\t\t*MESH_VERTCOL "
            f"{i} "
            f"{fmt(color[0])} "
            f"{fmt(color[1])} "
            f"{fmt(color[2])}"
        )

    out.append("\t\t}")

    out.append(
        f"\t\t*MESH_NUMCVFACES {len(faces)}"
    )

    out.append("\t\t*MESH_CFACELIST {")

    for i, (a, b, c) in enumerate(faces):
        out.append(
            "\t\t\t*MESH_CFACE "
            f"{i} {a} {b} {c}"
        )

    out.append("\t\t}")

    # --------------------------------------------------------
    # Normals
    #
    # Keep this AFTER vertex colors.
    # Existing GetVertexColors() uses MESH_NORMALS as useful
    # color-block termination marker.
    # --------------------------------------------------------

    out.append("\t\t*MESH_NORMALS {")

    for i, face in enumerate(faces):
        fn = face_normals[i]

        out.append(
            "\t\t\t*MESH_FACENORMAL "
            f"{i} "
            f"{fmt(fn[0])} "
            f"{fmt(fn[1])} "
            f"{fmt(fn[2])}"
        )

        for vertex_index in face:
            vn = vertex_normals[vertex_index]

            out.append(
                "\t\t\t*MESH_VERTEXNORMAL "
                f"{vertex_index} "
                f"{fmt(vn[0])} "
                f"{fmt(vn[1])} "
                f"{fmt(vn[2])}"
            )

    out.append("\t\t}")

    out.append("\t}")

    return "\n".join(out)


# ------------------------------------------------------------
# Main
# ------------------------------------------------------------

def main():
    if len(sys.argv) != 4:
        print(
            "Usage:\n"
            "  inject_mpfb_head_into_fullbody.py "
            "<source fullbody.ase> "
            "<runtime head json> "
            "<output fullbody.ase>"
        )

        raise SystemExit(2)

    source_path = Path(sys.argv[1])
    json_path = Path(sys.argv[2])
    output_path = Path(sys.argv[3])

    source_text = source_path.read_text(
        encoding="utf-8",
        errors="replace"
    )

    data = json.loads(
        json_path.read_text(
            encoding="utf-8"
        )
    )

    validate_data(data)

    geom_start, geom_end, geom = (
        find_head_geomobject(
            source_text
        )
    )

    mesh_start, mesh_end = (
        find_mesh_block(
            geom
        )
    )

    new_mesh = build_mesh_block(data)

    # Preserve NODE_TM, MATERIAL_REF and all other outer
    # GEOMOBJECT data. Replace only MESH.
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

    output_path.parent.mkdir(
        parents=True,
        exist_ok=True
    )

    output_path.write_text(
        output_text,
        encoding="utf-8"
    )

    print()
    print("=== FOTBILER MPFB ASE INJECTOR ===")
    print(
        "Vertices     :",
        len(data["vertices"])
    )
    print(
        "Faces        :",
        len(data["faces"])
    )
    print(
        "UV vertices  :",
        len(data["uv_vertices"])
    )
    print(
        "UV faces     :",
        len(data["uv_faces"])
    )
    print(
        "Joint        :",
        data["skin_binding"]["joint_id"]
    )
    print(
        "Weight       :",
        data["skin_binding"]["weight"]
    )
    print(
        "Vertex color :",
        tuple(
            data["skin_binding"]["ase_vertex_color"]
        )
    )
    print(
        "Material ID  :",
        data.get("material_id", 1)
    )
    print(
        "Output       :",
        output_path
    )
    print("SUCCESS")


if __name__ == "__main__":
    main()
