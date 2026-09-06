import bpy
import bmesh
import json
from pathlib import Path

OBJECT_NAME = "Fotbiler_BaseHead_POC"

obj = bpy.data.objects.get(OBJECT_NAME)

if obj is None or obj.type != "MESH":
    raise RuntimeError(f"{OBJECT_NAME} bulunamadı.")

root = Path.cwd()

output = (
    root
    / "character_work"
    / "export"
    / "fotbiler_base_head_poc.json"
)

output.parent.mkdir(
    parents=True,
    exist_ok=True
)

# Mesh'i kopyala. Orijinal .blend değişmeyecek.
mesh = obj.data.copy()

bm = bmesh.new()
bm.from_mesh(mesh)

# ASE runtime mesh triangle istiyor.
bmesh.ops.triangulate(
    bm,
    faces=list(bm.faces)
)

bm.to_mesh(mesh)
bm.free()

mesh.update()

vertices = [
    [
        float(v.co.x),
        float(v.co.y),
        float(v.co.z),
    ]
    for v in mesh.vertices
]

faces = []

for poly in mesh.polygons:
    if len(poly.vertices) != 3:
        raise RuntimeError(
            f"Triangulation başarısız: {len(poly.vertices)} vertex face"
        )

    faces.append([
        int(poly.vertices[0]),
        int(poly.vertices[1]),
        int(poly.vertices[2]),
    ])

xs = [v[0] for v in vertices]
ys = [v[1] for v in vertices]
zs = [v[2] for v in vertices]

data = {
    "format": "fotbiler-head-poc-v1",
    "coordinate_space": "ASE-local",
    "object": OBJECT_NAME,

    "vertices": vertices,
    "faces": faces,

    # Legacy runtime kafa tamamen joint 2'ye bağlı.
    "skin_binding": {
        "joint_id": 2,
        "weight": 1.0,
        "ase_vertex_color": [
            0.114,
            0.0,
            0.0
        ]
    },

    # Existing fullbody head uses skin material.
    "material_id": 1,

    "bounds": {
        "min": [
            min(xs),
            min(ys),
            min(zs),
        ],
        "max": [
            max(xs),
            max(ys),
            max(zs),
        ]
    }
}

output.write_text(
    json.dumps(
        data,
        indent=2
    ),
    encoding="utf-8"
)

print()
print("=== FOTBILER HEAD EXPORT ===")
print("Object   :", OBJECT_NAME)
print("Vertices :", len(vertices))
print("Faces    :", len(faces))
print("Output   :", output)
print("SUCCESS")

bpy.data.meshes.remove(mesh)
