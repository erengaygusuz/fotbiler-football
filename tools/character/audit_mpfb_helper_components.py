import bpy
from collections import defaultdict, deque

HUMAN_NAME = "MPFB_BaseHuman"
HELPER_GROUP_NAME = "HelperGeometry"

# Sadece kafa bölgesindeki helper geometrisini incele.
MIN_Z = 0.60

human = bpy.data.objects.get(HUMAN_NAME)

if human is None or human.type != "MESH":
    raise RuntimeError(f"{HUMAN_NAME} bulunamadı.")

mesh = human.data

helper_group = human.vertex_groups.get(
    HELPER_GROUP_NAME
)

if helper_group is None:
    raise RuntimeError(
        f"{HELPER_GROUP_NAME} bulunamadı."
    )

helper_indices = set()

for vert in mesh.vertices:
    for membership in vert.groups:
        if membership.group == helper_group.index:
            if vert.co.z >= MIN_Z:
                helper_indices.add(vert.index)
            break

print()
print("=== MPFB HELPER GEOMETRY AUDIT ===")
print("Helper verts above Z=0.60 :", len(helper_indices))

# ------------------------------------------------------------
# Adjacency
# ------------------------------------------------------------

adj = defaultdict(set)

for edge in mesh.edges:
    a, b = edge.vertices

    if a in helper_indices and b in helper_indices:
        adj[a].add(b)
        adj[b].add(a)

# ------------------------------------------------------------
# Components
# ------------------------------------------------------------

unvisited = set(helper_indices)
components = []

while unvisited:
    start = next(iter(unvisited))

    queue = deque([start])
    unvisited.remove(start)

    component = []

    while queue:
        current = queue.popleft()
        component.append(current)

        for neighbour in adj[current]:
            if neighbour in unvisited:
                unvisited.remove(neighbour)
                queue.append(neighbour)

    components.append(component)

components.sort(
    key=len,
    reverse=True
)

print("Components              :", len(components))
print()

# ------------------------------------------------------------
# Bounds / face count
# ------------------------------------------------------------

def component_info(indices):
    index_set = set(indices)

    coords = [
        mesh.vertices[i].co
        for i in indices
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

    size = (
        maxs[0] - mins[0],
        maxs[1] - mins[1],
        maxs[2] - mins[2],
    )

    center = (
        (mins[0] + maxs[0]) * 0.5,
        (mins[1] + maxs[1]) * 0.5,
        (mins[2] + maxs[2]) * 0.5,
    )

    faces = 0
    materials = set()

    for poly in mesh.polygons:
        if all(
            v in index_set
            for v in poly.vertices
        ):
            faces += 1
            materials.add(poly.material_index)

    return mins, maxs, size, center, faces, materials


for number, component in enumerate(
    components[:30],
    start=1
):
    mins, maxs, size, center, faces, materials = (
        component_info(component)
    )

    print(f"COMPONENT {number}")
    print("  vertices  :", len(component))
    print("  faces     :", faces)

    print(
        "  center    :",
        tuple(round(v, 6) for v in center)
    )

    print(
        "  min       :",
        tuple(round(v, 6) for v in mins)
    )

    print(
        "  max       :",
        tuple(round(v, 6) for v in maxs)
    )

    print(
        "  size      :",
        tuple(round(v, 6) for v in size)
    )

    print(
        "  materials :",
        sorted(materials)
    )

    print()

print("SUCCESS")
