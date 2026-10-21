#!/usr/bin/env python3
"""Pin the CRT initializer-table facts.

Skips itself when no sample set is present, so a fresh clone stays green.

These assertions exist because the count is easy to get wrong in a way that
looks fine: `_callers.tsv` omits zero-caller functions rather than recording a
zero, so "not in the file" must be treated as zero callers, not as unknown.
That mistake silently reports zero orphans and hides the entire effect.
"""

from __future__ import annotations

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from pe_rva import Image  # noqa: E402

from init_table import (  # noqa: E402
    DEFAULT_IMAGE,
    TARGET_IMPORTS,
    collect,
    import_slots,
    load_callers,
    read_pointer,
)

ROOT = pathlib.Path(__file__).resolve().parent.parent
CALLERS = ROOT / "recovered" / "functions" / "_callers.tsv"

UI_TABLE = 0x180062940
EXPECTED_SLOT_INDEX = 11


def main() -> int:
    image_path = pathlib.Path(DEFAULT_IMAGE)
    if not image_path.exists():
        print(f"SKIP: no sample set at {image_path}")
        return 0

    failures: list[str] = []

    def check(cond: bool, label: str, detail: str = "") -> None:
        print(f"{'ok  ' if cond else 'FAIL'}  {label}{'  ' + detail if detail else ''}")
        if not cond:
            failures.append(label)

    img = Image.from_path(image_path)

    # 1. The imports really are present, by name, not by assumed address.
    slots = import_slots(img)
    names = set(slots.values())
    check(
        any(n.endswith("!_initterm") for n in names),
        "_initterm is imported",
    )
    check(
        any(n.endswith("!_initterm_e") for n in names),
        "_initterm_e is imported",
    )

    # 2. The ranges, recovered from the imports alone.
    ranges, _ = collect(img, TARGET_IMPORTS)
    by_sym = {r.symbol: r for r in ranges}
    check("_initterm" in by_sym, "_initterm range recovered", f"{len(ranges)} range(s)")
    check("_initterm_e" in by_sym, "_initterm_e range recovered")

    if "_initterm" in by_sym:
        rng = by_sym["_initterm"]
        check(rng.first == 0x18063F068, "_initterm first", f"{rng.first:#x}")
        check(rng.last == 0x18063F938, "_initterm last", f"{rng.last:#x}")
        live = [read_pointer(img, rng.first + i * 8) for i in range(rng.slots)]
        nz = [v for v in live if v]
        check(len(nz) == 281, "281 live init entries", f"got {len(nz)}")
        check(UI_TABLE in nz, "UI table is an init entry")
        if UI_TABLE in nz:
            idx = nz.index(UI_TABLE)
            check(
                idx == EXPECTED_SLOT_INDEX,
                "UI table is entry 11",
                f"got {idx}",
            )

    if "_initterm_e" in by_sym:
        rng = by_sym["_initterm_e"]
        nz = [read_pointer(img, rng.first + i * 8) for i in range(rng.slots)]
        nz = [v for v in nz if v]
        check(len(nz) == 0, "_initterm_e range is empty", f"got {len(nz)}")

    # 3. The claim the note rests on: absence from _callers.tsv means zero.
    if not CALLERS.exists():
        print(f"SKIP  no _callers.tsv at {CALLERS}; recovered-tree checks skipped")
    else:
        callers = load_callers(CALLERS)
        check(bool(callers), "_callers.tsv parsed", f"{len(callers)} rows")
        if callers and "_initterm" in by_sym:
            rng = by_sym["_initterm"]
            nz = [read_pointer(img, rng.first + i * 8) for i in range(rng.slots)]
            nz = [v for v in nz if v]
            orphan = [v for v in nz if v not in callers]
            check(
                len(orphan) == 280,
                "280 init entries have zero direct callers",
                f"got {len(orphan)}",
            )
            check(
                callers.get(UI_TABLE) is None,
                "UI table has no direct-caller row",
            )

    print()
    if failures:
        print(f"{len(failures)} FAILED: {', '.join(failures)}")
        return 1
    print("all checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())