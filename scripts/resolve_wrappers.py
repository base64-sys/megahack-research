"""Map Mega Hack's runtime hook-wrapper indirection layer.

Mega Hack calls ~279 sites through a thunk that looks like this in the recovered
source:

    HMODULE base = GetModuleHandleA(NULL);              // FUN_18010ddc0
    auto  meta  = (WrapperMetadata*)(base + 0x132e50);  // static record
    auto  fn    = FUN_1801105e0(meta);                  // geode::hook::createWrapper
    fn(args);                                           // indirect call

`FUN_18010DDC0` is literally `GetModuleHandleA(NULL)`, so the offset is an RVA
from the image base. The target of each call is not statically visible: it is
encoded in a tulip WrapperMetadata record that Geode turns into a trampoline at
runtime.

This script finds every such thunk, records the metadata RVA, and reports which
metadata records land inside a recovered function. Output is a TSV next to the
recovered tree, plus a short summary. Read-only; writes nothing but the table.

    python scripts/resolve_wrappers.py
"""

from __future__ import annotations

import argparse
import collections
import pathlib
import re
import sys

REPO = pathlib.Path(__file__).resolve().parent.parent
FUNCTIONS = REPO / "recovered" / "functions"

# `lVar1 = FUN_18010ddc0(...);` -> the module base lands in a named local.
BASE_ASSIGN_RE = re.compile(r"(\w+)\s*=\s*FUN_18010ddc0\s*\(")
# `... = (code *)FUN_xxx(lVar1 + 0x132e50);` cached wrapper pointer
CACHED_RE = re.compile(r"FUN_[0-9a-f]+\(\s*(\w+)\s*\+\s*(0x[0-9a-f]+)\s*\)")
BASE_CALL_RE = re.compile(r"FUN_18010ddc0")
IMAGE_SIZE = 0x867000


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--functions", type=pathlib.Path, default=FUNCTIONS)
    parser.add_argument("--out", type=pathlib.Path, default=None)
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args(argv)

    if not args.functions.is_dir():
        raise SystemExit(f"no recovered tree at {args.functions}; run scripts/decompile_all.py first")

    files = sorted(args.functions.glob("*.c"))
    if not files:
        raise SystemExit(f"no .c files in {args.functions}")

    rows = []
    base_callers = []
    no_base_var = 0

    for path in files:
        text = path.read_text(encoding="utf-8", errors="replace")
        if not BASE_CALL_RE.search(text):
            continue
        base_callers.append(path.stem)

        bases = {m.group(1) for m in BASE_ASSIGN_RE.finditer(text)}
        if not bases:
            no_base_var += 1
            continue
        for match in CACHED_RE.finditer(text):
            basevar, rva = match.group(1), int(match.group(2), 16)
            if basevar not in bases:
                continue
            if not (0x1000 <= rva < IMAGE_SIZE):
                continue
            resolver = match.group(0).split("(")[0]
            rows.append((path.stem, resolver, rva))

    # Map each recovered function entry to its extent so a hook target that lands
    # inside a function body can still be attributed. Filenames carry the full
    # preferred-base VA, so subtract the image base to get an RVA.
    image_base = 0x180000000
    ranges = []
    for path in files:
        head = path.read_text(encoding="utf-8", errors="replace")[:1200]
        m = re.match(r"([0-9a-f]+)_", path.stem)
        if not m:
            continue
        va = int(m.group(1), 16)
        if va < image_base:
            continue
        start = va - image_base
        size = re.search(r"\*\s*size\s*:\s*(\d+)\s*bytes", head)
        end = start + (int(size.group(1)) if size else 0)
        ranges.append((start, end, path.stem))
    ranges.sort()

    def containing(rva: int) -> str:
        lo, hi = 0, len(ranges)
        while lo < hi:
            mid = (lo + hi) // 2
            if ranges[mid][0] <= rva:
                lo = mid + 1
            else:
                hi = mid
        if lo and ranges[lo - 1][0] <= rva < ranges[lo - 1][1]:
            start, _, stem = ranges[lo - 1]
            return f"{stem}+{rva - start:#x}"
        return "-"

    out = args.out or (args.functions / "_wrappers.tsv")
    with out.open("w", encoding="utf-8") as fh:
        fh.write("# thunk\tresolver\thook_target_rva\thook_target_va\tcontains\n")
        for stem, resolver, rva in sorted(set(rows)):
            fh.write(
                f"{stem}\t{resolver}\t{rva:#x}\t{0x180000000 + rva:#x}\t{containing(rva)}\n"
            )

    if not args.quiet:
        uniq = sorted(set(rows))
        unique_rvas = sorted({r for _, _, r in uniq})
        inside = [r for r in unique_rvas if containing(r) != "-"]
        print(f"recovered functions scanned : {len(files)}")
        print(f"thunk functions             : {len(base_callers)}  (call GetModuleHandleA)")
        print(f"  base var not recovered    : {no_base_var}")
        print(f"hook sites                  : {len(uniq)}")
        print(f"distinct hook targets       : {len(unique_rvas)}")
        print(f"targets inside a function   : {len(inside)}")
        print(f"table                       : {out}")
        print()
        print("Mega Hack declares its hook targets as RVAs from the module base and")
        print("reaches them through Geode's hook::createWrapper at runtime, so a")
        print("static decompiler shows an identical thunk at every site instead of")
        print("the callee. This table is the key to that layer: one row per hook.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())