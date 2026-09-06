import bpy
from mathutils import Vector
from pathlib import Path

ROOT = Path("/run/media/ereng/files/projects/fotbiler-football")

HAIR_NAME = "Fotbiler_Runtime_Hair_Short01"
HEAD_NAME = "Fotbiler_Runtime_Head_Reference"

OUT = (
    ROOT
    / "character_work/asset_packs/work"
    / "mpfb_fitted_short01_clearance_preview.blend"
)

hair = bpy.data.objects.get(HAIR_NAME)
head = bpy.data.objects.get(HEAD_NAME)

if hair is None:
    raise RuntimeError(f"Missing {HAIR_NAME}")

if head is None:
    raise RuntimeError(f"Missing {HEAD_NAME}")


def bounds(obj):
    pts = [
        obj.matrix_world @ v.co
        for v in obj.data.vertices
    ]

    mn = Vector((
        min(p.x for p in pts),
        min(p.y for p in pts),
        min(p.z for p in pts),
    ))

    mx = Vector((
        max(p.x for p in pts),
        max(p.y for p in pts),
        max(p.z for p in pts),
    ))

    return mn, mx


hmn, hmx = bounds(head)

center = Vector((
    (hmn.x + hmx.x) * 0.5,
    (hmn.y + hmx.y) * 0.5,
    (hmn.z + hmx.z) * 0.5,
))

print("Head center:", tuple(round(v, 6) for v in center))


# Preserve original.
result = hair.copy()
result.data = hair.data.copy()
result.name = "Fotbiler_Runtime_Hair_Short01_Clearance"

bpy.context.collection.objects.link(result)

hair.hide_viewport = True
hair.hide_render = True


# ------------------------------------------------------------
# Tiny expansion around the head.
#
# X/Y need slightly more clearance than Z.
# Hair top silhouette should barely change.
# ------------------------------------------------------------

SCALE_X = 1.025
SCALE_Y = 1.025
SCALE_Z = 1.012

for vertex in result.data.vertices:
    p = result.matrix_world @ vertex.co

    d = p - center

    p = Vector((
        center.x + d.x * SCALE_X,
        center.y + d.y * SCALE_Y,
        center.z + d.z * SCALE_Z,
    ))

    vertex.co = result.matrix_world.inverted() @ p

result.data.update()


mn, mx = bounds(result)

print()
print("Clearance hair bounds:")
print(" min:", tuple(round(v, 6) for v in mn))
print(" max:", tuple(round(v, 6) for v in mx))

print()
print("Scale:")
print(" X:", SCALE_X)
print(" Y:", SCALE_Y)
print(" Z:", SCALE_Z)


for obj in bpy.context.selected_objects:
    obj.select_set(False)

result.select_set(True)
head.select_set(True)

bpy.context.view_layer.objects.active = result

OUT.parent.mkdir(
    parents=True,
    exist_ok=True,
)

bpy.ops.wm.save_as_mainfile(
    filepath=str(OUT)
)

print()
print("SAVED:", OUT)
print("SUCCESS")
