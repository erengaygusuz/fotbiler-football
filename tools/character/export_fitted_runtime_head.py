import bpy
import bmesh
import json
from pathlib import Path

HEAD_NAME = "MPFB_FotbilerHead_Fitted"
EYE_NAMES = (
    "MPFB_Eye_Left_Fitted",
    "MPFB_Eye_Right_Fitted",
)

OUTPUT = (
    Path.cwd()
    / "character_work"
    / "mpfb"
    / "export"
    / "mpfb_fitted_runtime_head.json"
)


def get_mesh_object(name):
    obj = bpy.data.objects.get(name)

    if obj is None or obj.type != "MESH":
        raise RuntimeError(f"{name} bulunamadı.")

    return obj


head = get_mesh_object(HEAD_NAME)
eyes = [get_mesh_object(name) for name in EYE_NAMES]
sources = [head] + eyes

print()
print("=== SOURCE OBJECTS ===")

for obj in sources:
    print(
        obj.name,
        "verts =", len(obj.data.vertices),
        "faces =", len(obj.data.polygons)
    )

# ------------------------------------------------------------
# Duplicate objects so original blend stays unchanged
# ------------------------------------------------------------

collection = bpy.context.collection
dups = []

for src in sources:
    dup = src.copy()
    dup.data = src.data.copy()
    collection.objects.link(dup)
    dups.append(dup)

bpy.ops.object.select_all(action="DESELECT")

for obj in dups:
    obj.select_set(True)

bpy.context.view_layer.objects.active = dups[0]

# Join duplicates into one mesh object.
bpy.ops.object.join()

joined = bpy.context.view_layer.objects.active
joined.name = "MPFB_RuntimeHead_Merged"
joined.data.name = "MPFB_RuntimeHead_Merged_Mesh"

# Apply transforms just in case.
bpy.ops.object.transform_apply(
    location=True,
    rotation=True,
    scale=True
)

mesh = joined.data

# ------------------------------------------------------------
# Triangulate
# ------------------------------------------------------------

bm = bmesh.new()
bm.from_mesh(mesh)

bmesh.ops.triangulate(
    bm,
    faces=list(bm.faces)
)

bm.to_mesh(mesh)
bm.free()

mesh.update()

# ------------------------------------------------------------
# Export geometry
# ------------------------------------------------------------

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
    verts = list(poly.vertices)

    if len(verts) != 3:
        raise RuntimeError(
            f"Triangulation başarısız: {len(verts)} vertex face"
        )

    faces.append([
        int(verts[0]),
        int(verts[1]),
        int(verts[2]),
    ])

# ------------------------------------------------------------
# Export UV preserving active UV map
# ------------------------------------------------------------

uv_layer = mesh.uv_layers.active

if uv_layer is None:
    raise RuntimeError("Active UV layer bulunamadı.")

uv_lookup = {}
uv_vertices = []
uv_faces = []

def uv_key(u, v):
    return (round(float(u), 8), round(float(v), 8))

for poly in mesh.polygons:
    face_uv = []

    for loop_index in poly.loop_indices:
        uv = uv_layer.data[loop_index].uv
        key = uv_key(uv.x, uv.y)

        if key not in uv_lookup:
            uv_lookup[key] = len(uv_vertices)
            uv_vertices.append([
                float(uv.x),
                float(uv.y),
                0.0,
            ])

        face_uv.append(uv_lookup[key])

    if len(face_uv) != 3:
        raise RuntimeError("UV face triangle değil.")

    uv_faces.append(face_uv)

if len(uv_faces) != len(faces):
    raise RuntimeError(
        "UV face count ile geometry face count eşleşmiyor."
    )

# ------------------------------------------------------------
# Bounds / stats
# ------------------------------------------------------------

xs = [v[0] for v in vertices]
ys = [v[1] for v in vertices]
zs = [v[2] for v in vertices]

data = {
    "format": "fotbiler-runtime-head-v2",
    "source": {
        "head_object": HEAD_NAME,
        "eye_objects": list(EYE_NAMES),
        "uv_layer": uv_layer.name,
    },

    "vertices": vertices,
    "faces": faces,

    "uv_vertices": uv_vertices,
    "uv_faces": uv_faces,

    "skin_binding": {
        "joint_id": 2,
        "weight": 1.0,
        "ase_vertex_color": [0.114, 0.0, 0.0]
    },

    "material_id": 1,

    "bounds": {
        "min": [min(xs), min(ys), min(zs)],
        "max": [max(xs), max(ys), max(zs)],
    }
}

OUTPUT.parent.mkdir(parents=True, exist_ok=True)

OUTPUT.write_text(
    json.dumps(data, indent=2),
    encoding="utf-8"
)

triangles = len(faces)

print()
print("=== RUNTIME HEAD EXPORT ===")
print("Merged object :", joined.name)
print("Vertices      :", len(vertices))
print("Triangles     :", triangles)
print("UV vertices   :", len(uv_vertices))
print("UV faces      :", len(uv_faces))
print("UV layer      :", uv_layer.name)
print("Bounds min    :", data["bounds"]["min"])
print("Bounds max    :", data["bounds"]["max"])
print("Output        :", OUTPUT)
print("SUCCESS")
