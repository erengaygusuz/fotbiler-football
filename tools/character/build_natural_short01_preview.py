import bpy
import importlib
from pathlib import Path
from mathutils import Vector, Matrix

ROOT = Path("/run/media/ereng/files/projects/fotbiler-football")

BASE_BLEND = ROOT / "character_work/mpfb/mpfb_base_human.blend"
CLEAN_BLEND = ROOT / "character_work/mpfb/mpfb_clean_head.blend"

MHCLO = (
    ROOT
    / "character_work/asset_packs/work/fotbiler_candidates"
    / "makehuman_system_assets_cc0/hair/short01/short01.mhclo"
)

OUT = (
    ROOT
    / "character_work/asset_packs/work"
    / "mpfb_natural_short01_preview.blend"
)

HEAD_NAME = "MPFB_FotbilerHead_Clean"

EYE_NAMES = [
    "MPFB_Eye_Left",
    "MPFB_Eye_Right",
]

MPFB_PACKAGE = "bl_ext.blender_org.mpfb"

HumanService = importlib.import_module(
    f"{MPFB_PACKAGE}.services.humanservice"
).HumanService


# ------------------------------------------------------------
# Runtime anchor
#
# We preserve MPFB proportions.
#
# Scale is derived ONLY from head width:
#
# legacy width = 0.166
# MPFB width   = 0.17586
#
# No independent XYZ stretching.
# ------------------------------------------------------------

LEGACY_HEAD_WIDTH = 0.166

TARGET_CENTER = Vector((
    0.0,
    -0.330,
    1.6705,
))


def load_object(blend_path, name):
    with bpy.data.libraries.load(
        str(blend_path),
        link=False,
    ) as (src, dst):

        if name not in src.objects:
            raise RuntimeError(
                f"{name} missing from {blend_path}"
            )

        dst.objects = [name]

    return dst.objects[0]


def world_bounds(obj):
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


def make_plain_transformed(
    source,
    name,
    scale,
    translation,
):
    mw = source.matrix_world.copy()

    verts = []

    for v in source.data.vertices:
        p = mw @ v.co

        verts.append((
            p.x * scale + translation.x,
            p.y * scale + translation.y,
            p.z * scale + translation.z,
        ))

    faces = [
        tuple(poly.vertices)
        for poly in source.data.polygons
    ]

    mesh = bpy.data.meshes.new(name + "_Mesh")

    mesh.from_pydata(
        verts,
        [],
        faces,
    )

    mesh.update()

    obj = bpy.data.objects.new(
        name,
        mesh,
    )

    bpy.context.collection.objects.link(obj)

    # --------------------------------------------
    # Restore smooth appearance.
    # from_pydata defaults to flat polygons.
    # --------------------------------------------

    for poly in mesh.polygons:
        poly.use_smooth = True

    # Materials
    for mat in source.data.materials:
        mesh.materials.append(mat)

    if len(mesh.polygons) == len(source.data.polygons):
        for dst_poly, src_poly in zip(
            mesh.polygons,
            source.data.polygons,
        ):
            dst_poly.material_index = src_poly.material_index

    # --------------------------------------------
    # Preserve UV layout
    # --------------------------------------------

    if source.data.uv_layers:
        src_uv = source.data.uv_layers.active

        if src_uv is not None:
            dst_uv = mesh.uv_layers.new(
                name=src_uv.name
            )

            if len(dst_uv.data) == len(src_uv.data):
                for i in range(len(dst_uv.data)):
                    dst_uv.data[i].uv = (
                        src_uv.data[i].uv.copy()
                    )

    return obj


print()
print("=" * 72)
print("FOTBILER NATURAL HEAD + SHORT01")
print("=" * 72)


# ------------------------------------------------------------
# Base human
# ------------------------------------------------------------

human = bpy.data.objects.get("MPFB_BaseHuman")

if human is None:
    raise RuntimeError("MPFB_BaseHuman missing.")


# ------------------------------------------------------------
# Fit source short01 using MHCLO
# ------------------------------------------------------------

source_hair = HumanService.add_mhclo_asset(
    str(MHCLO),
    human,
    asset_type="Hair",
    subdiv_levels=0,
    material_type="GAMEENGINE",
    set_up_rigging=False,
)

if source_hair is None:
    raise RuntimeError("Could not fit short01.")


# ------------------------------------------------------------
# Clean MPFB head
# ------------------------------------------------------------

source_head = load_object(
    CLEAN_BLEND,
    HEAD_NAME,
)

src_min, src_max = world_bounds(source_head)

src_size = src_max - src_min
src_center = (src_min + src_max) * 0.5

UNIFORM_SCALE = (
    LEGACY_HEAD_WIDTH / src_size.x
)

translation = (
    TARGET_CENTER
    - src_center * UNIFORM_SCALE
)

print()
print("SOURCE HEAD")
print(
    " min:",
    tuple(round(v, 6) for v in src_min)
)
print(
    " max:",
    tuple(round(v, 6) for v in src_max)
)
print(
    " size:",
    tuple(round(v, 6) for v in src_size)
)

print()
print("SIMILARITY TRANSFORM")
print(" scale:", round(UNIFORM_SCALE, 9))
print(
    " translation:",
    tuple(round(v, 9) for v in translation)
)


# ------------------------------------------------------------
# Natural head
# ------------------------------------------------------------

runtime_head = make_plain_transformed(
    source_head,
    "Fotbiler_Natural_Head",
    UNIFORM_SCALE,
    translation,
)


# ------------------------------------------------------------
# Eyes: SAME transform
# ------------------------------------------------------------

for eye_name in EYE_NAMES:
    eye = load_object(
        CLEAN_BLEND,
        eye_name,
    )

    make_plain_transformed(
        eye,
        eye_name + "_Natural",
        UNIFORM_SCALE,
        translation,
    )


# ------------------------------------------------------------
# Hair:
# bake evaluated MPFB fitting result first
# ------------------------------------------------------------

depsgraph = bpy.context.evaluated_depsgraph_get()
bpy.context.view_layer.update()

evaluated_hair = source_hair.evaluated_get(
    depsgraph
)

baked_mesh = bpy.data.meshes.new_from_object(
    evaluated_hair,
    preserve_all_data_layers=True,
    depsgraph=depsgraph,
)

runtime_hair = bpy.data.objects.new(
    "Fotbiler_Natural_Hair_Short01",
    baked_mesh,
)

bpy.context.collection.objects.link(
    runtime_hair
)

mw = evaluated_hair.matrix_world.copy()

for vertex in runtime_hair.data.vertices:
    p = mw @ vertex.co

    vertex.co = Vector((
        p.x * UNIFORM_SCALE + translation.x,
        p.y * UNIFORM_SCALE + translation.y,
        p.z * UNIFORM_SCALE + translation.z,
    ))

runtime_hair.matrix_world = Matrix.Identity(4)
runtime_hair.parent = None
runtime_hair.data.update()


# ------------------------------------------------------------
# Final audit
# ------------------------------------------------------------

hmn, hmx = world_bounds(runtime_head)
smn, smx = world_bounds(runtime_hair)

print()
print("FINAL HEAD")
print(
    " min:",
    tuple(round(v, 6) for v in hmn)
)
print(
    " max:",
    tuple(round(v, 6) for v in hmx)
)
print(
    " size:",
    tuple(
        round(hmx[i] - hmn[i], 6)
        for i in range(3)
    )
)

print()
print("FINAL HAIR")
print(
    " min:",
    tuple(round(v, 6) for v in smn)
)
print(
    " max:",
    tuple(round(v, 6) for v in smx)
)

print()
print(
    "Head vertices:",
    len(runtime_head.data.vertices)
)
print(
    "Hair vertices:",
    len(runtime_hair.data.vertices)
)

# Remove working MPFB objects.
bpy.data.objects.remove(
    source_hair,
    do_unlink=True,
)

bpy.data.objects.remove(
    human,
    do_unlink=True,
)

bpy.data.objects.remove(
    source_head,
    do_unlink=True,
)

# Select final result.
for obj in bpy.context.selected_objects:
    obj.select_set(False)

runtime_head.select_set(True)
runtime_hair.select_set(True)

bpy.context.view_layer.objects.active = (
    runtime_hair
)

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
