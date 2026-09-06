#!/usr/bin/env python3

from pathlib import Path
from collections import Counter, defaultdict
import zipfile
import sys


ARCHIVES = [Path(x) for x in sys.argv[1:]]

if not ARCHIVES:
    raise SystemExit("Usage: audit_makehuman_archives.py archive.zip ...")

KEYWORDS = [
    "hair",
    "beard",
    "moustache",
    "mustache",
    "stubble",
    "eyebrow",
    "eyelash",
    "eye",
    "skin",
    "afro",
    "braid",
    "dread",
    "ponytail",
    "short",
    "long",
    "curly",
    "old",
    "young",
    "asian",
    "african",
    "european",
]


for archive in ARCHIVES:
    print()
    print("=" * 90)
    print("ARCHIVE:", archive)

    if not archive.exists():
        print("MISSING")
        continue

    if not zipfile.is_zipfile(archive):
        print("NOT ZIP / INVALID")
        continue

    with zipfile.ZipFile(archive) as z:
        names = [
            n for n in z.namelist()
            if not n.endswith("/")
        ]

        ext_counter = Counter()
        hits = defaultdict(list)

        for name in names:
            p = Path(name)
            ext = p.suffix.lower() or "<noext>"
            ext_counter[ext] += 1

            low = name.lower()

            for word in KEYWORDS:
                if word in low and len(hits[word]) < 25:
                    hits[word].append(name)

        print("Files:", len(names))

        print()
        print("Extensions:")
        for ext, count in ext_counter.most_common(25):
            print(f"  {ext:<12} {count:>6}")

        print()
        print("Character-related hits:")

        for word in KEYWORDS:
            values = hits[word]

            if not values:
                continue

            print()
            print(f"[{word}] {len(values)} shown")

            for value in values:
                print(" ", value)

print()
print("SUCCESS")
