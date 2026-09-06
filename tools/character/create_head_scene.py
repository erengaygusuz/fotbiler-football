import bpy
from pathlib import Path

root = Path.cwd()

source = (
    root
    / "character_work"
    / "legacy_head"
    / "legacy_runtime_head.obj"
)

output = (
    root
    / "character_work"
    / "fotbiler_head_poc.blend"
)

if not source.exists():
    raise RuntimeError(f"OBJ bulunamadı: {source}")

# Boş sahne.
bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)

# OBJ import.
bpy.ops.wm.obj_import(
    filepath=str(source)
)

mesh_objects = [
    obj
    for obj in bpy.context.selected_objects
    if obj.type == "MESH"
]

if len(mesh_objects) != 1:
    raise RuntimeError(
        f"1 mesh bekleniyordu, {len(mesh_objects)} bulundu."
    )

ref = mesh_objects[0]

ref.name = "REF_LegacyRuntimeHead"
ref.data.name = "REF_LegacyRuntimeHead_Mesh"

# Referans olarak göster.
ref.display_type = "WIRE"
ref.show_in_front = True

output.parent.mkdir(
    parents=True,
    exist_ok=True
)

bpy.ops.wm.save_as_mainfile(
    filepath=str(output)
)

print()
print("=== Fotbiler Head Scene ===")
print("Source :", source)
print("Object :", ref.name)
print("Verts  :", len(ref.data.vertices))
print("Faces  :", len(ref.data.polygons))
print("Saved  :", output)
print("SUCCESS")
