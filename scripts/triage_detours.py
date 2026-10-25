#!/usr/bin/env python3
"""List the shortest detours in the hook table, with their whole instruction body.

Most Mega Hack toggles are a handful of instructions. Those bodies are decisive on
their own: `xor eax, eax; ret` on `GameManager::isIconUnlocked` cannot mean anything
except "always unlocked". Finding them by hand means opening every row of
`_hook_sites.tsv`, so this tool answers "which detours are small enough to settle in
one reading?" instead.

Method
------
1. Read `recovered/functions/_hook_sites.tsv` (regenerated, not tracked) and take the
   detour symbol from its fifth column.
2. Ask `scripts/func_bounds.py` for the containing `.pdata` function. A detour with an
   unwind record has a measured extent; one without is a code cave, so its extent is
   measured by disassembling until `ret` or an unconditional `jmp` (conditional
   branches are followed through, because a two-branch detour continues past the
   first `jne`).
3. Print the rows at or below `--max` bytes, shortest first.

The GD method name comes from the same TSV and is a target name only: it says nothing
about what the detour does, which is exactly what the printed bytes are for. Tag
attribution is not done here; `scripts/tag_callback_map.py` supplies the registration
site and callback, and `scripts/callback_hooks.py` the hook record for one callback.

Usage (from the repository root)::

    python3 scripts/triage_detours.py samples/absolllute.hackmega/absolllute.hackmega_original.dll
    python3 scripts/triage_detours.py <image.dll> --max 0x10 --limit 40

Reads only local files and exits 0 when `recovered/` is absent, printing a skip line.
"""

from __future__ import annotations

import argparse
import pathlib
import re
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from pe_rva import Image  # noqa: E402

# capstone is vendored inside the sample set, never a tracked dependency, so the import
# waits until the image path says where that set lives (see main()).
capstone = None

ROOT = pathlib.Path(__file__).resolve().parent.parent
HOOK_SITES = ROOT / "recovered" / "functions" / "_hook_sites.tsv"
SYMBOL = re.compile(r"(?:FUN|LAB)_([0-9a-f]+)")
BRANCH_END = ("ret", "jmp")


def body(img: Image, md, va: int, cap: int = 0x40) -> tuple[int, str]:
    section = img.section_for(va - img.image_base)
    if section is None:
        return 0, "<outside the image>"
    offset = section.pointer_to_raw_data + (va - img.image_base - section.virtual_address)
    text: list[str] = []
    size = 0
    for instruction in md.disasm(img.data[offset:offset + cap], va):
        text.append(f"{instruction.mnemonic} {instruction.op_str}".strip())
        size += instruction.size
        if instruction.mnemonic in BRANCH_END:
            break
    return size, "; ".join(text)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("image", help="path to the sample DLL")
    parser.add_argument("--max", default="0x18", help="largest detour to show, bytes (default 0x18)")
    parser.add_argument("--limit", type=int, default=25, help="rows to print (default 25)")
    args = parser.parse_args(argv)

    if not HOOK_SITES.exists():
        print(f"SKIP: no recovered hook table at {HOOK_SITES.relative_to(ROOT)}")
        return 0
    if capstone is None:
        vendored = pathlib.Path(args.image).resolve().parent / ".analysis_tools"
        if vendored.is_dir():
            sys.path.insert(0, str(vendored))
        try:
            import capstone as module
        except ImportError:
            print(f"SKIP: capstone is not importable (looked in {vendored})")
            return 0
        globals()["capstone"] = module

    img = Image.from_path(args.image)
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
    limit = int(args.max, 0)

    rows: list[tuple[int, bool, str, str, str, str]] = []
    for line in HOOK_SITES.read_text(encoding="utf-8").splitlines():
        columns = line.split("\t")
        if len(columns) < 8:
            continue
        match = SYMBOL.search(columns[4])
        if match is None:
            continue
        va = int(match.group(1), 16)
        size, text = body(img, md, va)
        if not 0 < size <= limit:
            continue
        rows.append((size, "cave" in columns[4], columns[1], columns[2], hex(va), text))

    rows.sort(key=lambda row: (row[0], row[2]))
    print(f"{len(rows)} detours <= {limit:#x} bytes")
    for size, _cave, method, rva, va, text in rows[: args.limit]:
        print(f"{size:#4x} {method:44s} rva {rva:>10s} {va} {text}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
