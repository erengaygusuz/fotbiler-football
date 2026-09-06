import bpy
from pathlib import Path

HEAD_NAME = "MPFB_FotbilerHead_Clean"
EYE_NAMES = (
    "MPFB_Eye_Left",
    "MPFB_Eye_Right",
)

# Legacy fullbody.ase head bounds
TARGET_MIN = (-0.0830, -0.4470, 1.5230)
TARGET_MAX = ( 0.0830, -0.2130, 1.8180)


def get_object(name):
    obj = bpy.data.objects.get(name)

    if obj is None or obj.type != "MESH":
        raise RuntimeError(f"{name} bulunamadı.")

    return obj


head = get_object(HEAD_NAME)
eyes = [get_object(name) for name in EYE_NAMES]
all_objects = [head] + eyes


# ------------------------------------------------------------
# Source bounds: only from head mesh
# ------------------------------------------------------------

coords = [v.co.copy() for v in head.data.vertices]

src_min = (
    min(v.x for v in coords),
    min(v.y for v in coords),
    min(v.z for v in coords),
)

src_max = (
    max(v.x for v in coords),
    max(v.y for v in coords),
    max(v.z for v in coords),
)

src_size = tuple(src_max[i] - src_min[i] for i in range(3))
tgt_size = tuple(TARGET_MAX[i] - TARGET_MIN[i] for i in range(3))

print()
print("=== SOURCE HEAD BOUNDS ===")
print("min :", tuple(round(v, 6) for v in src_min))
print("max :", tuple(round(v, 6) for v in src_max))
print("size:", tuple(round(v, 6) for v in src_size))

print()
print("=== TARGET LEGACY BOUNDS ===")
print("min :", TARGET_MIN)
print("max :", TARGET_MAX)
print("size:", tgt_size)

for size in src_size:
    if abs(size) < 1e-9:
        raise RuntimeError("Geçersiz source bound.")


def remap(coord):
    x, y, z = coord

    nx = (x - src_min[0]) / src_size[0]
    ny = (y - src_min[1]) / src_size[1]
    nz = (z - src_min[2]) / src_size[2]

    return (
        TARGET_MIN[0] + nx * tgt_size[0],
        TARGET_MIN[1] + ny * tgt_size[1],
        TARGET_MIN[2] + nz * tgt_size[2],
    )


# ------------------------------------------------------------
# Apply fit to head + eyes
# ------------------------------------------------------------

for obj in all_objects:
    mesh = obj.data

    for v in mesh.vertices:
        x, y, z = remap(v.co)
        v.co.x = x
        v.co.y = y
        v.co.z = z

    mesh.update()

    obj.location = (0.0, 0.0, 0.0)
    obj.rotation_euler = (0.0, 0.0, 0.0)
    obj.scale = (1.0, 1.0, 1.0)


# ------------------------------------------------------------
# Rename for fitted stage
# ------------------------------------------------------------

head.name = "MPFB_FotbilerHead_Fitted"

if len(eyes) == 2:
    eyes[0].name = "MPFB_Eye_Left_Fitted"
    eyes[1].name = "MPFB_Eye_Right_Fitted"


# ------------------------------------------------------------
# Report fitted bounds
# ------------------------------------------------------------

new_coords = [v.co.copy() for v in head.data.vertices]

fit_min = (
    min(v.x for v in new_coords),
    min(v.y for v in new_coords),
    min(v.z for v in new_coords),
)

fit_max = (
    max(v.x for v in new_coords),
    max(v.y for v in new_coords),
    max(v.z for v in new_coords),
)

fit_size = tuple(fit_max[i] - fit_min[i] for i in range(3))

print()
print("=== FITTED HEAD BOUNDS ===")
print("min :", tuple(round(v, 6) for v in fit_min))
print("max :", tuple(round(v, 6) for v in fit_max))
print("size:", tuple(round(v, 6) for v in fit_size))

for obj in [head] + eyes:
    print()
    print(obj.name)
    print(" vertices :", len(obj.data.vertices))
    print(" faces    :", len(obj.data.polygons))


# ------------------------------------------------------------
# Save
# ------------------------------------------------------------

output = (
    Path.cwd()
    / "character_work"
    / "mpfb"
    / "mpfb_fitted_head.blend"
)

bpy.ops.wm.save_as_mainfile(
    filepath=str(output)
)

print()
print("Saved:", output)
print("SUCCESS")
