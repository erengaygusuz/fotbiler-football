import bpy
import importlib
from pathlib import Path
from mathutils import Vector, Matrix

ROOT = Path("/run/media/ereng/files/projects/fotbiler-football")

CLEAN_BLEND = ROOT / "character_work/mpfb/mpfb_clean_head.blend"

MHCLO = (
    ROOT
    / "character_work/asset_packs/work/fotbiler_candidates"
    / "makehuman_system_assets_cc0/hair/short01/short01.mhclo"
)

OUT = (
    ROOT
    / "character_work/asset_packs/work"
    / "mpfb_same_source_short01_preview.blend"
)

CLEAN_HEAD_NAME = "MPFB_FotbilerHead_Clean"

EYE_NAMES = [
    ("MPFB_Eye_Left", "Fotbiler_Runtime_Eye_Left"),
    ("MPFB_Eye_Right", "Fotbiler_Runtime_Eye_Right"),
]

MPFB_PACKAGE = "bl_ext.blender_org.mpfb"

HumanService = importlib.import_module(
    f"{MPFB_PACKAGE}.services.humanservice"
).HumanService

LEGACY_HEAD_WIDTH = 0.166

TARGET_CENTER = Vector((
    0.0,
    -0.330,
    1.6705,
))


def load_object(blend_path, object_name):
    with bpy.data.libraries.load(
        str(blend_path),
        link=False,
    ) as (src, dst):

        if object_name not in src.objects:
            raise RuntimeError(
                f"{object_name} not found in {blend_path}"
            )

        dst.objects = [object_name]

    obj = dst.objects[0]

    if obj is None:
        raise RuntimeError(
            f"Could not load {object_name}"
        )

    bpy.context.collection.objects.link(obj)

    return obj


def bake_evaluated_object(source, name, depsgraph):
    bpy.context.view_layer.update()

    evaluated = source.evaluated_get(depsgraph)

    mesh = bpy.data.meshes.new_from_object(
        evaluated,
        preserve_all_data_layers=True,
        depsgraph=depsgraph,
    )

    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)

    obj.matrix_world = evaluated.matrix_world.copy()

    return obj


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


def bake_similarity_transform(
    obj,
    scale,
    translation,
):
    mw = obj.matrix_world.copy()

    for vertex in obj.data.vertices:
        p = mw @ vertex.co

        vertex.co = Vector((
            p.x * scale + translation.x,
            p.y * scale + translation.y,
            p.z * scale + translation.z,
        ))

    obj.matrix_world = Matrix.Identity(4)
    obj.parent = None
    obj.data.update()


def show_bounds(label, obj):
    mn, mx = world_bounds(obj)

    print()
    print(label)
    print(
        " min :",
        tuple(round(x, 6) for x in mn)
    )
    print(
        " max :",
        tuple(round(x, 6) for x in mx)
    )
    print(
        " size:",
        tuple(
            round(mx[i] - mn[i], 6)
            for i in range(3)
        )
    )

    return mn, mx


print()
print("=" * 72)
print("FOTBILER SAME-EVALUATION HEAD + SHORT01")
print("=" * 72)


# ------------------------------------------------------------
# MPFB human from the currently opened base-human blend
# ------------------------------------------------------------

human = bpy.data.objects.get("MPFB_BaseHuman")

if human is None:
    raise RuntimeError("MPFB_BaseHuman not found.")


# ------------------------------------------------------------
# Fit short01 to THIS human
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
    raise RuntimeError("short01 fit failed.")


# ------------------------------------------------------------
# Load clean head but evaluate/bake its MPFB state BEFORE fitting
# to Fotbiler coordinates.
# ------------------------------------------------------------

source_head = load_object(
    CLEAN_BLEND,
    CLEAN_HEAD_NAME,
)

depsgraph = bpy.context.evaluated_depsgraph_get()
bpy.context.view_layer.update()

baked_head = bake_evaluated_object(
    source_head,
    "Fotbiler_Runtime_Head_Reference",
    depsgraph,
)

baked_hair = bake_evaluated_object(
    source_hair,
    "Fotbiler_Runtime_Hair_Short01",
    depsgraph,
)

source_eyes = []
baked_eyes = []

for source_name, runtime_name in EYE_NAMES:
    source_eye = load_object(
        CLEAN_BLEND,
        source_name,
    )

    baked_eye = bake_evaluated_object(
        source_eye,
        runtime_name,
        depsgraph,
    )

    source_eyes.append(source_eye)
    baked_eyes.append(baked_eye)


# ------------------------------------------------------------
# Measure the ACTUAL evaluated source head.
# ------------------------------------------------------------

src_min, src_max = show_bounds(
    "EVALUATED SOURCE HEAD",
    baked_head,
)

src_size = src_max - src_min
src_center = (src_min + src_max) * 0.5

uniform_scale = (
    LEGACY_HEAD_WIDTH / src_size.x
)

translation = (
    TARGET_CENTER
    - src_center * uniform_scale
)

print()
print("SIMILARITY TRANSFORM")
print(
    " scale:",
    round(uniform_scale, 9)
)
print(
    " translation:",
    tuple(round(x, 9) for x in translation)
)


# ------------------------------------------------------------
# CRITICAL:
# exact same transform on both evaluated head and evaluated hair.
# ------------------------------------------------------------

bake_similarity_transform(
    baked_head,
    uniform_scale,
    translation,
)

bake_similarity_transform(
    baked_hair,
    uniform_scale,
    translation,
)

for baked_eye in baked_eyes:
    bake_similarity_transform(
        baked_eye,
        uniform_scale,
        translation,
    )


# ------------------------------------------------------------
# Smooth head only.
# Hair keeps its authored shading.
# ------------------------------------------------------------

for poly in baked_head.data.polygons:
    poly.use_smooth = True

for baked_eye in baked_eyes:
    for poly in baked_eye.data.polygons:
        poly.use_smooth = True


# ------------------------------------------------------------
# Runtime objects must contain no MPFB deformation state.
# ------------------------------------------------------------

if baked_head.data.shape_keys:
    raise RuntimeError("Runtime head still has shape keys.")

if baked_hair.data.shape_keys:
    raise RuntimeError("Runtime hair still has shape keys.")

if baked_head.modifiers:
    raise RuntimeError("Runtime head still has modifiers.")

if baked_hair.modifiers:
    raise RuntimeError("Runtime hair still has modifiers.")

for baked_eye in baked_eyes:
    if baked_eye.data.shape_keys:
        raise RuntimeError(
            f"{baked_eye.name} still has shape keys."
        )

    if baked_eye.modifiers:
        raise RuntimeError(
            f"{baked_eye.name} still has modifiers."
        )


# ------------------------------------------------------------
# Final audit
# ------------------------------------------------------------

hmn, hmx = show_bounds(
    "FINAL HEAD",
    baked_head,
)

smn, smx = show_bounds(
    "FINAL SHORT01",
    baked_hair,
)

head_center = (hmn + hmx) * 0.5
hair_center = (smn + smx) * 0.5

print()
print("RELATION")
print(
    "hair center - head center:",
    tuple(
        round(x, 6)
        for x in (hair_center - head_center)
    )
)

print()
print("HEAD")
print(" verts:", len(baked_head.data.vertices))
print(" tris :", len(baked_head.data.loop_triangles))

baked_head.data.calc_loop_triangles()

print()
print("HAIR")
print(" verts:", len(baked_hair.data.vertices))

baked_hair.data.calc_loop_triangles()

print(" tris :", len(baked_hair.data.loop_triangles))


# ------------------------------------------------------------
# Remove every MPFB/source object from preview.
# ------------------------------------------------------------

for obj in [
    source_head,
    source_hair,
    *source_eyes,
    human,
]:
    if obj and obj.name in bpy.data.objects:
        bpy.data.objects.remove(
            obj,
            do_unlink=True,
        )


# ------------------------------------------------------------
# Selection
# ------------------------------------------------------------

for obj in bpy.context.selected_objects:
    obj.select_set(False)

baked_head.select_set(True)
baked_hair.select_set(True)

bpy.context.view_layer.objects.active = baked_hair


# ------------------------------------------------------------
# Save
# ------------------------------------------------------------

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
