import bpy
from pathlib import Path
from mathutils import Vector

OBJECT_NAME = "MPFB_FotbilerHead"

obj = bpy.data.objects.get(OBJECT_NAME)

if obj is None or obj.type != "MESH":
    raise RuntimeError(
        f"{OBJECT_NAME} bulunamadı."
    )

root = Path.cwd()

out_dir = (
    root
    / "character_work"
    / "mpfb"
    / "previews"
)

out_dir.mkdir(
    parents=True,
    exist_ok=True
)

# ------------------------------------------------------------
# Hide everything except head
# ------------------------------------------------------------

for other in bpy.context.scene.objects:
    if other != obj and other.type == "MESH":
        other.hide_render = True

obj.hide_render = False
obj.hide_viewport = False

# ------------------------------------------------------------
# Simple preview material
# ------------------------------------------------------------

mat = bpy.data.materials.get(
    "MPFB_PreviewMaterial"
)

if mat is None:
    mat = bpy.data.materials.new(
        "MPFB_PreviewMaterial"
    )

    mat.use_nodes = True

    bsdf = mat.node_tree.nodes.get(
        "Principled BSDF"
    )

    if bsdf:
        bsdf.inputs["Base Color"].default_value = (
            0.32,
            0.32,
            0.32,
            1.0,
        )

        bsdf.inputs["Roughness"].default_value = 0.72

obj.data.materials.clear()
obj.data.materials.append(mat)

# ------------------------------------------------------------
# Bounds in world space
# ------------------------------------------------------------

corners = [
    obj.matrix_world @ Vector(corner)
    for corner in obj.bound_box
]

min_v = Vector((
    min(v.x for v in corners),
    min(v.y for v in corners),
    min(v.z for v in corners),
))

max_v = Vector((
    max(v.x for v in corners),
    max(v.y for v in corners),
    max(v.z for v in corners),
))

center = (min_v + max_v) * 0.5
size = max_v - min_v

max_size = max(
    size.x,
    size.y,
    size.z
)

print()
print("=== MPFB HEAD PREVIEW ===")
print(
    "Center:",
    tuple(round(v, 6) for v in center)
)

print(
    "Size  :",
    tuple(round(v, 6) for v in size)
)

# ------------------------------------------------------------
# Camera
# ------------------------------------------------------------

camera_data = bpy.data.cameras.new(
    "PreviewCamera"
)

camera = bpy.data.objects.new(
    "PreviewCamera",
    camera_data
)

bpy.context.collection.objects.link(camera)

bpy.context.scene.camera = camera

camera.data.type = "ORTHO"
camera.data.ortho_scale = max_size * 1.28


def point_camera(location):
    camera.location = location

    direction = center - camera.location

    camera.rotation_euler = (
        direction.to_track_quat(
            "-Z",
            "Y"
        ).to_euler()
    )


# ------------------------------------------------------------
# Lights
# ------------------------------------------------------------

def make_area(name, location, energy, size):
    data = bpy.data.lights.new(
        name,
        type="AREA"
    )

    data.energy = energy
    data.shape = "DISK"
    data.size = size

    light = bpy.data.objects.new(
        name,
        data
    )

    bpy.context.collection.objects.link(
        light
    )

    light.location = location

    direction = center - light.location

    light.rotation_euler = (
        direction.to_track_quat(
            "-Z",
            "Y"
        ).to_euler()
    )

    return light


make_area(
    "Key",
    center + Vector((
        -0.35,
        -0.45,
        0.30
    )),
    180,
    0.35
)

make_area(
    "Fill",
    center + Vector((
        0.35,
        -0.20,
        0.10
    )),
    80,
    0.30
)

make_area(
    "Rim",
    center + Vector((
        0.10,
        0.35,
        0.30
    )),
    120,
    0.25
)

# ------------------------------------------------------------
# World/render settings
# ------------------------------------------------------------

scene = bpy.context.scene

scene.render.engine = "BLENDER_EEVEE"

scene.render.resolution_x = 768
scene.render.resolution_y = 768
scene.render.resolution_percentage = 100

scene.render.image_settings.file_format = "PNG"

scene.world.color = (
    0.035,
    0.035,
    0.035
)

distance = max_size * 4.0

# Face direction in MakeHuman is approximately -Y.
views = {
    "front": Vector((
        0.0,
        -distance,
        0.0
    )),

    "side": Vector((
        distance,
        0.0,
        0.0
    )),

    "perspective": Vector((
        distance * 0.70,
        -distance * 0.80,
        distance * 0.25
    )),
}

for name, offset in views.items():

    point_camera(
        center + offset
    )

    scene.render.filepath = str(
        out_dir
        / f"mpfb_head_{name}.png"
    )

    bpy.ops.render.render(
        write_still=True
    )

    print(
        "Saved:",
        scene.render.filepath
    )

print()
print("SUCCESS")
