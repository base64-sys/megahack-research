#!/usr/bin/env python3
"""Check every address cited in the feature inventory against the binary.

The mapped-rows table in ``notes/feature-inventory.md`` is the project's claim about
which toggle is which. A registration VA or callback there is a *measurement*, and a
hand-transcribed one has already been wrong once: four rows carried registration
addresses that did not exist, written from recall rather than read from the tool
output. This script makes that class of error impossible to commit unnoticed.

For each ``| Added ... |`` row it extracts

* the tag (``tag 409 (`0x199` ...)``),
* every ``registered at `0x...` `` address,
* every ``callback `FUN_...` `` address,

and requires them to agree with ``tag_callback_map.registrations()`` and its
nearest-preceding-``lea`` callback rule. Rows that cite no registration are counted
and skipped, not failed.

Usage (repository root, sample set present)::

    python3 scripts/check_inventory_evidence.py samples/absolllute.hackmega/absolllute.hackmega_original.dll

Exit code 1 on any disagreement. No third-party packages.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import tag_callback_map as tcm  # noqa: E402  (tracked, dependency-free)
from pe_rva import Image  # noqa: E402

INVENTORY = Path("notes/feature-inventory.md")

TAG_RE = re.compile(r"tag (\d+) \(`(0x[0-9a-fA-F]+)`")
REG_RE = re.compile(r"registered at `(0x[0-9a-fA-F]+)`")
CB_RE = re.compile(r"callback[s]? `FUN_([0-9a-fA-F]+)`")


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print(
            "usage: python3 scripts/check_inventory_evidence.py "
            "samples/absolllute.hackmega/absolllute.hackmega_original.dll",
            file=sys.stderr,
        )
        return 2
    img = Image.from_path(argv[1])

    by_tag: dict[int, int] = {}
    for va, tag in tcm.registrations(img):
        by_tag.setdefault(tag, va)
    callback_of = {
        tag: tcm._nearest_callback(img, va) for tag, va in by_tag.items()
    }

    text = INVENTORY.read_text(encoding="utf-8")
    problems: list[str] = []
    checked = 0
    skipped = 0

    for line in text.splitlines():
        if not line.startswith("| Added ") or not line.rstrip().endswith("|"):
            continue
        tag_match = TAG_RE.search(line)
        regs = REG_RE.findall(line)
        cbs = ["0x" + c.lower() for c in CB_RE.findall(line)]
        if tag_match is None and not regs:
            skipped += 1
            continue
        if tag_match is None:
            problems.append(f"registration cited without a parseable tag: {line[:90]}")
            continue
        tag = int(tag_match.group(1))
        hex_tag = int(tag_match.group(2), 16)
        if hex_tag != tag:
            problems.append(f"tag {tag} disagrees with its hex form {hex_tag:#x}")
            continue
        if tag not in by_tag:
            # Map A/B config tags are not hook registrations at all; only a row that
            # cites a registration address is claiming one exists.
            if regs:
                problems.append(f"tag {tag}: registration cited but not in the binary")
            else:
                skipped += 1
            continue
        checked += 1
        for reg in regs:
            if int(reg, 16) != by_tag[tag]:
                problems.append(
                    f"tag {tag}: registration {reg} != measured {by_tag[tag]:#x}"
                )
        for cb in cbs:
            measured = callback_of.get(tag)
            if measured is None:
                continue
            if int(cb, 16) != measured:
                problems.append(
                    f"tag {tag}: callback {cb} != measured {measured:#x}"
                )

    print(f"rows checked: {checked}   rows without a registration citation: {skipped}")
    if problems:
        for p in problems:
            print("FAIL:", p)
        return 1
    print("all cited registrations and callbacks match the binary")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
