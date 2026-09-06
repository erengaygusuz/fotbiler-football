import bpy
import importlib
from pathlib import Path
from mathutils import Vector, Matrix

ROOT = Path("/run/media/ereng/files/projects/fotbiler-football")

MHCLO = (
    ROOT
    / "character_work/asset_packs/work/fotbiler_candidates"
    / "makehuman_system_assets_cc0/hair/short01/short01.mhclo"
)

CLEAN_BLEND = ROOT / "character_work/mpfb/mpfb_clean_head.blend"
FITTED_BLEND = ROOT / "character_work/mpfb/mpfb_fitted_head.blend"

OUT = (
    ROOT
    / "character_work/asset_packs/work"
    / "mpfb_fitted_short01_preview.blend"
)

CLEAN_HEAD_NAME = "MPFB_FotbilerHead_Clean"
FITTED_HEAD_NAME = "MPFB_FotbilerHead_Fitted"

FITTED_EYES = [
    "MPFB_Eye_Left_Fitted",
    "MPFB_Eye_Right_Fitted",
]

MPFB_PACKAGE = "bl_ext.blender_org.mpfb"

HumanService = importlib.import_module(
    f"{MPFB_PACKAGE}.services.humanservice"
).HumanService

ObjectService = importlib.import_module(
    f"{MPFB_PACKAGE}.services.objectservice"
).ObjectService


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


def triangle_count(obj):
    obj.data.calc_loop_triangles()
    return len(obj.data.loop_triangles)


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

    return obj


print()
print("=" * 72)
print("FOTBILER SHORT01 CORRECTED FIT")
print("=" * 72)


# ------------------------------------------------------------
# Existing MPFB basemesh
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
    raise RuntimeError("MPFB_BaseHuman not found.")

print("Human:", human.name)


# ------------------------------------------------------------
# Fit MakeHuman short01 to the original MPFB human
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
    raise RuntimeError("short01 MHCLO fit failed.")

print()
print("Source hair:")
print(" verts:", len(source_hair.data.vertices))
print(" tris :", triangle_count(source_hair))


# ------------------------------------------------------------
# Load EXACT head objects
# ------------------------------------------------------------

clean_head = load_object(
    CLEAN_BLEND,
    CLEAN_HEAD_NAME,
)

fitted_head = load_object(
    FITTED_BLEND,
    FITTED_HEAD_NAME,
)

src_min, src_max = world_bounds(clean_head)
dst_min, dst_max = world_bounds(fitted_head)

print()
print("CLEAN HEAD")
print(" min :", tuple(round(x, 6) for x in src_min))
print(" max :", tuple(round(x, 6) for x in src_max))
print(
    " size:",
    tuple(
        round(src_max[i] - src_min[i], 6)
        for i in range(3)
    )
)

print()
print("FITTED HEAD")
print(" min :", tuple(round(x, 6) for x in dst_min))
print(" max :", tuple(round(x, 6) for x in dst_max))
print(
    " size:",
    tuple(
        round(dst_max[i] - dst_min[i], 6)
        for i in range(3)
    )
)


# ------------------------------------------------------------
# Exact affine used for MPFB clean head -> Fotbiler runtime head
# ------------------------------------------------------------

src_size = src_max - src_min
dst_size = dst_max - dst_min

scale = Vector((
    dst_size.x / src_size.x,
    dst_size.y / src_size.y,
    dst_size.z / src_size.z,
))

translation = Vector((
    dst_min.x - src_min.x * scale.x,
    dst_min.y - src_min.y * scale.y,
    dst_min.z - src_min.z * scale.z,
))

print()
print("AFFINE")
print(
    " scale:",
    tuple(round(x, 9) for x in scale)
)
print(
    " trans:",
    tuple(round(x, 9) for x in translation)
)


# ------------------------------------------------------------
# Transform short01 through SAME affine
# ------------------------------------------------------------

# Bake the fully evaluated MPFB hair first.
#
# Important: copying source_hair.data directly can preserve MPFB
# fitting state such as shape keys/modifiers. Raw vertex bounds may
# then look correct while Blender renders the evaluated mesh elsewhere.
depsgraph = bpy.context.evaluated_depsgraph_get()
bpy.context.view_layer.update()

evaluated_hair = source_hair.evaluated_get(depsgraph)

baked_mesh = bpy.data.meshes.new_from_object(
    evaluated_hair,
    preserve_all_data_layers=True,
    depsgraph=depsgraph,
)

runtime_hair = bpy.data.objects.new(
    "Fotbiler_Runtime_Hair_Short01",
    baked_mesh,
)

bpy.context.collection.objects.link(runtime_hair)

# Use the evaluated object's world matrix, then bake the exact
# clean-head -> Fotbiler-runtime-head affine into the vertices.
source_matrix = evaluated_hair.matrix_world.copy()

for vertex in runtime_hair.data.vertices:
    p = source_matrix @ vertex.co

    vertex.co = Vector((
        p.x * scale.x + translation.x,
        p.y * scale.y + translation.y,
        p.z * scale.z + translation.z,
    ))

runtime_hair.matrix_world = Matrix.Identity(4)
runtime_hair.parent = None

# Runtime hair must be a plain static mesh.
runtime_hair.data.update()

if runtime_hair.data.shape_keys is not None:
    raise RuntimeError(
        "Baked runtime hair unexpectedly still has shape keys."
    )

if len(runtime_hair.modifiers) != 0:
    raise RuntimeError(
        "Baked runtime hair unexpectedly still has modifiers."
    )

print()
print("BAKED HAIR")
print(" shape keys:", runtime_hair.data.shape_keys)
print(" modifiers :", len(runtime_hair.modifiers))
print(" parent    :", runtime_hair.parent)


# ------------------------------------------------------------
# Add fitted runtime head/eyes as PLAIN meshes.
#
# The MPFB .blend can retain shape-key data. The raw mesh vertices
# contain our fitted runtime coordinates, but Blender's viewport may
# evaluate old MPFB shape-key coordinates instead.
#
# Build new mesh datablocks directly from the fitted raw geometry so
# the preview exactly matches the coordinates exported to Fotbiler.
# ------------------------------------------------------------

def make_plain_mesh_object(source, name):
    src_mesh = source.data

    vertices = [
        tuple(v.co)
        for v in src_mesh.vertices
    ]

    faces = [
        tuple(p.vertices)
        for p in src_mesh.polygons
    ]

    mesh = bpy.data.meshes.new(name + "_Mesh")
    mesh.from_pydata(vertices, [], faces)
    mesh.update()

    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)

    # Preserve materials.
    for mat in src_mesh.materials:
        mesh.materials.append(mat)

    # Preserve polygon material assignment where possible.
    if len(mesh.polygons) == len(src_mesh.polygons):
        for dst_poly, src_poly in zip(
            mesh.polygons,
            src_mesh.polygons,
        ):
            dst_poly.material_index = src_poly.material_index

    return obj


runtime_head = make_plain_mesh_object(
    fitted_head,
    "Fotbiler_Runtime_Head_Reference",
)

runtime_eyes = []

for eye_name in FITTED_EYES:
    source_eye = load_object(
        FITTED_BLEND,
        eye_name,
    )

    runtime_eye = make_plain_mesh_object(
        source_eye,
        eye_name,
    )

    runtime_eyes.append(runtime_eye)

    bpy.data.objects.remove(
        source_eye,
        do_unlink=True,
    )

# We no longer need the MPFB-backed fitted head object.
bpy.data.objects.remove(
    fitted_head,
    do_unlink=True,
)

fitted_head = runtime_head


# ------------------------------------------------------------
# Hide MPFB source human + temporary source hair
# ------------------------------------------------------------

# The preview must contain only the actual runtime result.
# Remove MPFB source objects completely instead of merely hiding them.
bpy.data.objects.remove(source_hair, do_unlink=True)
bpy.data.objects.remove(human, do_unlink=True)


# ------------------------------------------------------------
# Final bounds
# ------------------------------------------------------------

hmn, hmx = world_bounds(fitted_head)
smn, smx = world_bounds(runtime_hair)

print()
print("=" * 72)
print("FINAL HEAD")
print(" min :", tuple(round(x, 6) for x in hmn))
print(" max :", tuple(round(x, 6) for x in hmx))

print()
print("FINAL SHORT01")
print(" min :", tuple(round(x, 6) for x in smn))
print(" max :", tuple(round(x, 6) for x in smx))
print(
    " size:",
    tuple(
        round(smx[i] - smn[i], 6)
        for i in range(3)
    )
)
print(" verts:", len(runtime_hair.data.vertices))
print(" tris :", triangle_count(runtime_hair))


# ------------------------------------------------------------
# Select result
# ------------------------------------------------------------

for obj in bpy.context.selected_objects:
    obj.select_set(False)

fitted_head.select_set(True)
runtime_hair.select_set(True)

bpy.context.view_layer.objects.active = runtime_hair


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
