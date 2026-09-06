import bpy
import sys
import traceback
from pathlib import Path

ROOT = Path.cwd()

DEPS_DIR = (
    ROOT
    / "character_work"
    / "private_reference"
    / "tools"
    / "blender-python-deps"
)

if not DEPS_DIR.exists():
    raise RuntimeError(
        f"Blender Python dependencies missing: {DEPS_DIR}"
    )

sys.path.insert(0, str(DEPS_DIR))

import numpy

print(
    "Private numpy:",
    numpy.__version__,
    numpy.__file__
)

ADDON_DIR = (
    ROOT
    / "character_work"
    / "private_reference"
    / "tools"
    / "pes-face-hair-modifier-45"
)

MODEL_DIR = (
    ROOT
    / "character_work"
    / "private_reference"
    / "work"
    / "pes_101054"
    / "fpk"
)

FACE = MODEL_DIR / "face_high.fmdl"
HAIR = MODEL_DIR / "hair_high.fmdl"

OUTPUT = (
    ROOT
    / "character_work"
    / "private_reference"
    / "work"
    / "pes_101054"
    / "pes_101054_face_hair.blend"
)

if not ADDON_DIR.exists():
    raise RuntimeError(
        f"Addon directory missing: {ADDON_DIR}"
    )

if not FACE.exists():
    raise RuntimeError(f"Missing: {FACE}")

if not HAIR.exists():
    raise RuntimeError(f"Missing: {HAIR}")

sys.path.insert(0, str(ADDON_DIR))

print()
print("=== PES FMDL IMPORT TEST ===")
print("Addon :", ADDON_DIR)
print("Face  :", FACE)
print("Hair  :", HAIR)
print()

import PES_Face_Hair_Modifier as pes

print(
    "Addon version:",
    getattr(pes, "bl_info", {}).get("version")
)

# Register Blender custom properties/classes required by importer.
try:
    pes.register()
    print("Addon register: OK")
except Exception as exc:
    print("Addon register failed:")
    traceback.print_exc()
    raise

scene = bpy.context.scene

# Geometry-only first test.
scene.fmdl_import_extensions_enabled = True
scene.fmdl_import_loop_preservation = True
scene.fmdl_import_mesh_splitting = True
scene.fmdl_import_load_textures = False
scene.fmdl_import_all_bounding_boxes = False
scene.fixmeshesmooth = True

# Clean default scene objects.
bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)

print()
print("--- IMPORT FACE ---")

pes.importFmdlfile(
    str(FACE),
    "Skeleton_Face",
    "mesh_id_face",
    "face_high"
)

print("Face import OK")

print()
print("--- IMPORT HAIR ---")

pes.importFmdlfile(
    str(HAIR),
    "Skeleton_Hair",
    "mesh_id_hair",
    "hair_high"
)

print("Hair import OK")

print()
print("=== OBJECT AUDIT ===")

mesh_objects = []
armatures = []

for obj in bpy.data.objects:

    if obj.type == "MESH":
        mesh_objects.append(obj)

        mesh = obj.data

        print()
        print("MESH:", obj.name)
        print("  vertices :", len(mesh.vertices))
        print("  edges    :", len(mesh.edges))
        print("  polygons :", len(mesh.polygons))
        print(
            "  triangles:",
            sum(
                max(0, len(p.vertices) - 2)
                for p in mesh.polygons
            )
        )
        print(
            "  UV layers:",
            [uv.name for uv in mesh.uv_layers]
        )
        print(
            "  materials:",
            [m.name for m in mesh.materials]
        )
        print(
            "  groups   :",
            len(obj.vertex_groups)
        )

    elif obj.type == "ARMATURE":
        armatures.append(obj)

        print()
        print("ARMATURE:", obj.name)
        print(
            "  bones:",
            len(obj.data.bones)
        )

        print(
            "  bone names:",
            [b.name for b in obj.data.bones][:60]
        )

print()
print("=== TOTALS ===")
print("Meshes    :", len(mesh_objects))
print("Armatures :", len(armatures))

if not mesh_objects:
    raise RuntimeError(
        "No mesh objects imported."
    )

# World-space bounds across imported meshes.
points = []

for obj in mesh_objects:
    mw = obj.matrix_world

    for v in obj.data.vertices:
        p = mw @ v.co
        points.append(
            (float(p.x), float(p.y), float(p.z))
        )

xs = [p[0] for p in points]
ys = [p[1] for p in points]
zs = [p[2] for p in points]

print()
print("=== WORLD BOUNDS ===")
print(
    "min:",
    (
        min(xs),
        min(ys),
        min(zs),
    )
)

print(
    "max:",
    (
        max(xs),
        max(ys),
        max(zs),
    )
)

print(
    "size:",
    (
        max(xs) - min(xs),
        max(ys) - min(ys),
        max(zs) - min(zs),
    )
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
