import bpy
import importlib
import sys
from pathlib import Path


def dynamic_import(absolute_package_str, key):
    """
    MPFB is a Blender Extension.

    Extension packages may actually be loaded under a name such as:
        bl_ext.blender_org.mpfb....

    Find the already-loaded MPFB module by suffix.
    """

    for module_name in list(sys.modules.keys()):

        if module_name.endswith(absolute_package_str):

            module = importlib.import_module(
                module_name
            )

            if not hasattr(module, key):
                raise AttributeError(
                    f"{module_name} has no attribute {key}"
                )

            print(
                f"Resolved {absolute_package_str}"
                f" -> {module_name}"
            )

            return getattr(module, key)

    raise RuntimeError(
        f"MPFB module not found: {absolute_package_str}"
    )


print()
print("=== MPFB MODULES ===")

for name in sorted(sys.modules):
    if "mpfb" in name.lower():
        print(name)


HumanService = dynamic_import(
    "mpfb.services.humanservice",
    "HumanService"
)


# ------------------------------------------------------------
# Clean default Blender scene
# ------------------------------------------------------------

bpy.ops.object.select_all(
    action="SELECT"
)

bpy.ops.object.delete(
    use_global=False
)


# ------------------------------------------------------------
# Create actual MakeHuman / MPFB basemesh
#
# No rig.
# No clothing.
# No texture.
#
# This is only the geometry compatibility test.
# ------------------------------------------------------------

human = HumanService.create_human(
    mask_helpers=True,
    detailed_helpers=False,
    extra_vertex_groups=False,
    feet_on_ground=False,
    scale=0.1,
)

if human is None:
    raise RuntimeError(
        "HumanService.create_human() returned None"
    )

human.name = "MPFB_BaseHuman"


# ------------------------------------------------------------
# Statistics
# ------------------------------------------------------------

mesh = human.data

print()
print("=== MPFB HUMAN ===")

print("Object       :", human.name)
print("Vertices     :", len(mesh.vertices))
print("Edges        :", len(mesh.edges))
print("Polygons     :", len(mesh.polygons))

triangles = sum(
    max(1, len(poly.vertices) - 2)
    for poly in mesh.polygons
)

print("Triangles    :", triangles)

print(
    "Dimensions   :",
    tuple(
        round(v, 6)
        for v in human.dimensions
    )
)

print(
    "Location     :",
    tuple(
        round(v, 6)
        for v in human.location
    )
)

print()
print("=== VERTEX GROUPS ===")

if human.vertex_groups:
    for group in human.vertex_groups:
        print(group.name)
else:
    print("(none)")


# ------------------------------------------------------------
# Bounds
# ------------------------------------------------------------

coords = [
    v.co
    for v in mesh.vertices
]

mins = (
    min(v.x for v in coords),
    min(v.y for v in coords),
    min(v.z for v in coords),
)

maxs = (
    max(v.x for v in coords),
    max(v.y for v in coords),
    max(v.z for v in coords),
)

print()
print("=== LOCAL BOUNDS ===")
print(
    "min:",
    tuple(round(v, 6) for v in mins)
)
print(
    "max:",
    tuple(round(v, 6) for v in maxs)
)


# ------------------------------------------------------------
# Save
# ------------------------------------------------------------

root = Path.cwd()

output = (
    root
    / "character_work"
    / "mpfb"
    / "mpfb_base_human.blend"
)

output.parent.mkdir(
    parents=True,
    exist_ok=True
)

bpy.ops.wm.save_as_mainfile(
    filepath=str(output)
)

print()
print("Saved:", output)
print("SUCCESS")
