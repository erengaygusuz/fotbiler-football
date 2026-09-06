import bpy
import shutil
from pathlib import Path
from mathutils import Vector

ROOT = Path("/run/media/ereng/files/projects/fotbiler-football")

HAIR_NAME = "Fotbiler_Runtime_Hair_Short01"

OUT_DIR = (
    ROOT
    / "character_work/asset_packs/work/runtime_short01"
)

OUT_ASE = OUT_DIR / "fotbiler_short01.ase"
OUT_TEXTURE = OUT_DIR / "fotbiler_short01.png"

SOURCE_TEXTURE = (
    ROOT
    / "character_work/asset_packs/work/fotbiler_candidates"
    / "makehuman_system_assets_cc0"
    / "hair/short01/short01_diffuse.png"
)

RUNTIME_TEXTURE_PATH = (
    "media/objects/players/textures/hair/fotbiler_short01.png"
)

# Measured from Fotbiler's straight.bind pose:
#
# joint[2] = neck
# position = (0.0, -0.03, 1.61)
#
# HairStyle is positioned at this joint by HumanoidBase.
# Therefore hairstyle ASE vertices must be relative to it.
BIND_JOINT = Vector((0.0, -0.03, 1.61))


def fmt(v):
    if abs(v) < 0.0000005:
        v = 0.0
    return f"{v:.7f}"


def tri_count(mesh):
    mesh.calc_loop_triangles()
    return len(mesh.loop_triangles)


hair = bpy.data.objects.get(HAIR_NAME)

if hair is None:
    raise RuntimeError(
        f"Object not found: {HAIR_NAME}"
    )

if hair.type != "MESH":
    raise RuntimeError(
        f"{HAIR_NAME} is not a mesh"
    )

if not SOURCE_TEXTURE.exists():
    raise RuntimeError(
        f"Texture not found: {SOURCE_TEXTURE}"
    )

mesh = hair.data
mesh.calc_loop_triangles()

if not mesh.uv_layers:
    raise RuntimeError("Hair mesh has no UV map.")

uv_layer = mesh.uv_layers.active

if uv_layer is None:
    raise RuntimeError("No active UV map.")


# ------------------------------------------------------------
# World -> Fotbiler joint-local vertices
# ------------------------------------------------------------

mw = hair.matrix_world.copy()
normal_matrix = mw.to_3x3().inverted().transposed()

vertices = []

for vertex in mesh.vertices:
    world = mw @ vertex.co
    local = world - BIND_JOINT
    vertices.append(local)


# ------------------------------------------------------------
# Vertex normals
# ------------------------------------------------------------

vertex_normals = []

for vertex in mesh.vertices:
    n = normal_matrix @ vertex.normal

    if n.length > 0.0:
        n.normalize()

    vertex_normals.append(n)


# ------------------------------------------------------------
# Triangles and UVs
#
# Use one TVERT per triangle corner. It is slightly larger than
# deduplicated UV data, but completely preserves UV seams and is
# simple/robust for this first runtime PoC.
# ------------------------------------------------------------

triangles = []
tverts = []
tfaces = []

for tri_index, tri in enumerate(mesh.loop_triangles):
    vis = tuple(tri.vertices)

    triangles.append(vis)

    uv_indices = []

    for loop_index in tri.loops:
        uv = uv_layer.data[loop_index].uv

        # ASELoader negates V when loading, so write -V here
        # to preserve Blender's resulting UV coordinate.
        tverts.append(
            (float(uv.x), -float(uv.y), 0.0)
        )

        uv_indices.append(len(tverts) - 1)

    tfaces.append(tuple(uv_indices))


# ------------------------------------------------------------
# Bounds audit
# ------------------------------------------------------------

mn = Vector((
    min(v.x for v in vertices),
    min(v.y for v in vertices),
    min(v.z for v in vertices),
))

mx = Vector((
    max(v.x for v in vertices),
    max(v.y for v in vertices),
    max(v.z for v in vertices),
))


# ------------------------------------------------------------
# ASE
# ------------------------------------------------------------

lines = []

a = lines.append

a("*3DSMAX_ASCIIEXPORT\t200")
a('*COMMENT "Fotbiler runtime hairstyle exporter"')

a("*SCENE {")
a('\t*SCENE_FILENAME "fotbiler_short01.blend"')
a("\t*SCENE_FIRSTFRAME 0")
a("\t*SCENE_LASTFRAME 0")
a("\t*SCENE_FRAMESPEED 30")
a("\t*SCENE_TICKSPERFRAME 160")
a("\t*SCENE_BACKGROUND_STATIC 0.000 0.000 0.000")
a("\t*SCENE_AMBIENT_STATIC 0.000 0.000 0.000")
a("}")

a("*MATERIAL_LIST {")
a("\t*MATERIAL_COUNT 1")
a("\t*MATERIAL 0 {")
a('\t\t*MATERIAL_NAME "Fotbiler Short01 Hair"')
a('\t\t*MATERIAL_CLASS "Standard"')
a("\t\t*MATERIAL_AMBIENT 0.200 0.200 0.200")
a("\t\t*MATERIAL_DIFFUSE 1.000 1.000 1.000")
a("\t\t*MATERIAL_SPECULAR 0.050 0.050 0.050")
a("\t\t*MATERIAL_SHINE 0.050")
a("\t\t*MATERIAL_SHINESTRENGTH 0.010")
a("\t\t*MATERIAL_TRANSPARENCY 0.000")
a("\t\t*MATERIAL_WIRESIZE 1.000")
a("\t\t*MATERIAL_SHADING Blinn")
a("\t\t*MATERIAL_XP_FALLOFF 0.000")
a("\t\t*MATERIAL_SELFILLUM 0.000")
a("\t\t*MATERIAL_FALLOFF In")
a("\t\t*MATERIAL_XP_TYPE Filter")

a("\t\t*MAP_DIFFUSE {")
a('\t\t\t*MAP_NAME "Fotbiler Short01 Diffuse"')
a('\t\t\t*MAP_CLASS "Bitmap"')
a("\t\t\t*MAP_SUBNO 1")
a("\t\t\t*MAP_AMOUNT 1.000")
a(f'\t\t\t*BITMAP "{RUNTIME_TEXTURE_PATH}"')
a("\t\t\t*MAP_TYPE Screen")
a("\t\t\t*UVW_U_OFFSET 0.000")
a("\t\t\t*UVW_V_OFFSET 0.000")
a("\t\t\t*UVW_U_TILING 1.000")
a("\t\t\t*UVW_V_TILING 1.000")
a("\t\t\t*UVW_ANGLE 0.000")
a("\t\t\t*UVW_BLUR 1.000")
a("\t\t\t*UVW_BLUR_OFFSET 0.000")
a("\t\t\t*UVW_NOUSE_AMT 1.000")
a("\t\t\t*UVW_NOISE_SIZE 1.000")
a("\t\t\t*UVW_NOISE_LEVEL 1")
a("\t\t\t*UVW_NOISE_PHASE 0.000")
a("\t\t\t*BITMAP_FILTER Pyramidal")
a("\t\t}")

a("\t}")
a("}")

a("*GEOMOBJECT {")
a('\t*NODE_NAME "fotbiler_short01"')

# Identity NODE_TM.
# ASELoader uses these rows for normal rotation.
a("\t*NODE_TM {")
a('\t\t*NODE_NAME "fotbiler_short01"')
a("\t\t*INHERIT_POS 0 0 0")
a("\t\t*INHERIT_ROT 0 0 0")
a("\t\t*INHERIT_SCL 0 0 0")
a("\t\t*TM_ROW0 1.000 0.000 0.000")
a("\t\t*TM_ROW1 0.000 1.000 0.000")
a("\t\t*TM_ROW2 0.000 0.000 1.000")
a("\t\t*TM_ROW3 0.000 0.000 0.000")
a("\t\t*TM_POS 0.000 0.000 0.000")
a("\t\t*TM_ROTAXIS 0.000 0.000 1.000")
a("\t\t*TM_ROTANGLE 0.000")
a("\t\t*TM_SCALE 1.000 1.000 1.000")
a("\t\t*TM_SCALEAXIS 0.000 0.000 0.000")
a("\t\t*TM_SCALEAXISANG 0.000")
a("\t}")

a("\t*MESH {")
a("\t\t*TIMEVALUE 0")
a(f"\t\t*MESH_NUMVERTEX {len(vertices)}")
a(f"\t\t*MESH_NUMFACES {len(triangles)}")

a("\t\t*MESH_VERTEX_LIST {")

for i, v in enumerate(vertices):
    a(
        f"\t\t\t*MESH_VERTEX {i} "
        f"{fmt(v.x)} {fmt(v.y)} {fmt(v.z)}"
    )

a("\t\t}")

a("\t\t*MESH_FACE_LIST {")

for i, (v0, v1, v2) in enumerate(triangles):
    a(
        f"\t\t\t*MESH_FACE {i}: "
        f"A: {v0} B: {v1} C: {v2} "
        f"AB: 1 BC: 1 CA: 1 "
        f"*MESH_SMOOTHING 1 "
        f"*MESH_MTLID 0"
    )

a("\t\t}")

a(f"\t\t*MESH_NUMTVERTEX {len(tverts)}")

a("\t\t*MESH_TVERTLIST {")

for i, uv in enumerate(tverts):
    a(
        f"\t\t\t*MESH_TVERT {i} "
        f"{fmt(uv[0])} {fmt(uv[1])} {fmt(uv[2])}"
    )

a("\t\t}")

a(f"\t\t*MESH_NUMTVFACES {len(tfaces)}")

a("\t\t*MESH_TFACELIST {")

for i, (a0, a1, a2) in enumerate(tfaces):
    a(
        f"\t\t\t*MESH_TFACE {i} "
        f"{a0} {a1} {a2}"
    )

a("\t\t}")

a("\t\t*MESH_NORMALS {")

for i, tri in enumerate(triangles):
    p0 = vertices[tri[0]]
    p1 = vertices[tri[1]]
    p2 = vertices[tri[2]]

    fn = (p1 - p0).cross(p2 - p0)

    if fn.length > 0.0:
        fn.normalize()

    a(
        f"\t\t\t*MESH_FACENORMAL {i} "
        f"{fmt(fn.x)} {fmt(fn.y)} {fmt(fn.z)}"
    )

    for vi in tri:
        n = vertex_normals[vi]

        a(
            f"\t\t\t\t*MESH_VERTEXNORMAL {vi} "
            f"{fmt(n.x)} {fmt(n.y)} {fmt(n.z)}"
        )

a("\t\t}")
a("\t}")

a("\t*MATERIAL_REF 0")
a("}")


# ------------------------------------------------------------
# Write
# ------------------------------------------------------------

OUT_DIR.mkdir(parents=True, exist_ok=True)

OUT_ASE.write_text(
    "\n".join(lines) + "\n",
    encoding="utf-8",
)

shutil.copy2(
    SOURCE_TEXTURE,
    OUT_TEXTURE,
)


# ------------------------------------------------------------
# Report
# ------------------------------------------------------------

print()
print("=== FOTBILER SHORT01 ASE EXPORT ===")
print("Object      :", hair.name)
print("Vertices    :", len(vertices))
print("Triangles   :", len(triangles))
print("TVerts      :", len(tverts))
print("Texture     :", SOURCE_TEXTURE.name)
print("ASE         :", OUT_ASE)
print("Texture out :", OUT_TEXTURE)

print()
print("Joint-local bounds:")
print(
    " min:",
    tuple(round(x, 6) for x in mn)
)
print(
    " max:",
    tuple(round(x, 6) for x in mx)
)
print(
    " size:",
    tuple(
        round(mx[i] - mn[i], 6)
        for i in range(3)
    )
)

print()
print("Bind joint:", tuple(BIND_JOINT))
print("SUCCESS")
