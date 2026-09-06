import bpy
from pathlib import Path

ROOT = Path.cwd()

OUTPUT = (
    ROOT
    / "character_work"
    / "private_reference"
    / "work"
    / "pes_101054"
    / "pes_101054_head_hair_only.blend"
)

HIDE = {
    "mesh_id_face_0",
    "mesh_id_hair_0",
}

SHOW = {
    "mesh_id_face_1",
    "mesh_id_face_2",
    "mesh_id_face_3",
    "mesh_id_hair_1",
}

print()
print("=== PES HEAD + HAIR VIEW PREP ===")

for name in sorted(HIDE):
    obj = bpy.data.objects.get(name)

    if obj is None:
        print("MISSING:", name)
        continue

    obj.hide_viewport = True
    obj.hide_render = True
    obj.hide_set(True)

    print("HIDE:", name)

for name in sorted(SHOW):
    obj = bpy.data.objects.get(name)

    if obj is None:
        print("MISSING:", name)
        continue

    obj.hide_viewport = False
    obj.hide_render = False
    obj.hide_set(False)

    print("SHOW:", name)

print()
print("Visible mesh objects:")

for obj in bpy.data.objects:
    if obj.type != "MESH":
        continue

    state = "HIDDEN" if obj.hide_get() or obj.hide_viewport else "VISIBLE"

    print(
        f"{obj.name:<30} "
        f"{state:<8} "
        f"verts={len(obj.data.vertices):5d} "
        f"polys={len(obj.data.polygons):5d}"
    )

OUTPUT.parent.mkdir(
    parents=True,
    exist_ok=True
)

bpy.ops.wm.save_as_mainfile(
    filepath=str(OUTPUT)
)

print()
print("Saved:", OUTPUT)
print("SUCCESS")
