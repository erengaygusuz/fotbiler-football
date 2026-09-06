import bpy
import math
from pathlib import Path
from mathutils import Vector

OBJECT_NAME = "Fotbiler_BaseHead_POC"

obj = bpy.data.objects.get(OBJECT_NAME)

if obj is None or obj.type != "MESH":
    raise RuntimeError(f"{OBJECT_NAME} bulunamadı.")

root = Path.cwd()
out_dir = root / "character_work" / "previews"
out_dir.mkdir(parents=True, exist_ok=True)

# ------------------------------------------------------------
# World-space bounds
# ------------------------------------------------------------

world_vertices = [
    obj.matrix_world @ v.co
    for v in obj.data.vertices
]

min_v = Vector((
    min(v.x for v in world_vertices),
    min(v.y for v in world_vertices),
    min(v.z for v in world_vertices),
))

max_v = Vector((
    max(v.x for v in world_vertices),
    max(v.y for v in world_vertices),
    max(v.z for v in world_vertices),
))

center = (min_v + max_v) * 0.5
size = max_v - min_v
max_size = max(size)

print("World center:", tuple(center))
print("World size  :", tuple(size))

# ------------------------------------------------------------
# Render settings
# ------------------------------------------------------------

scene = bpy.context.scene

scene.render.engine = "BLENDER_EEVEE"

scene.render.resolution_x = 768
scene.render.resolution_y = 768
scene.render.resolution_percentage = 100

scene.render.image_settings.file_format = "PNG"

scene.render.film_transparent = False

scene.world.color = (
    0.055,
    0.055,
    0.055,
)

# ------------------------------------------------------------
# Material
# ------------------------------------------------------------

mat = bpy.data.materials.get("Fotbiler_PreviewSkin")

if mat is None:
    mat = bpy.data.materials.new("Fotbiler_PreviewSkin")

mat.diffuse_color = (
    0.50,
    0.28,
    0.18,
    1.0,
)

if len(obj.data.materials) == 0:
    obj.data.materials.append(mat)
else:
    obj.data.materials[0] = mat

# Legacy reference render dışı.
ref = bpy.data.objects.get("REF_LegacyRuntimeHead")

if ref:
    ref.hide_render = True

# ------------------------------------------------------------
# Camera
# ------------------------------------------------------------

camera_data = bpy.data.cameras.new("PreviewCamera")
camera = bpy.data.objects.new(
    "PreviewCamera",
    camera_data
)

bpy.context.collection.objects.link(camera)

scene.camera = camera

camera.data.lens = 70


def point_camera(target):
    direction = target - camera.location

    camera.rotation_euler = direction.to_track_quat(
        "-Z",
        "Y"
    ).to_euler()


# ------------------------------------------------------------
# Lighting
# ------------------------------------------------------------

def add_area(name, location, energy, size):
    data = bpy.data.lights.new(
        name=name,
        type="AREA"
    )

    data.energy = energy
    data.shape = "DISK"
    data.size = size

    light = bpy.data.objects.new(
        name,
        data
    )

    bpy.context.collection.objects.link(light)

    light.location = location

    direction = center - location

    light.rotation_euler = direction.to_track_quat(
        "-Z",
        "Y"
    ).to_euler()

    return light


distance = max_size * 4.0

add_area(
    "Key",
    center + Vector((
        distance * 0.8,
        -distance * 1.1,
        distance * 0.7,
    )),
    700,
    max_size * 2.0,
)

add_area(
    "Fill",
    center + Vector((
        -distance * 0.9,
        -distance * 0.6,
        distance * 0.25,
    )),
    350,
    max_size * 2.2,
)

add_area(
    "Rim",
    center + Vector((
        0,
        distance,
        distance * 0.8,
    )),
    500,
    max_size * 1.5,
)

# ------------------------------------------------------------
# Object-local axes transformed to world space.
#
# Game head:
# X = left/right
# Y = front/back; face ≈ -Y
# Z = up
# ------------------------------------------------------------

basis = obj.matrix_world.to_3x3()

front_dir = (basis @ Vector((0, -1, 0))).normalized()
side_dir = (basis @ Vector((1, 0, 0))).normalized()

perspective_dir = (
    front_dir * 0.80
    + side_dir * 0.55
    + (basis @ Vector((0, 0, 1))).normalized() * 0.18
).normalized()


views = {
    "front": front_dir,
    "side": side_dir,
    "perspective": perspective_dir,
}


for name, direction in views.items():

    camera.location = (
        center
        + direction * distance
    )

    point_camera(center)

    scene.render.filepath = str(
        out_dir / f"fotbiler_head_{name}.png"
    )

    bpy.ops.render.render(
        write_still=True
    )

    print(
        "Rendered:",
        scene.render.filepath
    )

print("SUCCESS")
