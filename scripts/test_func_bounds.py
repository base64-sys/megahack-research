#!/usr/bin/env python3
"""Pin the `.pdata` function-bounds reader against the baseline artifact.

Skips itself when no sample set is present, so a fresh clone stays green.

The notes quote detour extents ("complete, 0x6E bytes, last instruction is the tail
jump") that exist only because of this table. If the record stride, the sort order or the
containment test drifted, those extents would silently become fiction, so each property is
asserted here rather than assumed.
"""

from __future__ import annotations

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from func_bounds import FunctionTable  # noqa: E402
from pe_rva import Image  # noqa: E402

ROOT = pathlib.Path(__file__).resolve().parent.parent
IMAGE = ROOT / "samples" / "absolllute.hackmega" / "absolllute.hackmega_original.dll"

EXPECTED_RECORDS = 19689

# (VA, expected begin VA, expected end VA) - all three are cited in notes/.
KNOWN = [
    (0x180172E50, 0x180172E50, 0x180172EBE),  # Stop Triggers on Death detour
    (0x180166AF0, 0x180166AF0, 0x180166C33),  # Wave Trail Fix detour
    (0x180166B50, 0x180166AF0, 0x180166C33),  # middle of that detour
]


def main() -> int:
    if not IMAGE.exists():
        print(f"SKIP: no sample set at {IMAGE.relative_to(ROOT)}")
        return 0

    failures: list[str] = []

    def check(condition: bool, label: str, detail: str = "") -> None:
        print(f"{'ok  ' if condition else 'FAIL'}  {label}{'  ' + detail if detail else ''}")
        if not condition:
            failures.append(label)

    img = Image.from_path(IMAGE)
    table = FunctionTable.from_image(img)

    check(len(table.functions) == EXPECTED_RECORDS, "record count", f"{len(table.functions)}")
    begins = [f.begin for f in table.functions]
    check(begins == sorted(begins), "records sorted by begin")
    check(not table.overlaps(), "no overlapping ranges")

    for va, begin, end in KNOWN:
        function = table.containing(va - img.image_base)
        actual = (img.image_base + function.begin, img.image_base + function.end) if function else None
        check(actual == (begin, end), f"{va:#x}", f"{tuple(hex(v) for v in actual)}")

    # A string literal is not a function; the reader must say so rather than clamp.
    outside = table.containing(0x18057A894 - img.image_base)
    check(outside is None, "non-text address is not a function")

    print()
    if failures:
        print(f"{len(failures)} check(s) failed: {', '.join(failures)}")
        return 1
    print("all function-bounds checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
