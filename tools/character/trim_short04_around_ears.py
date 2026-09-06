import bpy
import bmesh
from pathlib import Path
from mathutils import Vector

ROOT = Path("/run/media/ereng/files/projects/fotbiler-football")

OUT = (
    ROOT
    / "character_work/asset_packs/work"
    / "mpfb_fitted_short04_eartrim_preview.blend"
)

HEAD_NAME = "Fotbiler_Runtime_Head_Reference"
HAIR_NAME = "Fotbiler_Runtime_Hair_Short04"

head = bpy.data.objects.get(HEAD_NAME)
hair = bpy.data.objects.get(HAIR_NAME)

if head is None:
    raise RuntimeError(f"Missing head: {HEAD_NAME}")

if hair is None:
    raise RuntimeError(f"Missing hair: {HAIR_NAME}")


def world_vertices(obj):
    return [
        obj.matrix_world @ v.co
        for v in obj.data.vertices
    ]


def bounds(points):
    mn = Vector((
        min(p.x for p in points),
        min(p.y for p in points),
        min(p.z for p in points),
    ))

    mx = Vector((
        max(p.x for p in points),
        max(p.y for p in points),
        max(p.z for p in points),
    ))

    return mn, mx


def triangles(obj):
    return sum(
        max(0, len(p.vertices) - 2)
        for p in obj.data.polygons
    )


# ------------------------------------------------------------
# Head bounds
# ------------------------------------------------------------

hp = world_vertices(head)
mn, mx = bounds(hp)

size = mx - mn

print("=== HEAD ===")
print("min :", tuple(round(v, 6) for v in mn))
print("max :", tuple(round(v, 6) for v in mx))
print("size:", tuple(round(v, 6) for v in size))


# ------------------------------------------------------------
# Estimate ear positions from outermost head geometry.
#
# Ear vertices should be among the widest vertices of the head
# in the middle vertical region.
# ------------------------------------------------------------

z_low = mn.z + size.z * 0.30
z_high = mn.z + size.z * 0.68

left_candidates = [
    p for p in hp
    if p.x <= mn.x + size.x * 0.10
    and z_low <= p.z <= z_high
]

right_candidates = [
    p for p in hp
    if p.x >= mx.x - size.x * 0.10
    and z_low <= p.z <= z_high
]


def average(points):
    if not points:
        raise RuntimeError("Could not detect ear vertices.")

    return sum(points, Vector()) / len(points)


left_center = average(left_candidates)
right_center = average(right_candidates)

print()
print("=== DETECTED EARS ===")
print(
    "left :",
    tuple(round(v, 6) for v in left_center),
    "verts:",
    len(left_candidates),
)
print(
    "right:",
    tuple(round(v, 6) for v in right_center),
    "verts:",
    len(right_candidates),
)


# ------------------------------------------------------------
# Ear exclusion ellipsoids.
#
# Deliberately slightly larger than the ears:
# the remaining tiny sideburn region will later live in the
# face/hairline texture instead of expensive geometry.
# ------------------------------------------------------------

radius_x = size.x * 0.115
radius_y = size.y * 0.20
radius_z = size.z * 0.17

print()
print("Ear clear radii:")
print(" x:", round(radius_x, 6))
print(" y:", round(radius_y, 6))
print(" z:", round(radius_z, 6))


def inside_ear_guard(p, center):
    dx = (p.x - center.x) / radius_x
    dy = (p.y - center.y) / radius_y
    dz = (p.z - center.z) / radius_z

    return dx * dx + dy * dy + dz * dz <= 1.0


# ------------------------------------------------------------
# Duplicate runtime hair
# ------------------------------------------------------------

trimmed = hair.copy()
trimmed.data = hair.data.copy()
trimmed.name = "Fotbiler_Runtime_Hair_Short04_EarTrim"

bpy.context.collection.objects.link(trimmed)

hair.hide_viewport = True
hair.hide_render = True


# ------------------------------------------------------------
# Remove triangles entering either ear guard.
#
# A face is removed when:
# - its center enters the ear volume, OR
# - at least two of its vertices enter the ear volume.
#
# This retains significantly more topology than deleting every
# triangle with a single grazing vertex.
# ------------------------------------------------------------

bm = bmesh.new()
bm.from_mesh(trimmed.data)

bm.faces.ensure_lookup_table()
bm.verts.ensure_lookup_table()

mw = trimmed.matrix_world

remove_faces = []

for face in bm.faces:
    points = [
        mw @ v.co
        for v in face.verts
    ]

    center = sum(points, Vector()) / len(points)

    remove = False

    for ear_center in (left_center, right_center):
        inside_count = sum(
            1
            for p in points
            if inside_ear_guard(p, ear_center)
        )

        if inside_ear_guard(center, ear_center) or inside_count >= 2:
            remove = True
            break

    if remove:
        remove_faces.append(face)


before_faces = len(bm.faces)

bmesh.ops.delete(
    bm,
    geom=remove_faces,
    context='FACES'
)

bm.to_mesh(trimmed.data)
bm.free()

trimmed.data.update()

after_faces = len(trimmed.data.polygons)

print()
print("=== EAR TRIM ===")
print("faces before :", before_faces)
print("faces removed:", len(remove_faces))
print("faces after  :", after_faces)
print("triangles    :", triangles(trimmed))


# ------------------------------------------------------------
# Select result
# ------------------------------------------------------------

for obj in bpy.context.selected_objects:
    obj.select_set(False)

trimmed.select_set(True)
head.select_set(True)

bpy.context.view_layer.objects.active = trimmed


# ------------------------------------------------------------
# Save non-destructively
# ------------------------------------------------------------

OUT.parent.mkdir(parents=True, exist_ok=True)

bpy.ops.wm.save_as_mainfile(
    filepath=str(OUT)
)

print()
print("SAVED:", OUT)
print("SUCCESS")
