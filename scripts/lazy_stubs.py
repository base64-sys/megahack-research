"""List Mega Hack's lazily-bound host calls and the host RVA behind each one.

Mega Hack does not import Geometry Dash code. For host functions it does not
import at all it goes through a generated stub: cache check, then a slow path
that takes `GetModuleHandleA(NULL)` and adds a fixed immediate. That immediate
is the host RVA, so the stub's target can be named statically.

Finding the stubs: every slow path calls the cached base getter
(`0x18010DDC0` by default) and then does `lea rcx, [rax + imm32]`. This script
scans `.text` for that pair in the instruction bytes and reads the immediate.

Usage:
    python3 scripts/lazy_stubs.py samples/absolllute.hackmega/absolllute.hackmega_original.dll
    python3 scripts/lazy_stubs.py <dll> --names recovered/functions/_hook_sites.tsv
    python3 scripts/lazy_stubs.py <dll> --getter 0x18010DDC0 --rva 0x2C64E0

`--names` takes the Ghidra hook-site table (`registrar_va`, `gd_method`,
`gd_rva`, ...) and annotates any stub whose host RVA it already names. If the
file is absent the listing is still complete, just unnamed.
"""

from __future__ import annotations

import argparse
import pathlib
import struct
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from pe_rva import Image  # noqa: E402

IMAGE_BASE = 0x180000000
BASE_GETTER = 0x18010DDC0      # cached GetModuleHandleA(NULL)
CALL = b"\xe8"
LEA_RCX_RAX = b"\x48\x8d\x88"  # lea rcx, [rax + imm32]
WINDOW = 40                    # a stub reaches its lea within a few instructions


def text_section(img: Image):
    for sec in img.sections:
        if sec.name.rstrip("\x00") == ".text":
            return sec
    raise SystemExit("no .text section")


def stubs(img: Image, getter: int) -> list[tuple[int, int]]:
    """Return (call_site_va, host_rva) for every lazy-binding slow path."""
    sec = text_section(img)
    blob = img.data[sec.pointer_to_raw_data:
                    sec.pointer_to_raw_data + sec.size_of_raw_data]
    base = sec.virtual_address + IMAGE_BASE
    out: list[tuple[int, int]] = []
    pos = 0
    while True:
        i = blob.find(CALL, pos)
        if i < 0 or i + 5 > len(blob):
            break
        rel = struct.unpack_from("<i", blob, i + 1)[0]
        va = base + i
        if (va + 5 + rel) & 0xFFFFFFFFFFFFFFFF == getter:
            window = blob[i + 5:i + 5 + WINDOW]
            j = window.find(LEA_RCX_RAX)
            if j >= 0:
                imm = struct.unpack_from("<i", window, j + 3)[0]
                out.append((va, imm & 0xFFFFFFFF))
        pos = i + 1
    return out


def load_names(path: pathlib.Path) -> dict[int, str]:
    names: dict[int, str] = {}
    if not path.exists():
        return names
    for line in path.read_text(encoding="utf-8").splitlines():
        if not line or line.startswith("#"):
            continue
        parts = line.split("\t")
        if len(parts) < 3:
            continue
        try:
            rva = int(parts[2], 16)
        except ValueError:
            continue
        names.setdefault(rva, parts[1])
    return names


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("dll", type=pathlib.Path)
    ap.add_argument("--getter", type=lambda s: int(s, 0), default=BASE_GETTER,
                    help="VA of the cached GetModuleHandleA(NULL) helper")
    ap.add_argument("--names", type=pathlib.Path,
                    default=pathlib.Path("recovered/functions/_hook_sites.tsv"),
                    help="hook-site TSV used to annotate host RVAs")
    ap.add_argument("--rva", type=lambda s: int(s, 0), action="append",
                    help="print only stubs for these host RVAs (repeatable)")
    args = ap.parse_args(argv)

    img = Image.from_path(str(args.dll))
    found = stubs(img, args.getter)
    if not found:
        print("no lazy stubs found", file=sys.stderr)
        return 1

    names = load_names(args.names)
    wanted = set(args.rva) if args.rva else None
    for va, rva in found:
        if wanted is not None and rva not in wanted:
            continue
        label = names.get(rva, "")
        print(f"{va:#012x} -> host rva {rva:#08x}" + (f"  {label}" if label else ""))

    distinct = {r for _, r in found}
    named = sum(1 for r in distinct if r in names)
    print(f"stubs: {len(found)}  distinct host rvas: {len(distinct)}  named: {named}",
          file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
