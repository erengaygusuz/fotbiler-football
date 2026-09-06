import bpy
import bmesh
from pathlib import Path

HUMAN_NAME = "MPFB_BaseHuman"
BODY_GROUP_NAME = "body"

# Legacy runtime head yüksekliği.
LEGACY_HEAD_HEIGHT = 0.295

human = bpy.data.objects.get(HUMAN_NAME)

if human is None or human.type != "MESH":
    raise RuntimeError(
        f"{HUMAN_NAME} bulunamadı."
    )

mesh = human.data

body_group = human.vertex_groups.get(
    BODY_GROUP_NAME
)

if body_group is None:
    raise RuntimeError(
        f"{BODY_GROUP_NAME} vertex group bulunamadı."
    )

# ------------------------------------------------------------
# Gerçek body surface vertexlerini belirle
# ------------------------------------------------------------

body_indices = set()

for vert in mesh.vertices:
    for membership in vert.groups:
        if membership.group == body_group.index:
            body_indices.add(vert.index)
            break

print()
print("=== MPFB BODY ===")
print("All vertices  :", len(mesh.vertices))
print("Body vertices :", len(body_indices))

if not body_indices:
    raise RuntimeError(
        "Body vertex group boş."
    )

body_z = [
    mesh.vertices[i].co.z
    for i in body_indices
]

max_z = max(body_z)

# MPFB Z-slice audit:
# 0.60 civarı dar boyun,
# 0.59 altında omuz genişlemesi başlıyor.
cut_z = 0.60

print("Body max Z    :", max_z)
print("Neck cut Z    :", cut_z)
print("Native height :", max_z - cut_z)

# ------------------------------------------------------------
# Mesh kopyası
# ------------------------------------------------------------

head_mesh = mesh.copy()

bm = bmesh.new()
bm.from_mesh(head_mesh)

bm.verts.ensure_lookup_table()
bm.verts.index_update()

to_delete = []

for vert in bm.verts:

    original_index = vert.index

    # Helper / joint geometry sil.
    if original_index not in body_indices:
        to_delete.append(vert)
        continue

    # Boynun altındaki her şeyi sil.
    if vert.co.z < cut_z:
        to_delete.append(vert)

bmesh.ops.delete(
    bm,
    geom=to_delete,
    context="VERTS"
)

# Loose geometry varsa temizle.
loose_verts = [
    v
    for v in bm.verts
    if not v.link_faces
]

if loose_verts:
    bmesh.ops.delete(
        bm,
        geom=loose_verts,
        context="VERTS"
    )

bm.verts.ensure_lookup_table()
bm.edges.ensure_lookup_table()
bm.faces.ensure_lookup_table()

# Neck opening gibi boundary edge sayısını ölç.
boundary_edges = [
    edge
    for edge in bm.edges
    if len(edge.link_faces) == 1
]

bm.to_mesh(head_mesh)
bm.free()

head_mesh.update()

# ------------------------------------------------------------
# Yeni object
# ------------------------------------------------------------

old = bpy.data.objects.get(
    "MPFB_FotbilerHead"
)

if old:
    bpy.data.objects.remove(
        old,
        do_unlink=True
    )

head = bpy.data.objects.new(
    "MPFB_FotbilerHead",
    head_mesh
)

bpy.context.collection.objects.link(head)

head.matrix_world = human.matrix_world.copy()

# Original full body'yi preview'da gizle.
human.hide_viewport = True
human.hide_render = True

# Smooth shading.
for poly in head_mesh.polygons:
    poly.use_smooth = True

# ------------------------------------------------------------
# Stats
# ------------------------------------------------------------

triangles = sum(
    max(1, len(poly.vertices) - 2)
    for poly in head_mesh.polygons
)

coords = [
    v.co
    for v in head_mesh.vertices
]

mins = (
    min(v.x for v in coords),
    min(v.y for v in coords),
    min(v.z for v in coords),
)

maxs = (
    max(v.x for v in coords),
    max(v.y for v in coords),
    max(v.z for v in coords),
)

print()
print("=== MPFB FOTBILER HEAD ===")
print("Vertices       :", len(head_mesh.vertices))
print("Polygons       :", len(head_mesh.polygons))
print("Triangles      :", triangles)
print("Boundary edges :", len(boundary_edges))

print(
    "UV layers      :",
    [uv.name for uv in head_mesh.uv_layers]
)

print(
    "Bounds min     :",
    tuple(round(x, 6) for x in mins)
)

print(
    "Bounds max     :",
    tuple(round(x, 6) for x in maxs)
)

print(
    "Dimensions     :",
    tuple(
        round(x, 6)
        for x in head.dimensions
    )
)

# ------------------------------------------------------------
# Save
# ------------------------------------------------------------

root = Path.cwd()

output = (
    root
    / "character_work"
    / "mpfb"
    / "mpfb_head.blend"
)

bpy.ops.wm.save_as_mainfile(
    filepath=str(output)
)

print()
print("Saved:", output)
print("SUCCESS")
