import bpy
from collections import defaultdict, deque

HUMAN_NAME = "MPFB_BaseHuman"
BODY_GROUP_NAME = "body"
LEGACY_HEAD_HEIGHT = 0.295

human = bpy.data.objects.get(HUMAN_NAME)

if human is None or human.type != "MESH":
    raise RuntimeError(f"{HUMAN_NAME} bulunamadı.")

mesh = human.data

body_group = human.vertex_groups.get(BODY_GROUP_NAME)

if body_group is None:
    raise RuntimeError(f"{BODY_GROUP_NAME} bulunamadı.")

# ------------------------------------------------------------
# Body surface indices
# ------------------------------------------------------------

body_indices = set()

for vert in mesh.vertices:
    if any(
        membership.group == body_group.index
        for membership in vert.groups
    ):
        body_indices.add(vert.index)

max_z = max(
    mesh.vertices[i].co.z
    for i in body_indices
)

cut_z = max_z - LEGACY_HEAD_HEIGHT

selected = {
    i
    for i in body_indices
    if mesh.vertices[i].co.z >= cut_z
}

print()
print("=== MPFB HEAD COMPONENT AUDIT ===")
print("Body vertices :", len(body_indices))
print("Cut Z         :", cut_z)
print("Above cut     :", len(selected))

# ------------------------------------------------------------
# Adjacency
# ------------------------------------------------------------

adj = defaultdict(set)

for edge in mesh.edges:
    a, b = edge.vertices

    if a in selected and b in selected:
        adj[a].add(b)
        adj[b].add(a)

# ------------------------------------------------------------
# Connected components
# ------------------------------------------------------------

unvisited = set(selected)
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

print("Components    :", len(components))
print()

# ------------------------------------------------------------
# Bounds
# ------------------------------------------------------------

def bounds(indices):
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

    sizes = (
        maxs[0] - mins[0],
        maxs[1] - mins[1],
        maxs[2] - mins[2],
    )

    return mins, maxs, sizes


highest_vertex = max(
    selected,
    key=lambda i: mesh.vertices[i].co.z
)

print(
    "Highest vertex:",
    highest_vertex,
    tuple(
        round(x, 6)
        for x in mesh.vertices[highest_vertex].co
    )
)

print()

for number, component in enumerate(
    components[:20],
    start=1
):
    mins, maxs, sizes = bounds(component)

    contains_top = highest_vertex in component

    print(
        f"COMPONENT {number}"
        + ("  <-- CONTAINS HEAD TOP" if contains_top else "")
    )

    print("  vertices :", len(component))

    print(
        "  min      :",
        tuple(round(x, 6) for x in mins)
    )

    print(
        "  max      :",
        tuple(round(x, 6) for x in maxs)
    )

    print(
        "  size     :",
        tuple(round(x, 6) for x in sizes)
    )

    print()

print("SUCCESS")
