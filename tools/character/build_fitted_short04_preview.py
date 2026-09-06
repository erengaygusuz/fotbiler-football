import bpy
import importlib
from pathlib import Path
from mathutils import Vector, Matrix

ROOT = Path("/run/media/ereng/files/projects/fotbiler-football")

MHCLO = (
    ROOT
    / "character_work/asset_packs/work/fotbiler_candidates"
    / "makehuman_system_assets_cc0/hair/short04/short04.mhclo"
)

CLEAN_HEAD_BLEND = ROOT / "character_work/mpfb/mpfb_clean_head.blend"
FITTED_HEAD_BLEND = ROOT / "character_work/mpfb/mpfb_fitted_head.blend"

OUT = (
    ROOT
    / "character_work/asset_packs/work"
    / "mpfb_fitted_short04_preview.blend"
)

MPFB_PACKAGE = "bl_ext.blender_org.mpfb"

HumanService = importlib.import_module(
    f"{MPFB_PACKAGE}.services.humanservice"
).HumanService

ObjectService = importlib.import_module(
    f"{MPFB_PACKAGE}.services.objectservice"
).ObjectService


def world_bounds(obj):
    points = [
        obj.matrix_world @ v.co
        for v in obj.data.vertices
    ]

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


def triangle_count(obj):
    return sum(
        max(0, len(p.vertices) - 2)
        for p in obj.data.polygons
    )


def load_mesh_objects(blend_path):
    with bpy.data.libraries.load(
        str(blend_path),
        link=False
    ) as (data_from, data_to):
        data_to.objects = list(data_from.objects)

    return [
        obj
        for obj in data_to.objects
        if obj is not None and obj.type == "MESH"
    ]


print()
print("=== FOTBILER SHORT04 RUNTIME FIT ===")

for p in [MHCLO, CLEAN_HEAD_BLEND, FITTED_HEAD_BLEND]:
    if not p.exists():
        raise RuntimeError(f"Missing: {p}")


# ------------------------------------------------------------
# Existing MPFB human
# ------------------------------------------------------------

human = bpy.data.objects.get("MPFB_BaseHuman")

if human is None:
    for obj in bpy.data.objects:
        if obj.type != "MESH":
            continue

        try:
            if ObjectService.object_is_basemesh(obj):
                human = obj
                break
        except Exception:
            pass

if human is None:
    raise RuntimeError("MPFB basemesh not found.")

print("Human:", human.name)


# ------------------------------------------------------------
# Fit short04 to original MPFB human
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
    raise RuntimeError("Could not create short04 hair.")

print()
print("Source fitted hair:")
print("  vertices :", len(hair.data.vertices))
print("  triangles:", triangle_count(hair))
print("  uv maps  :", [uv.name for uv in hair.data.uv_layers])


# ------------------------------------------------------------
# Load clean and already-fitted head references
# ------------------------------------------------------------

clean_objects = load_mesh_objects(CLEAN_HEAD_BLEND)
fitted_objects = load_mesh_objects(FITTED_HEAD_BLEND)

if not clean_objects:
    raise RuntimeError("No meshes in clean head blend.")

if not fitted_objects:
    raise RuntimeError("No meshes in fitted head blend.")


# Main head = mesh with most vertices.
clean_head = max(
    clean_objects,
    key=lambda o: len(o.data.vertices)
)

fitted_head = max(
    fitted_objects,
    key=lambda o: len(o.data.vertices)
)

src_min, src_max = world_bounds(clean_head)
dst_min, dst_max = world_bounds(fitted_head)

print()
print("Clean head:")
print(" ", clean_head.name)
print("  min:", tuple(round(x, 6) for x in src_min))
print("  max:", tuple(round(x, 6) for x in src_max))

print()
print("Fitted head:")
print(" ", fitted_head.name)
print("  min:", tuple(round(x, 6) for x in dst_min))
print("  max:", tuple(round(x, 6) for x in dst_max))


# ------------------------------------------------------------
# Derive EXACT affine fit used by the head:
# independent XYZ mapping from clean bounds -> fitted bounds.
# ------------------------------------------------------------

scale = Vector((
    (dst_max.x - dst_min.x) / (src_max.x - src_min.x),
    (dst_max.y - dst_min.y) / (src_max.y - src_min.y),
    (dst_max.z - dst_min.z) / (src_max.z - src_min.z),
))

translation = Vector((
    dst_min.x - src_min.x * scale.x,
    dst_min.y - src_min.y * scale.y,
    dst_min.z - src_min.z * scale.z,
))

print()
print("Affine:")
print("  scale      :", tuple(round(x, 9) for x in scale))
print("  translation:", tuple(round(x, 9) for x in translation))


# ------------------------------------------------------------
# Create runtime hair copy and bake the transform into geometry
# ------------------------------------------------------------

runtime_hair = hair.copy()
runtime_hair.data = hair.data.copy()
runtime_hair.name = "Fotbiler_Runtime_Hair_Short04"
runtime_hair.parent = None

bpy.context.collection.objects.link(runtime_hair)

source_matrix = hair.matrix_world.copy()

for vertex in runtime_hair.data.vertices:
    p = source_matrix @ vertex.co

    vertex.co = Vector((
        p.x * scale.x + translation.x,
        p.y * scale.y + translation.y,
        p.z * scale.z + translation.z,
    ))

runtime_hair.matrix_world = Matrix.Identity(4)


# ------------------------------------------------------------
# Add already fitted Fotbiler head + eye meshes to scene
# ------------------------------------------------------------

for obj in fitted_objects:
    if obj.name not in bpy.context.collection.objects:
        bpy.context.collection.objects.link(obj)

    obj.hide_viewport = False
    obj.hide_render = False

    if obj == fitted_head:
        obj.name = "Fotbiler_Runtime_Head_Reference"


# ------------------------------------------------------------
# Hide source/full human and temporary hair
# ------------------------------------------------------------

human.hide_viewport = True
human.hide_render = True

hair.hide_viewport = True
hair.hide_render = True

for obj in clean_objects:
    obj.hide_viewport = True
    obj.hide_render = True


# ------------------------------------------------------------
# Runtime audit
# ------------------------------------------------------------

hair_min, hair_max = world_bounds(runtime_hair)

print()
print("=== RUNTIME SHORT04 ===")
print("Vertices :", len(runtime_hair.data.vertices))
print("Triangles:", triangle_count(runtime_hair))
print("UV maps  :", [uv.name for uv in runtime_hair.data.uv_layers])
print(
    "Bounds min:",
    tuple(round(x, 6) for x in hair_min)
)
print(
    "Bounds max:",
    tuple(round(x, 6) for x in hair_max)
)
print(
    "Bounds size:",
    tuple(
        round(hair_max[i] - hair_min[i], 6)
        for i in range(3)
    )
)

print()
print("=== MATERIALS ===")

for mat in runtime_hair.data.materials:
    if not mat:
        continue

    print("Material:", mat.name)

    if mat.use_nodes and mat.node_tree:
        for node in mat.node_tree.nodes:
            if node.type == "TEX_IMAGE" and node.image:
                print(
                    "  image:",
                    node.image.name,
                    node.image.size[0],
                    "x",
                    node.image.size[1],
                )


# ------------------------------------------------------------
# Selection
# ------------------------------------------------------------

for obj in bpy.context.selected_objects:
    obj.select_set(False)

runtime_hair.select_set(True)
fitted_head.select_set(True)
bpy.context.view_layer.objects.active = runtime_hair


# ------------------------------------------------------------
# Save
# ------------------------------------------------------------

OUT.parent.mkdir(parents=True, exist_ok=True)

bpy.ops.wm.save_as_mainfile(filepath=str(OUT))

print()
print("SAVED:", OUT)
print("SUCCESS")
