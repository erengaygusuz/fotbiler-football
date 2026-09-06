import bpy
from mathutils import Vector


def bounds(obj):
    pts = [
        obj.matrix_world @ Vector(corner)
        for corner in obj.bound_box
    ]

    xs = [p.x for p in pts]
    ys = [p.y for p in pts]
    zs = [p.z for p in pts]

    mn = (
        min(xs),
        min(ys),
        min(zs),
    )

    mx = (
        max(xs),
        max(ys),
        max(zs),
    )

    size = (
        mx[0] - mn[0],
        mx[1] - mn[1],
        mx[2] - mn[2],
    )

    center = (
        (mn[0] + mx[0]) * 0.5,
        (mn[1] + mx[1]) * 0.5,
        (mn[2] + mx[2]) * 0.5,
    )

    return mn, mx, size, center


print()
print("=== PES IMPORTED MESH AUDIT ===")

for obj in sorted(
    [o for o in bpy.data.objects if o.type == "MESH"],
    key=lambda o: o.name
):
    mesh = obj.data

    mn, mx, size, center = bounds(obj)

    armatures = [
        m.object.name
        for m in obj.modifiers
        if m.type == "ARMATURE" and m.object
    ]

    group_stats = {}

    for v in mesh.vertices:
        for assignment in v.groups:
            stat = group_stats.setdefault(
                assignment.group,
                [0, 0.0]
            )

            stat[0] += 1
            stat[1] += assignment.weight

    ranked = sorted(
        group_stats.items(),
        key=lambda item: (
            item[1][0],
            item[1][1],
        ),
        reverse=True
    )

    print()
    print("=" * 70)
    print("OBJECT:", obj.name)
    print("vertices :", len(mesh.vertices))
    print("polygons :", len(mesh.polygons))
    print(
        "triangles:",
        sum(
            max(0, len(p.vertices) - 2)
            for p in mesh.polygons
        )
    )

    print(
        "materials:",
        [m.name for m in mesh.materials]
    )

    print(
        "UV layers:",
        [uv.name for uv in mesh.uv_layers]
    )

    print(
        "parent:",
        obj.parent.name if obj.parent else None
    )

    print(
        "armature modifiers:",
        armatures
    )

    print("bounds min :", mn)
    print("bounds max :", mx)
    print("bounds size:", size)
    print("center     :", center)

    print()
    print("Top vertex groups:")

    for index, (count, total) in ranked[:20]:
        try:
            name = obj.vertex_groups[index].name
        except Exception:
            name = f"group_{index}"

        print(
            f"  {name:<35}"
            f" vertices={count:<6}"
            f" total_weight={total:.3f}"
        )

print()
print("SUCCESS")
