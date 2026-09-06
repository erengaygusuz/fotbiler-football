#!/usr/bin/env python3

from pathlib import Path
import zipfile

OUT = Path(
    "character_work/asset_packs/work/fotbiler_candidates"
)

SELECTIONS = {
    Path("/home/ereng/Downloads/makehuman_system_assets_cc0.zip"): [
        "hair/short01/",
        "hair/short02/",
        "hair/short03/",
        "hair/short04/",
        "hair/afro01/",
    ],

    Path("/home/ereng/Downloads/hair01_cc0.zip"): [
        "hair/cortu_short_messy_hair/",
    ],

    Path("/home/ereng/Downloads/hair02_ccby.zip"): [
        "hair/elvs_short_side_do/",
        "hair/elvs_micky_afro/",
        "hair/elvs_braided_rows/",
    ],

    Path("/home/ereng/Downloads/bodyparts05_cc0.zip"): [
        "clothes/wdg_scruffy_beard/",
        "clothes/rehmanpolanski_moustache_viking/",
    ],
}


for archive, prefixes in SELECTIONS.items():
    print()
    print("ARCHIVE:", archive)

    if not archive.exists():
        print("  MISSING")
        continue

    target = OUT / archive.stem
    target.mkdir(parents=True, exist_ok=True)

    with zipfile.ZipFile(archive) as z:
        names = z.namelist()

        selected = [
            n
            for n in names
            if any(n.startswith(prefix) for prefix in prefixes)
            and not n.endswith("/")
        ]

        for name in selected:
            z.extract(name, target)
            print(" ", name)

        print("Extracted:", len(selected))

print()
print("SUCCESS")
