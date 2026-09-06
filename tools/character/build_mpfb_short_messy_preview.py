import bpy
import sys
from pathlib import Path
from mathutils import Vector

ROOT = Path("/run/media/ereng/files/projects/fotbiler-football")


MHCLO = (
    ROOT
    / "character_work/asset_packs/work/fotbiler_candidates"
    / "hair01_cc0/hair/cortu_short_messy_hair"
    / "cortu_short_messy_hair.mhclo"
)

OUT = (
    ROOT
    / "character_work/asset_packs/work"
    / "mpfb_short_messy_preview.blend"
)

import importlib

# Blender 4.2+ extensions are loaded under bl_ext.<repo>.<extension>.
# Import MPFB through the already initialized extension namespace instead
# of importing it a second time as a top-level "mpfb" package.
MPFB_PACKAGE = "bl_ext.blender_org.mpfb"

HumanService = importlib.import_module(
    f"{MPFB_PACKAGE}.services.humanservice"
).HumanService

ObjectService = importlib.import_module(
    f"{MPFB_PACKAGE}.services.objectservice"
).ObjectService

print("MPFB package:", MPFB_PACKAGE)


print()
print("=== FOTBILER MPFB HAIR PREVIEW ===")
print("MHCLO:", MHCLO)

if not MHCLO.exists():
    raise RuntimeError(f"MHCLO not found: {MHCLO}")


# ------------------------------------------------------------
# Find MPFB base human
# ------------------------------------------------------------

human = bpy.data.objects.get("MPFB_BaseHuman")

if human is None:
    candidates = []

    for obj in bpy.data.objects:
        if obj.type != "MESH":
            continue

        try:
            if ObjectService.object_is_basemesh(obj):
                candidates.append(obj)
        except Exception:
            pass

    print("Basemesh candidates:", [o.name for o in candidates])

    if len(candidates) == 1:
        human = candidates[0]

if human is None:
    raise RuntimeError("MPFB basemesh could not be found.")

print("Basemesh:", human.name)
print("Basemesh vertices:", len(human.data.vertices))


# ------------------------------------------------------------
# Add hair using MPFB's own MHCLO pipeline
# ------------------------------------------------------------

hair = HumanService.add_mhclo_asset(
    str(MHCLO),
    human,
    asset_type="Hair",
    subdiv_levels=0,
    material_type="GAMEENGINE",
    set_up_rigging=False,
)

if hair is None:
    raise RuntimeError("HumanService.add_mhclo_asset returned None.")

hair.name = "Fotbiler_Hair_ShortMessy"


# ------------------------------------------------------------
# Geometry audit
# ------------------------------------------------------------

verts = len(hair.data.vertices)
polys = len(hair.data.polygons)

tris = sum(
    max(0, len(poly.vertices) - 2)
    for poly in hair.data.polygons
)

print()
print("=== FITTED HAIR ===")
print("Object   :", hair.name)
print("Vertices :", verts)
print("Polygons :", polys)
print("Triangles:", tris)
print("UV maps  :", [uv.name for uv in hair.data.uv_layers])


# ------------------------------------------------------------
# Bounds
# ------------------------------------------------------------

world_points = [
    hair.matrix_world @ Vector(corner)
    for corner in hair.bound_box
]

mn = Vector((
    min(p.x for p in world_points),
    min(p.y for p in world_points),
    min(p.z for p in world_points),
))

mx = Vector((
    max(p.x for p in world_points),
    max(p.y for p in world_points),
    max(p.z for p in world_points),
))

print("Bounds min:", tuple(round(v, 6) for v in mn))
print("Bounds max:", tuple(round(v, 6) for v in mx))
print(
    "Bounds size:",
    tuple(round(mx[i] - mn[i], 6) for i in range(3))
)


# ------------------------------------------------------------
# Material / texture audit
# ------------------------------------------------------------

print()
print("=== MATERIALS ===")

for material in hair.data.materials:
    if material is None:
        continue

    print("Material:", material.name)

    if not material.use_nodes or material.node_tree is None:
        print("  no node tree")
        continue

    for node in material.node_tree.nodes:
        if node.type == "TEX_IMAGE" and node.image:
            print(
                "  image:",
                node.image.name,
                node.image.size[0],
                "x",
                node.image.size[1],
            )


# ------------------------------------------------------------
# Nice viewport state
# ------------------------------------------------------------

for obj in bpy.context.selected_objects:
    obj.select_set(False)

hair.select_set(True)
human.select_set(True)

bpy.context.view_layer.objects.active = hair


# ------------------------------------------------------------
# Save
# ------------------------------------------------------------

OUT.parent.mkdir(parents=True, exist_ok=True)

bpy.ops.wm.save_as_mainfile(filepath=str(OUT))

print()
print("SAVED:", OUT)
print("SUCCESS")
