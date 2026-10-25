#!/usr/bin/env python3
"""Pin the import-thunk resolver against the baseline artifact.

Skips itself when no sample set is present, so a fresh clone stays green.

The assertions exist because this tool is now the source of cocos2d names quoted in
`notes/render-hooks.md` and `notes/gameplay-hooks.md`. A silent shift in the import walk
(descriptor stride, ILT vs IAT, ordinal bit) would rename every call in those notes without
failing anything else, so the totals and a handful of specific resolutions are pinned here.
"""

from __future__ import annotations

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from import_names import imports, stubs  # noqa: E402
from pe_rva import Image  # noqa: E402

ROOT = pathlib.Path(__file__).resolve().parent.parent
IMAGE = ROOT / "samples" / "absolllute.hackmega" / "absolllute.hackmega_original.dll"

# Totals recorded in notes/host-abi.md from the independent `pefile` pass on 2026-10-20.
EXPECTED_IMPORTS = 1010
EXPECTED_DLLS = 30
# `FF 25` byte pairs anywhere in .text; not all of them are stubs.
EXPECTED_HITS = 894
EXPECTED_RESOLVED = 701

KNOWN = {
    0x180532410: "libcocos2d.dll!?count@CCArray@cocos2d@@QEBAIXZ",
    0x1805330B0: "libcocos2d.dll!?clear@CCDrawNode@cocos2d@@QEAAXXZ",
    0x1805331B0: (
        "libcocos2d.dll!?drawPolygon@CCDrawNode@cocos2d@@QEAA_NPEAVCCPoint@2@IAEBU_ccColor4F"
        "@2@M1W4BorderAlignment@2@@Z"
    ),
    0x1805323C0: (
        "libcocos2d.dll!?createWithVariableList@CCSequence@cocos2d@@SAPEAV12@PEAVCCFiniteTime"
        "Action@2@PEAD@Z"
    ),
}


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
    table = imports(img)
    found = stubs(img)
    dlls = {dll for dll, _ in table.values()}
    resolved = {va: table[slot] for va, slot in found.items() if slot in table}

    check(len(table) == EXPECTED_IMPORTS, "import count", f"{len(table)}")
    check(len(dlls) == EXPECTED_DLLS, "DLL count", f"{len(dlls)}")
    check(len(found) == EXPECTED_HITS, "FF 25 hit count", f"{len(found)}")
    check(len(resolved) == EXPECTED_RESOLVED, "resolveable stubs", f"{len(resolved)}")

    for va, expected in KNOWN.items():
        actual = resolved.get(va)
        rendered = f"{actual[0]}!{actual[1]}" if actual else "unresolved"
        check(rendered == expected, f"{va:#x}", rendered)

    # An IAT slot passed directly must resolve to the same symbol as its stub.
    stub = 0x180532410
    slot = found.get(stub)
    same = slot is not None and table.get(slot) == resolved.get(stub)
    check(same, "stub and IAT slot agree", f"slot {slot:#x}" if slot else "no slot")

    print()
    if failures:
        print(f"{len(failures)} check(s) failed: {', '.join(failures)}")
        return 1
    print("all import-name checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
