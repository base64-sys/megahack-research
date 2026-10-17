#!/usr/bin/env python3
"""Recover the CRT static-initializer tables and every function they call.

A direct-call call graph cannot see a function that is invoked through a
pointer. MSVC runs global constructors from tables in `.rdata` via
`_initterm` / `_initterm_e`, so any such function legitimately has zero
callers while executing on every DLL load. That reading is indistinguishable
from dead code unless the table is recovered, so this tool does that and
writes every entry out.

For each import named `_initterm` or `_initterm_e` it finds the IAT jump
thunk, finds the call sites of that thunk, recovers the `(first, last)`
argument pair from the `lea` pair that feeds the call, and emits one row per
table entry.

Only fixed instruction prefixes are decoded by hand, so this runs on a fresh
clone without capstone or pefile. Everything else comes from pe_rva.py.
"""

from __future__ import annotations

import argparse
import csv
import dataclasses
import pathlib
import struct
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from pe_rva import Image  # noqa: E402

DEFAULT_IMAGE = "samples/absolllute.hackmega/absolllute.hackmega_original.dll"
DEFAULT_OUT = "recovered/functions/_init_table.tsv"

EXECUTABLE = 0x20000000
READABLE = 0x40000000

TARGET_IMPORTS = ("_initterm", "_initterm_e")

DIR_IMPORT = 1
OPT_MAGIC_PE32PLUS = 0x20B


@dataclasses.dataclass(frozen=True)
class Range:
    """An `_initterm(first, last)` call and the table it walks."""

    call_site: int
    thunk: int
    symbol: str
    first: int
    last: int

    @property
    def slots(self) -> int:
        return max(0, (self.last - self.first) // 8)


def data_directory(img: Image, index: int) -> tuple[int, int]:
    """Return `(rva, size)` of a PE data directory, reading headers directly."""
    data = img.data
    pe_off = struct.unpack_from("<I", data, 0x3C)[0]
    if data[pe_off : pe_off + 4] != b"PE\x00\x00":
        raise ValueError("not a PE image: bad signature")
    opt_off = pe_off + 4 + 20
    magic = struct.unpack_from("<H", data, opt_off)[0]
    if magic != OPT_MAGIC_PE32PLUS:
        raise ValueError(f"expected PE32+ (0x20B), got {magic:#x}")
    count = struct.unpack_from("<I", data, opt_off + 108)[0]
    if index >= count:
        raise ValueError(f"data directory {index} not present ({count} entries)")
    return struct.unpack_from("<II", data, opt_off + 112 + index * 8)


def import_slots(img: Image) -> dict[int, str]:
    """Map every IAT slot VA to `dll!symbol`."""
    import_rva, _ = data_directory(img, DIR_IMPORT)
    off = img.resolve(img.to_va(import_rva))
    if not off.ok or off.file_offset is None:
        raise ValueError("import directory is not file-backed")
    data = img.data
    slots: dict[int, str] = {}
    cursor = off.file_offset
    while True:
        oft, _, _, name_rva, first_thunk = struct.unpack_from("<IIIII", data, cursor)
        if not any((oft, name_rva, first_thunk)):
            break
        dll = _cstr(img, img.to_va(name_rva))
        thunk_rva = first_thunk
        while True:
            entry = struct.unpack_from("<Q", data, img.resolve(img.to_va(thunk_rva)).file_offset)[0]
            if not entry:
                break
            if entry & (1 << 63):
                slots[img.to_va(thunk_rva)] = f"{dll}#ord{entry & 0xFFFF}"
            else:
                res = img.resolve(img.to_va(entry & 0x7FFFFFFF))
                slots[img.to_va(thunk_rva)] = f"{dll}!{_cstr(img, img.to_va(entry & 0x7FFFFFFF), 2)}"
            thunk_rva += 8
        cursor += 20
    return slots


def _cstr(img: Image, va: int, skip: int = 0) -> str:
    """NUL-terminated ASCII at `va`, starting `skip` bytes in."""
    res = img.resolve(va)
    if not res.ok or res.file_offset is None:
        return ""
    start = res.file_offset + skip
    end = img.data.find(b"\x00", start)
    if end < 0:
        return ""
    return img.data[start:end].decode("ascii", "replace")


def executable_ranges(img: Image) -> list[tuple[int, int]]:
    """File-backed VA spans of executable sections."""
    spans = []
    for sec in img.sections:
        if not sec.characteristics & EXECUTABLE:
            continue
        size = min(sec.size_of_raw_data, sec.virtual_size)
        if size <= 0:
            continue
        spans.append((img.to_va(sec.virtual_address), img.to_va(sec.virtual_address) + size))
    return spans


def find_jmp_thunks(img: Image, slot_va: int) -> list[int]:
    """VA of every `jmp qword ptr [rip + disp32]` whose target is `slot_va`."""
    out: list[int] = []
    data = img.data
    for lo, hi in executable_ranges(img):
        for va in range(lo, hi - 6):
            off = img.resolve(va).file_offset
            if data[off] != 0xFF or data[off + 1] != 0x25:
                continue
            disp = struct.unpack_from("<i", data, off + 2)[0]
            if va + 6 + disp == slot_va:
                out.append(va)
    return out


def find_calls_to(img: Image, target: int) -> list[int]:
    """VA of every `call rel32` whose destination is `target`."""
    out: list[int] = []
    data = img.data
    for lo, hi in executable_ranges(img):
        for va in range(lo, hi - 5):
            off = img.resolve(va).file_offset
            if data[off] != 0xE8:
                continue
            disp = struct.unpack_from("<i", data, off + 1)[0]
            if va + 5 + disp == target:
                out.append(va)
    return out


def recover_arg_pair(img: Image, call_site: int, window: int = 0x60) -> tuple[int, int] | None:
    """Recover `(rcx, rdx)` from the nearest preceding RIP-relative `lea` pair."""
    data = img.data
    first = last = None
    for va in range(call_site - window, call_site - 6):
        off = img.resolve(va).file_offset
        if data[off] != 0x48 or data[off + 1] != 0x8D:
            continue
        modrm = data[off + 2]
        if modrm & 0xC7 != 0x05:
            continue
        tgt = va + 7 + struct.unpack_from("<i", data, off + 3)[0]
        reg = (modrm >> 3) & 7
        if reg == 1:
            first = tgt
        elif reg == 2:
            last = tgt
    if first is None or last is None or last <= first:
        return None
    return first, last


def read_pointer(img: Image, va: int) -> int:
    res = img.resolve(va)
    if not res.ok or res.file_offset is None or res.file_offset + 8 > len(img.data):
        return 0
    return struct.unpack_from("<Q", img.data, res.file_offset)[0]


def collect(img: Image, wanted: tuple[str, ...]) -> tuple[list[Range], dict[int, str]]:
    slots = import_slots(img)
    by_symbol = {}
    for slot, name in slots.items():
        for sym in wanted:
            if name.endswith("!" + sym):
                by_symbol.setdefault(sym, slot)
    ranges: list[Range] = []
    for sym in wanted:
        slot = by_symbol.get(sym)
        if slot is None:
            continue
        for thunk in find_jmp_thunks(img, slot):
            for site in find_calls_to(img, thunk):
                pair = recover_arg_pair(img, site)
                if pair is None:
                    continue
                ranges.append(Range(site, thunk, sym, pair[0], pair[1]))
    return ranges, slots


def load_callers(path: pathlib.Path) -> dict[int, int]:
    """Function VA -> direct caller count, from `_callers.tsv` if present.

    Rows are keyed by `<hexva>_<name>`, not a bare VA, so the key has to be
    split. A missing key means "no row", which is not the same as zero.
    """
    counts: dict[int, int] = {}
    if not path.exists():
        return counts
    for line in path.read_text(encoding="utf-8").splitlines():
        if not line or line.startswith("#"):
            continue
        parts = line.split("\t")
        if len(parts) < 2:
            continue
        head = parts[0].split("_", 1)[0]
        try:
            counts[int(head, 16)] = int(parts[1])
        except ValueError:
            continue
    return counts


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("image", nargs="?", default=DEFAULT_IMAGE)
    ap.add_argument("-o", "--out", default=DEFAULT_OUT)
    ap.add_argument("--callers", default="recovered/functions/_callers.tsv")
    ap.add_argument("--show", action="store_true", help="print every entry")
    args = ap.parse_args(argv)

    image_path = pathlib.Path(args.image)
    if not image_path.exists():
        print(f"image not found: {image_path}", file=sys.stderr)
        return 2
    img = Image.from_path(image_path)

    ranges, _ = collect(img, TARGET_IMPORTS)
    if not ranges:
        print("no _initterm / _initterm_e ranges recovered", file=sys.stderr)
        return 1

    callers = load_callers(pathlib.Path(args.callers))
    rows = []
    for rng in ranges:
        for i in range(rng.slots):
            slot_va = rng.first + i * 8
            target = read_pointer(img, slot_va)
            rows.append(
                {
                    "symbol": rng.symbol,
                    "call_site": f"{rng.call_site:#x}",
                    "thunk": f"{rng.thunk:#x}",
                    "first": f"{rng.first:#x}",
                    "last": f"{rng.last:#x}",
                    "index": i,
                    "slot_va": f"{slot_va:#x}",
                    "target_va": f"{target:#x}" if target else "",
                    "direct_callers": callers.get(target, 0) if target else "",
                }
            )

    out = pathlib.Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    with out.open("w", encoding="utf-8", newline="") as fh:
        w = csv.DictWriter(fh, fieldnames=list(rows[0].keys()), delimiter="\t", lineterminator="\n")
        w.writeheader()
        w.writerows(rows)
    print(f"wrote {len(rows)} rows to {out}")

    for rng in ranges:
        live = [read_pointer(img, rng.first + i * 8) for i in range(rng.slots)]
        nz = [v for v in live if v]
        orphan = [v for v in nz if v not in callers]
        print(
            f"{rng.symbol:12s} call {rng.call_site:#x} -> [{rng.first:#x}, {rng.last:#x}) "
            f"{rng.slots} slots, {len(nz)} non-zero, {len(orphan)} with zero direct callers"
        )
        if args.show:
            for i, v in enumerate(nz):
                print(f"    [{i:3d}] {v:#x}  callers={callers.get(v, 'n/a')}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())