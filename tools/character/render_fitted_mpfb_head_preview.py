import bpy
from pathlib import Path
from mathutils import Vector

HEAD_NAME = "MPFB_FotbilerHead_Fitted"
EYE_NAMES = (
    "MPFB_Eye_Left_Fitted",
    "MPFB_Eye_Right_Fitted",
)

head = bpy.data.objects.get(HEAD_NAME)

if head is None:
    raise RuntimeError(f"{HEAD_NAME} bulunamadı.")

eyes = []

for name in EYE_NAMES:
    obj = bpy.data.objects.get(name)

    if obj is None:
        raise RuntimeError(f"{name} bulunamadı.")

    eyes.append(obj)

objects = [head] + eyes

# ------------------------------------------------------------
# Hide unrelated geometry
# ------------------------------------------------------------

for obj in bpy.context.scene.objects:
    if obj.type == "MESH":
        obj.hide_render = obj not in objects
        obj.hide_viewport = obj not in objects

# ------------------------------------------------------------
# Materials
# ------------------------------------------------------------

def material(name, base, roughness):
    mat = bpy.data.materials.get(name)

    if mat is None:
        mat = bpy.data.materials.new(name)
        mat.use_nodes = True

    bsdf = mat.node_tree.nodes.get("Principled BSDF")

    if bsdf:
        bsdf.inputs["Base Color"].default_value = (*base, 1.0)
        bsdf.inputs["Roughness"].default_value = roughness

    return mat


skin = material(
    "PreviewSkin",
    (0.34, 0.28, 0.24),
    0.72
)

eye_white = material(
    "PreviewEye",
    (0.65, 0.65, 0.65),
    0.35
)

head.data.materials.clear()
head.data.materials.append(skin)

for eye in eyes:
    eye.data.materials.clear()
    eye.data.materials.append(eye_white)

    for poly in eye.data.polygons:
        poly.use_smooth = True

# ------------------------------------------------------------
# Combined world bounds
# ------------------------------------------------------------

points = []

for obj in objects:
    points.extend(
        obj.matrix_world @ Vector(corner)
        for corner in obj.bound_box
    )

min_v = Vector((
    min(v.x for v in points),
    min(v.y for v in points),
    min(v.z for v in points),
))

max_v = Vector((
    max(v.x for v in points),
    max(v.y for v in points),
    max(v.z for v in points),
))

center = (min_v + max_v) * 0.5
size = max_v - min_v
max_size = max(size)

print()
print("=== CLEAN HEAD PREVIEW ===")
print("Center :", tuple(round(v, 6) for v in center))
print("Size   :", tuple(round(v, 6) for v in size))

# ------------------------------------------------------------
# Eye geometry stats
# ------------------------------------------------------------

for eye in eyes:
    verts = [
        eye.matrix_world @ v.co
        for v in eye.data.vertices
    ]

    eye_min = Vector((
        min(v.x for v in verts),
        min(v.y for v in verts),
        min(v.z for v in verts),
    ))

    eye_max = Vector((
        max(v.x for v in verts),
        max(v.y for v in verts),
        max(v.z for v in verts),
    ))

    eye_center = (eye_min + eye_max) * 0.5

    print(
        eye.name,
        "center =",
        tuple(round(v, 6) for v in eye_center)
    )

# ------------------------------------------------------------
# Camera
# ------------------------------------------------------------

camera_data = bpy.data.cameras.new("PreviewCamera")
camera = bpy.data.objects.new(
    "PreviewCamera",
    camera_data
)

bpy.context.collection.objects.link(camera)

scene = bpy.context.scene
scene.camera = camera

camera.data.type = "ORTHO"
camera.data.ortho_scale = max_size * 1.28


def point_camera(location):
    camera.location = location

    direction = center - location

    camera.rotation_euler = (
        direction.to_track_quat(
            "-Z",
            "Y"
        ).to_euler()
    )

# ------------------------------------------------------------
# Lights
# ------------------------------------------------------------

def add_area(name, offset, energy, area_size):
    data = bpy.data.lights.new(
        name,
        type="AREA"
    )

    data.energy = energy
    data.shape = "DISK"
    data.size = area_size

    light = bpy.data.objects.new(
        name,
        data
    )

    bpy.context.collection.objects.link(light)

    light.location = center + Vector(offset)

    light.rotation_euler = (
        (center - light.location)
        .to_track_quat("-Z", "Y")
        .to_euler()
    )


add_area(
    "Key",
    (-0.35, -0.45, 0.30),
    180,
    0.35
)

add_area(
    "Fill",
    (0.35, -0.20, 0.10),
    80,
    0.30
)

add_area(
    "Rim",
    (0.10, 0.35, 0.30),
    120,
    0.25
)

# ------------------------------------------------------------
# Render
# ------------------------------------------------------------

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

out_dir = (
    Path.cwd()
    / "character_work"
    / "mpfb"
    / "fitted_previews"
)

out_dir.mkdir(
    parents=True,
    exist_ok=True
)

distance = max_size * 4.0

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
        / f"fitted_head_{name}.png"
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
