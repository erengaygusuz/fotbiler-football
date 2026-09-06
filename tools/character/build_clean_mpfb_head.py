import bpy
import bmesh
from collections import defaultdict, deque
from pathlib import Path

HUMAN_NAME = "MPFB_BaseHuman"
BODY_GROUP_NAME = "body"
HELPER_GROUP_NAME = "HelperGeometry"

CUT_Z = 0.60

human = bpy.data.objects.get(HUMAN_NAME)

if human is None or human.type != "MESH":
    raise RuntimeError(f"{HUMAN_NAME} bulunamadı.")

mesh = human.data

body_group = human.vertex_groups.get(BODY_GROUP_NAME)
helper_group = human.vertex_groups.get(HELPER_GROUP_NAME)

if body_group is None:
    raise RuntimeError("body vertex group bulunamadı.")

if helper_group is None:
    raise RuntimeError("HelperGeometry vertex group bulunamadı.")


# ------------------------------------------------------------
# Helpers
# ------------------------------------------------------------

def group_indices(group):
    result = set()

    for vert in mesh.vertices:
        for membership in vert.groups:
            if membership.group == group.index:
                result.add(vert.index)
                break

    return result


def connected_components(indices):
    indices = set(indices)
    adj = defaultdict(set)

    for edge in mesh.edges:
        a, b = edge.vertices

        if a in indices and b in indices:
            adj[a].add(b)
            adj[b].add(a)

    remaining = set(indices)
    components = []

    while remaining:
        start = next(iter(remaining))
        remaining.remove(start)

        queue = deque([start])
        component = []

        while queue:
            current = queue.popleft()
            component.append(current)

            for other in adj[current]:
                if other in remaining:
                    remaining.remove(other)
                    queue.append(other)

        components.append(component)

    components.sort(key=len, reverse=True)

    return components


def bounds(indices):
    coords = [mesh.vertices[i].co for i in indices]

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

    size = tuple(
        maxs[i] - mins[i]
        for i in range(3)
    )

    center = tuple(
        (mins[i] + maxs[i]) * 0.5
        for i in range(3)
    )

    return mins, maxs, size, center


def create_object_from_vertices(
    name,
    keep_indices,
):
    keep_indices = set(keep_indices)

    new_mesh = mesh.copy()

    bm = bmesh.new()
    bm.from_mesh(new_mesh)

    bm.verts.ensure_lookup_table()
    bm.verts.index_update()

    to_delete = [
        v
        for v in bm.verts
        if v.index not in keep_indices
    ]

    bmesh.ops.delete(
        bm,
        geom=to_delete,
        context="VERTS"
    )

    loose = [
        v
        for v in bm.verts
        if not v.link_faces
    ]

    if loose:
        bmesh.ops.delete(
            bm,
            geom=loose,
            context="VERTS"
        )

    bm.to_mesh(new_mesh)
    bm.free()

    new_mesh.update()

    obj = bpy.data.objects.new(
        name,
        new_mesh
    )

    bpy.context.collection.objects.link(obj)

    obj.matrix_world = human.matrix_world.copy()

    return obj


# ------------------------------------------------------------
# CLEAN HEAD
# ------------------------------------------------------------

body_indices = group_indices(body_group)

head_mesh = mesh.copy()

bm = bmesh.new()
bm.from_mesh(head_mesh)

bm.verts.ensure_lookup_table()
bm.verts.index_update()

# Önce helper/joint dışındaki gerçek body yüzeyini bırak.
to_delete = [
    v
    for v in bm.verts
    if v.index not in body_indices
]

bmesh.ops.delete(
    bm,
    geom=to_delete,
    context="VERTS"
)

# Gerçek bir geometric plane cut yap.
geom = (
    list(bm.verts)
    + list(bm.edges)
    + list(bm.faces)
)

bmesh.ops.bisect_plane(
    bm,
    geom=geom,
    plane_co=(0.0, 0.0, CUT_Z),
    plane_no=(0.0, 0.0, 1.0),
    dist=0.000001,
)

# Bisect sonrası aşağıdaki geometriyi sil.
below = [
    v
    for v in bm.verts
    if v.co.z < CUT_Z - 0.000001
]

if below:
    bmesh.ops.delete(
        bm,
        geom=below,
        context="VERTS"
    )

# Loose verts temizle.
loose = [
    v
    for v in bm.verts
    if not v.link_faces
]

if loose:
    bmesh.ops.delete(
        bm,
        geom=loose,
        context="VERTS"
    )

bm.verts.ensure_lookup_table()
bm.edges.ensure_lookup_table()
bm.faces.ensure_lookup_table()

boundary_edges = [
    edge
    for edge in bm.edges
    if len(edge.link_faces) == 1
]

bm.to_mesh(head_mesh)
bm.free()

head_mesh.update()

head = bpy.data.objects.new(
    "MPFB_FotbilerHead_Clean",
    head_mesh
)

bpy.context.collection.objects.link(head)

head.matrix_world = human.matrix_world.copy()

for poly in head_mesh.polygons:
    poly.use_smooth = True


# ------------------------------------------------------------
# EYEBALL COMPONENTS
# ------------------------------------------------------------

helper_indices = group_indices(helper_group)

head_helpers = {
    i
    for i in helper_indices
    if mesh.vertices[i].co.z >= CUT_Z
}

components = connected_components(
    head_helpers
)

eye_candidates = []

for component in components:
    mins, maxs, size, center = bounds(component)

    sx, sy, sz = size
    cx, cy, cz = center

    # MPFB eyeball heuristic:
    # - eye height
    # - front of head
    # - ~3 cm sphere
    # - mirrored left/right candidates
    if (
        0.70 <= cz <= 0.75
        and -0.15 <= cy <= -0.10
        and 0.020 <= sx <= 0.040
        and 0.020 <= sy <= 0.040
        and 0.020 <= sz <= 0.040
        and abs(cx) >= 0.015
    ):
        eye_candidates.append(
            (component, center, size)
        )

eye_candidates.sort(
    key=lambda item: item[1][0]
)

if len(eye_candidates) != 2:
    print("Eye candidates:", len(eye_candidates))

    for comp, center, size in eye_candidates:
        print(
            " candidate:",
            center,
            size
        )

    raise RuntimeError(
        "Exactly two eyeballs bulunamadı."
    )

left_data, right_data = eye_candidates

eye_left = create_object_from_vertices(
    "MPFB_Eye_Left",
    left_data[0]
)

eye_right = create_object_from_vertices(
    "MPFB_Eye_Right",
    right_data[0]
)


# ------------------------------------------------------------
# Hide original
# ------------------------------------------------------------

human.hide_viewport = True
human.hide_render = True


# ------------------------------------------------------------
# Stats
# ------------------------------------------------------------

head_tris = sum(
    max(1, len(poly.vertices) - 2)
    for poly in head_mesh.polygons
)

print()
print("=== CLEAN MPFB HEAD ===")
print("Vertices       :", len(head_mesh.vertices))
print("Polygons       :", len(head_mesh.polygons))
print("Triangles      :", head_tris)
print("Boundary edges :", len(boundary_edges))
print(
    "UV layers      :",
    [uv.name for uv in head_mesh.uv_layers]
)

for eye in (eye_left, eye_right):
    print()
    print(eye.name)
    print(" Vertices :", len(eye.data.vertices))
    print(" Faces    :", len(eye.data.polygons))
    print(
        " Center   :",
        tuple(
            round(v, 6)
            for v in eye.location
        )
    )


# ------------------------------------------------------------
# Save
# ------------------------------------------------------------

output = (
    Path.cwd()
    / "character_work"
    / "mpfb"
    / "mpfb_clean_head.blend"
)

bpy.ops.wm.save_as_mainfile(
    filepath=str(output)
)

print()
print("Saved:", output)
print("SUCCESS")
