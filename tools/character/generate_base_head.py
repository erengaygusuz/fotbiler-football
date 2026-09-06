import bpy
import math
from mathutils import Vector

REFERENCE_NAME = "REF_LegacyRuntimeHead"
OUTPUT_NAME = "Fotbiler_BaseHead_POC"

ref = bpy.data.objects.get(REFERENCE_NAME)

if ref is None or ref.type != "MESH":
    raise RuntimeError(
        f"{REFERENCE_NAME} mesh'i bulunamadı."
    )

# ------------------------------------------------------------
# Legacy kafa local bounds
# ------------------------------------------------------------

coords = [v.co.copy() for v in ref.data.vertices]

min_x = min(v.x for v in coords)
max_x = max(v.x for v in coords)

min_y = min(v.y for v in coords)
max_y = max(v.y for v in coords)

min_z = min(v.z for v in coords)
max_z = max(v.z for v in coords)

size_x = max_x - min_x
size_y = max_y - min_y
size_z = max_z - min_z

center = Vector((
    (min_x + max_x) * 0.5,
    (min_y + max_y) * 0.5,
    (min_z + max_z) * 0.5,
))

print()
print("=== LEGACY HEAD ===")
print("Bounds X:", min_x, max_x, "size:", size_x)
print("Bounds Y:", min_y, max_y, "size:", size_y)
print("Bounds Z:", min_z, max_z, "size:", size_z)
print("Center  :", tuple(center))

# Eski PoC varsa sil.
old = bpy.data.objects.get(OUTPUT_NAME)

if old:
    bpy.data.objects.remove(old, do_unlink=True)

# ------------------------------------------------------------
# Base topology
#
# 48 segments x 32 latitude divisions
# ~1490 vertices
# ------------------------------------------------------------

segments = 48
rings = 32

verts = []
faces = []

# Top pole
verts.append(Vector((0, 0, 1)))
top = 0

for ring in range(1, rings):
    theta = math.pi * ring / rings

    st = math.sin(theta)
    ct = math.cos(theta)

    for seg in range(segments):
        phi = 2.0 * math.pi * seg / segments

        verts.append(Vector((
            st * math.cos(phi),
            st * math.sin(phi),
            ct,
        )))

bottom = len(verts)
verts.append(Vector((0, 0, -1)))


def ri(ring, seg):
    return 1 + (ring - 1) * segments + (seg % segments)


# Top
for seg in range(segments):
    faces.append((
        top,
        ri(1, seg),
        ri(1, seg + 1),
    ))

# Middle
for ring in range(1, rings - 1):
    for seg in range(segments):
        faces.append((
            ri(ring, seg),
            ri(ring + 1, seg),
            ri(ring + 1, seg + 1),
            ri(ring, seg + 1),
        ))

# Bottom
for seg in range(segments):
    faces.append((
        ri(rings - 1, seg),
        bottom,
        ri(rings - 1, seg + 1),
    ))

# ------------------------------------------------------------
# Legacy ölçülerine göre insan-kafası benzeri deformasyon
#
# Gameplay Football coordinates:
# X = left/right
# Y = front/back
# Z = vertical
#
# Legacy head front ≈ negative Y.
# ------------------------------------------------------------

rx = size_x * 0.50
ry = size_y * 0.50
rz = size_z * 0.50

new_verts = []

for p in verts:
    nx = p.x
    ny = p.y
    nz = p.z

    x = nx * rx
    y = ny * ry
    z = nz * rz

    # Cranium.
    if nz > 0:
        x *= 1.0 + 0.06 * nz

    # Slightly fuller rear skull.
    if ny > 0:
        y *= 1.07

    # Narrow lower face / jaw.
    if nz < -0.20:
        t = min(
            1.0,
            (-nz - 0.20) / 0.80
        )
        x *= 1.0 - 0.30 * t

    front = max(0.0, -ny)

    # Brow / forehead transition.
    brow = (
        math.exp(-((nx / 0.65) ** 4))
        *
        math.exp(-(((nz - 0.22) / 0.16) ** 2))
    )

    y -= ry * 0.055 * brow * front

    # Nose.
    nose = (
        math.exp(-((nx / 0.19) ** 2))
        *
        math.exp(-(((nz + 0.02) / 0.23) ** 2))
    )

    y -= ry * 0.34 * nose * front

    # Mid-face / muzzle.
    mouth_area = (
        math.exp(-((nx / 0.38) ** 4))
        *
        math.exp(-(((nz + 0.35) / 0.20) ** 2))
    )

    y -= ry * 0.055 * mouth_area * front

    # Chin.
    chin = (
        math.exp(-((nx / 0.36) ** 2))
        *
        math.exp(-(((nz + 0.70) / 0.17) ** 2))
    )

    y -= ry * 0.11 * chin

    # Cheek bones.
    cheek = (
        math.exp(
            -(((abs(nx) - 0.53) / 0.22) ** 2)
        )
        *
        math.exp(
            -(((nz + 0.06) / 0.27) ** 2)
        )
    )

    x *= 1.0 + 0.05 * cheek

    # Narrow lowest part for neck transition.
    if nz < -0.83:
        t = min(
            1.0,
            (-nz - 0.83) / 0.17
        )

        x *= 1.0 - 0.20 * t

    new_verts.append(Vector((
        center.x + x,
        center.y + y,
        center.z + z,
    )))

# ------------------------------------------------------------
# Blender mesh
# ------------------------------------------------------------

mesh = bpy.data.meshes.new(
    OUTPUT_NAME + "_Mesh"
)

mesh.from_pydata(
    new_verts,
    [],
    faces
)

mesh.update()

obj = bpy.data.objects.new(
    OUTPUT_NAME,
    mesh
)

bpy.context.collection.objects.link(obj)

# Legacy OBJ import transform'ını aynen kullan.
obj.matrix_world = ref.matrix_world.copy()

# Smooth shading.
for poly in mesh.polygons:
    poly.use_smooth = True

# Basit materyal.
mat = bpy.data.materials.get("Fotbiler_BaseSkin")

if mat is None:
    mat = bpy.data.materials.new(
        "Fotbiler_BaseSkin"
    )

    mat.diffuse_color = (
        0.55,
        0.32,
        0.22,
        1.0,
    )

obj.data.materials.append(mat)

# Legacy referans tel kafes.
ref.display_type = "WIRE"
ref.show_in_front = True

# Yeni objeyi aktif seç.
bpy.ops.object.select_all(action="DESELECT")

obj.select_set(True)
bpy.context.view_layer.objects.active = obj

triangles = sum(
    len(poly.vertices) - 2
    for poly in mesh.polygons
)

print()
print("=== FOTBILER BASE HEAD ===")
print("Object    :", obj.name)
print("Vertices  :", len(mesh.vertices))
print("Polygons  :", len(mesh.polygons))
print("Triangles :", triangles)
print(
    "Rotation  :",
    tuple(round(v, 4) for v in obj.rotation_euler)
)

if not bpy.data.filepath:
    raise RuntimeError("Blend filepath yok.")

bpy.ops.wm.save_as_mainfile(
    filepath=bpy.data.filepath
)

print("Saved     :", bpy.data.filepath)
print("SUCCESS")
