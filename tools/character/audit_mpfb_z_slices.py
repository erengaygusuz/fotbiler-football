import bpy

HUMAN_NAME = "MPFB_BaseHuman"
BODY_GROUP_NAME = "body"

human = bpy.data.objects.get(HUMAN_NAME)

if human is None or human.type != "MESH":
    raise RuntimeError(f"{HUMAN_NAME} bulunamadı.")

mesh = human.data
body_group = human.vertex_groups.get(BODY_GROUP_NAME)

if body_group is None:
    raise RuntimeError(f"{BODY_GROUP_NAME} bulunamadı.")

body_indices = set()

for vert in mesh.vertices:
    if any(
        membership.group == body_group.index
        for membership in vert.groups
    ):
        body_indices.add(vert.index)

print()
print("=== MPFB Z SLICE AUDIT ===")
print("Body vertices:", len(body_indices))
print()

# Head/neck/shoulder region only.
z_start = 0.50
z_end = 0.85
step = 0.01
half = step / 2.0

print(
    f"{'Z':>6} "
    f"{'verts':>6} "
    f"{'width X':>10} "
    f"{'depth Y':>10} "
    f"{'min X':>9} "
    f"{'max X':>9}"
)

print("-" * 60)

z = z_end

while z >= z_start - 1e-9:

    indices = [
        i
        for i in body_indices
        if abs(mesh.vertices[i].co.z - z) <= half
    ]

    if indices:
        xs = [
            mesh.vertices[i].co.x
            for i in indices
        ]

        ys = [
            mesh.vertices[i].co.y
            for i in indices
        ]

        width = max(xs) - min(xs)
        depth = max(ys) - min(ys)

        print(
            f"{z:6.2f} "
            f"{len(indices):6d} "
            f"{width:10.5f} "
            f"{depth:10.5f} "
            f"{min(xs):9.5f} "
            f"{max(xs):9.5f}"
        )

    z -= step

print()
print("SUCCESS")
