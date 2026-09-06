import bpy
from mathutils import Vector

HEAD_NAMES = [
    "Fotbiler_Runtime_Head_Reference",
    "MPFB_FotbilerHead_Fitted",
]

HAIR_NAME = "Fotbiler_Runtime_Hair_Short01"


def bounds_local(obj):
    pts = [v.co.copy() for v in obj.data.vertices]

    mn = Vector((
        min(p.x for p in pts),
        min(p.y for p in pts),
        min(p.z for p in pts),
    ))
    mx = Vector((
        max(p.x for p in pts),
        max(p.y for p in pts),
        max(p.z for p in pts),
    ))
    return mn, mx


def bounds_world(obj):
    pts = [
        obj.matrix_world @ v.co
        for v in obj.data.vertices
    ]

    mn = Vector((
        min(p.x for p in pts),
        min(p.y for p in pts),
        min(p.z for p in pts),
    ))
    mx = Vector((
        max(p.x for p in pts),
        max(p.y for p in pts),
        max(p.z for p in pts),
    ))
    return mn, mx


def show(name, obj):
    lmn, lmx = bounds_local(obj)
    wmn, wmx = bounds_world(obj)

    print()
    print("=" * 70)
    print(name)
    print("object name :", obj.name)
    print("location    :", tuple(round(x, 9) for x in obj.location))
    print("rotation    :", tuple(round(x, 9) for x in obj.rotation_euler))
    print("scale       :", tuple(round(x, 9) for x in obj.scale))

    print("matrix_world:")
    for row in obj.matrix_world:
        print(" ", tuple(round(x, 9) for x in row))

    print("local min   :", tuple(round(x, 6) for x in lmn))
    print("local max   :", tuple(round(x, 6) for x in lmx))
    print("world min   :", tuple(round(x, 6) for x in wmn))
    print("world max   :", tuple(round(x, 6) for x in wmx))
    print(
        "world center:",
        tuple(round((wmn[i] + wmx[i]) * 0.5, 6) for i in range(3))
    )


hair = bpy.data.objects.get(HAIR_NAME)

if hair is None:
    raise RuntimeError(f"Missing hair: {HAIR_NAME}")

head = None

for name in HEAD_NAMES:
    head = bpy.data.objects.get(name)
    if head:
        break

if head is None:
    candidates = [
        o for o in bpy.data.objects
        if o.type == "MESH"
        and "Head" in o.name
    ]

    print("Head candidates:", [o.name for o in candidates])

    if candidates:
        head = max(
            candidates,
            key=lambda o: len(o.data.vertices)
        )

if head is None:
    raise RuntimeError("Could not find fitted head.")

show("HEAD", head)
show("HAIR", hair)

hmn, hmx = bounds_world(head)
smn, smx = bounds_world(hair)

print()
print("=" * 70)
print("RELATION")

head_center = (hmn + hmx) * 0.5
hair_center = (smn + smx) * 0.5

print(
    "hair center - head center:",
    tuple(round(x, 6) for x in (hair_center - head_center))
)

print()
print("EXPECTED LEGACY RUNTIME HEAD BOUNDS")
print(" min: (-0.083, -0.447, 1.523)")
print(" max: ( 0.083, -0.213, 1.818)")

print()
print("EXPECTED LEGACY SHORT01 LOCAL APPROX")
print(" x: ~ -0.086 .. +0.086")
print(" y: ~ -0.106 .. +0.106")
print(" z: ~ +0.118 .. +0.316")

print()
print("SUCCESS")
