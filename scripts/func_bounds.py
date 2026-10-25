#!/usr/bin/env python3
"""Report the function that contains a VA/RVA, using the MSVC `.pdata` table.

Why this exists: "the disassembly ended in a `ret`" is not evidence that a detour body
ends there. Windows x64 images carry a `RUNTIME_FUNCTION` record for every non-leaf
function in `.pdata` (12 bytes: begin RVA, end RVA, unwind-info RVA, sorted by begin).
Reading that table turns a guess about a body's extent into a measured extent, which is
what `notes/` cites when it says a detour is "complete".

The table is also the cheapest way to find the *start* of a function an address sits in
the middle of - detours are often jumped into after a hot patch, so a linear disassembly
from the hook site is not guaranteed to be aligned.

Usage (from the repository root)::

    python3 scripts/func_bounds.py samples/absolllute.hackmega/absolllute.hackmega_original.dll 0x180172E50
    python3 scripts/func_bounds.py <image.dll> 0x172e50 --rva
    python3 scripts/func_bounds.py <image.dll> --count

Both VAs (image base + RVA) and bare RVAs are accepted; `--rva` forces the second
reading. Read-only, no third-party packages: sections come from `scripts/pe_rva.py`.
"""

from __future__ import annotations

import argparse
import bisect
import struct
import sys
from dataclasses import dataclass
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from pe_rva import Image

PDATA = ".pdata"
ENTRY_SIZE = 12


@dataclass(frozen=True)
class Function:
    begin: int  # RVA
    end: int  # RVA
    unwind: int  # RVA

    @property
    def size(self) -> int:
        return self.end - self.begin


class FunctionTable:
    """The image's `.pdata` records, sorted by begin RVA."""

    def __init__(self, functions: list[Function]) -> None:
        self.functions = sorted(functions, key=lambda f: f.begin)
        self._begins = [f.begin for f in self.functions]

    @classmethod
    def from_image(cls, img: Image) -> "FunctionTable":
        section = next((s for s in img.sections if s.name == PDATA), None)
        if section is None:
            raise ValueError(f"image has no {PDATA} section")
        raw = img.data[section.pointer_to_raw_data:
                       section.pointer_to_raw_data + section.size_of_raw_data]
        records: list[Function] = []
        for offset in range(0, len(raw) - ENTRY_SIZE + 1, ENTRY_SIZE):
            begin, end, unwind = struct.unpack_from("<III", raw, offset)
            if begin == 0 and end == 0:
                continue
            if end <= begin:
                continue
            records.append(Function(begin, end, unwind))
        return cls(records)

    def containing(self, rva: int) -> Function | None:
        index = bisect.bisect_right(self._begins, rva) - 1
        if index < 0:
            return None
        candidate = self.functions[index]
        return candidate if candidate.begin <= rva < candidate.end else None

    def overlaps(self) -> list[tuple[Function, Function]]:
        """Return pairs of records whose ranges intersect (should be empty)."""
        pairs = []
        for previous, current in zip(self.functions, self.functions[1:]):
            if previous.end > current.begin:
                pairs.append((previous, current))
        return pairs


def _resolve(text: str, img: Image, force_rva: bool) -> int:
    value = int(text, 16)
    if force_rva:
        return value
    # A value below the image base can only be an RVA; otherwise treat it as a VA.
    return value - img.image_base if value >= img.image_base else value


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("image", help="path to the sample DLL")
    parser.add_argument("addresses", nargs="*", help="VA or RVA to locate")
    parser.add_argument("--rva", action="store_true", help="treat addresses as RVAs")
    parser.add_argument("--count", action="store_true", help="print the record count only")
    args = parser.parse_args(argv)

    img = Image.from_path(args.image)
    table = FunctionTable.from_image(img)

    if args.count:
        print(f"{len(table.functions)} .pdata records")
        return 0

    if not args.addresses:
        parser.error("give at least one address, or --count")

    status = 0
    for text in args.addresses:
        rva = _resolve(text, img, args.rva)
        function = table.containing(rva)
        if function is None:
            print(f"{text}: not inside any .pdata function")
            status = 1
            continue
        entry = "entry" if rva == function.begin else f"+{rva - function.begin:#x} into it"
        print(
            f"{text}: {img.image_base + function.begin:#x}..{img.image_base + function.end:#x}"
            f" (rva {function.begin:#x}..{function.end:#x}, size {function.size:#x},"
            f" {entry})"
        )
    return status


if __name__ == "__main__":
    raise SystemExit(main())
