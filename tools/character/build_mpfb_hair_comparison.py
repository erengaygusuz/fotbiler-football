import bpy
import importlib
from pathlib import Path
from mathutils import Matrix

ROOT = Path("/run/media/ereng/files/projects/fotbiler-football")

BASE = (
    ROOT
    / "character_work/asset_packs/work/fotbiler_candidates"
    / "makehuman_system_assets_cc0/hair"
)

HAIRS = [
    ("short01", BASE / "short01/short01.mhclo"),
    ("short02", BASE / "short02/short02.mhclo"),
    ("short03", BASE / "short03/short03.mhclo"),
    ("short04", BASE / "short04/short04.mhclo"),
]

OUT = (
    ROOT
    / "character_work/asset_packs/work"
    / "mpfb_hair_comparison.blend"
)

MPFB_PACKAGE = "bl_ext.blender_org.mpfb"

HumanService = importlib.import_module(
    f"{MPFB_PACKAGE}.services.humanservice"
).HumanService

ObjectService = importlib.import_module(
    f"{MPFB_PACKAGE}.services.objectservice"
).ObjectService


def triangle_count(obj):
    return sum(
        max(0, len(poly.vertices) - 2)
        for poly in obj.data.polygons
    )


# ------------------------------------------------------------
# Find original MPFB basemesh
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

print("Basemesh:", human.name)


# ------------------------------------------------------------
# Build four comparison copies
# ------------------------------------------------------------

spacing = 2.2
start_x = -((len(HAIRS) - 1) * spacing) / 2.0

created = []

for index, (name, mhclo) in enumerate(HAIRS):
    if not mhclo.exists():
        raise RuntimeError(f"Missing: {mhclo}")

    print()
    print("=" * 70)
    print("HAIR:", name)

    fitted = HumanService.add_mhclo_asset(
        str(mhclo),
        human,
        asset_type="Hair",
        subdiv_levels=0,
        material_type="GAMEENGINE",
        set_up_rigging=False,
    )

    if fitted is None:
        raise RuntimeError(f"Failed to load {name}")

    offset_x = start_x + index * spacing
    transform = Matrix.Translation((offset_x, 0.0, 0.0))

    # Human display copy
    hcopy = human.copy()
    hcopy.data = human.data.copy()
    hcopy.name = f"Comparison_Human_{name}"
    hcopy.parent = None
    hcopy.matrix_world = transform @ human.matrix_world
    bpy.context.collection.objects.link(hcopy)

    # Hair display copy
    haircopy = fitted.copy()
    haircopy.data = fitted.data.copy()
    haircopy.name = f"Comparison_Hair_{name}"
    haircopy.parent = None
    haircopy.matrix_world = transform @ fitted.matrix_world
    bpy.context.collection.objects.link(haircopy)

    verts = len(haircopy.data.vertices)
    tris = triangle_count(haircopy)

    print("Vertices :", verts)
    print("Triangles:", tris)
    print("UV maps  :", [uv.name for uv in haircopy.data.uv_layers])

    for mat in haircopy.data.materials:
        if not mat:
            continue

        print("Material :", mat.name)

        if mat.use_nodes and mat.node_tree:
            for node in mat.node_tree.nodes:
                if node.type == "TEX_IMAGE" and node.image:
                    print(
                        "  Texture:",
                        node.image.name,
                        node.image.size[0],
                        "x",
                        node.image.size[1],
                    )

    # Text label
    bpy.ops.object.text_add(
        location=(offset_x, 0.0, 2.35)
    )

    label = bpy.context.object
    label.name = f"Label_{name}"
    label.data.body = f"{name}\n{tris} tri"
    label.data.align_x = "CENTER"
    label.data.size = 0.16
    label.rotation_euler = (1.5708, 0.0, 0.0)

    created.extend([hcopy, haircopy])

    # Remove temporary fitted object; comparison copy remains.
    bpy.data.objects.remove(fitted, do_unlink=True)


# ------------------------------------------------------------
# Hide source human
# ------------------------------------------------------------

human.hide_viewport = True
human.hide_render = True


# ------------------------------------------------------------
# Save
# ------------------------------------------------------------

OUT.parent.mkdir(parents=True, exist_ok=True)

bpy.ops.wm.save_as_mainfile(filepath=str(OUT))

print()
print("SAVED:", OUT)
print("SUCCESS")
