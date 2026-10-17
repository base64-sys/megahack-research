"""Search Mega Hack for an FNV-1a-64 hash table, keyed by strings we can read.

`FUN_18007CB60` (VA `0x18007CB60`, 40 callers) hashes a `std::string` and returns
a `uint64_t`. It uses the FNV-1a-64 basis `0xCBF29CE484222325` and prime
`0x100000001B3`, so every setting key, asset path and UI string that passes
through it has a predictable 8-byte fingerprint.

That gives a way to answer a question the decompiler cannot: *is this string
actually registered, and where?* Hash every string we have recovered and look
for that 8-byte little-endian value in the image. A hit is a hit in the strong
sense - the exact 64-bit value was written by the binary - and it hands back the
address of whatever table contains it.

    python scripts/fnv_hash_probe.py --list KEYBINDS
    python scripts/fnv_hash_probe.py --text CONFIG
    python scripts/fnv_hash_probe.py --dump 0x18080c378

Two variants of the inner loop are supported, because the decompiled body is a
4x-unrolled loop whose parenthesisation is hard to read by eye:

- ``std``  - textbook FNV-1a: ``h = (h ^ byte) * prime`` per byte.
- ``mh``   - exactly what the compiled loop appears to do, applying the prime
  once per byte plus a fifth time per 4-byte block.

Neither is assumed correct. ``--variant both`` reports which one hits, and a
variant that hits nothing is reported as such rather than quietly dropped.
"""

from __future__ import annotations

import argparse
import collections
import pathlib
import struct
import sys

REPO = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from build_xref_index import LITERAL_RE  # noqa: E402
from decode_sso import decode_from_file  # noqa: E402
from pe_rva import Image  # noqa: E402

FUNCTIONS = REPO / "recovered" / "functions"
DEFAULT_DLL = REPO / "samples" / "absolllute.hackmega" / "absolllute.hackmega_original.dll"


def strings_in_tree(text: str) -> list[str]:
    """Literals Ghidra already decoded inline, which need no SSO reconstruction."""
    return [m for m in LITERAL_RE.findall(text) if "\\" not in m]

BASIS = 0xCBF29CE484222325
PRIME = 0x100000001B3
MASK = 0xFFFFFFFFFFFFFFFF


def fnv1a64_std(data: bytes) -> int:
    h = BASIS
    for byte in data:
        h = ((h ^ byte) * PRIME) & MASK
    return h


def fnv1a64_mh(data: bytes) -> int:
    """FNV-1a-64 with the multiply applied a fifth time per 4-byte block.

    Taken from the shape of the unrolled loop at `0x18007CB60`. Recorded as a
    candidate, not as a fact: the decompiled parenthesisation is ambiguous and
    this variant exists to be tested against the image, not trusted.
    """
    h = BASIS
    i = 0
    n = len(data)
    while i + 4 <= n:
        for k in range(4):
            h = ((h ^ data[i + k]) * PRIME) & MASK
        h = (h * PRIME) & MASK
        i += 4
    while i < n:
        h = ((h ^ data[i]) * PRIME) & MASK
        i += 1
    return h


VARIANTS = {"std": fnv1a64_std, "mh": fnv1a64_mh}


def candidates(given: list[str] | None) -> list[tuple[str, str]]:
    """Return [(source, text)] of every string worth hashing.

    With no ``--list`` this is the whole recovered tree: inline literals, the
    literal Ghidra already decoded, and SSO literals rebuilt from immediates.
    """
    if given:
        return [("argument", t) for t in given]

    out: list[tuple[str, str]] = []
    for path in sorted(FUNCTIONS.glob("*.c")):
        text = path.read_text(encoding="utf-8", errors="replace")
        for literal in strings_in_tree(text):
            out.append((path.stem, literal))
        for _, decoded in decode_from_file(path):
            out.append((path.stem, decoded))
    return out


def scan(image: Image, values: dict[int, set[str]]):
    """Find every 8-byte LE occurrence of any value in `values`, in one pass.

    Sliding the window across the whole image once and testing membership beats
    searching the image per value: the naive form is O(values x filesize), which
    at a hundred thousand candidates against 8.7 MB does not finish. Offsets are
    turned into addresses with `va_of_file_offset`, never by subtracting the
    image base from a file position.

    Only file-backed bytes are searched. A value living in a section's zero-fill
    tail exists at run time and cannot be found here; that is a limitation of the
    search, not evidence of absence, and the report says so.
    """
    wanted = set(values)
    hits: dict[int, list[str]] = collections.defaultdict(list)
    blob = image.data
    unpack = struct.Struct("<Q").unpack_from
    last = len(blob) - 8
    for off in range(last + 1):
        names = values.get(unpack(blob, off)[0])
        if names is not None:
            hits[off].extend(names)
    return hits


def control(image: Image) -> bool:
    """Positive control: prove the scanner can actually find a known 8-byte value.

    Without this, "zero hits" is indistinguishable from a broken scan. The FNV
    basis `0xCBF29CE484222325` is loaded by `FUN_18007CB60` as a `movabs` immediate,
    so its little-endian bytes are certainly present in `.text` and must be found.
    """
    hits = scan(image, {BASIS: {"FNV-1a-64 basis (control)"}})
    if not hits:
        return False
    off = min(hits)
    va = image.va_of_file_offset(off)
    print(f"control OK: FNV basis found at {va:#x} (file offset {off:#x})")
    return True


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("--dll", type=pathlib.Path, default=DEFAULT_DLL)
    parser.add_argument("--list", help="file of candidate strings, one per line")
    parser.add_argument("--text", action="append", default=[], help="hash this exact string")
    parser.add_argument("--variant", choices=["std", "mh", "both"], default="both")
    parser.add_argument("--dump", help="print the stored bytes at this VA")
    args = parser.parse_args(argv)

    if args.dump:
        image = Image.from_path(args.dll)
        va = int(args.dump, 0)
        res = image.resolve(va)
        print(f"resolves as: {res.kind} - {res.note}")
        if res.ok:
            data = image.read(va, 64)
            for i in range(0, len(data), 16):
                row = data[i : i + 16]
                text = "".join(chr(c) if 32 <= c < 127 else "." for c in row)
                print(f"{va + i:#x}  {row.hex(' '):<47}  {text}")
        else:
            print("no file backing at this address; nothing to dump")
        return 0

    given: list[str] = list(args.text)
    if args.list:
        given += [
            line.strip()
            for line in args.list.read_text(encoding="utf-8").splitlines()
            if line.strip() and not line.startswith("#")
        ]

    pool = candidates(given or None)
    if not pool:
        print("no candidate strings found; is recovered/ populated?", file=sys.stderr)
        return 1

    image = Image.from_path(args.dll)
    names = ["std", "mh"] if args.variant == "both" else [args.variant]

    print(f"dll        : {args.dll.name}")
    print(f"candidates : {len(pool)} ({len({t for _, t in pool})} distinct)")
    print("note       : searches file-backed bytes only; a value in a section's")
    print("             zero-fill tail exists at run time and is not findable here")
    print()
    if not control(image):
        print("CONTROL FAILED: the scanner cannot find a value known to be present,")
        print("so any zero result below would be meaningless. Stopping.")
        return 2
    print()

    for variant in names:
        fn = VARIANTS[variant]
        values: dict[int, set[str]] = collections.defaultdict(set)
        for _, text in pool:
            values[fn(text.encode("utf-8"))].add(text)
        hits = scan(image, values)
        print(f"variant {variant}: {len(values)} hashes, {len(hits)} byte-level hits")
        if not hits:
            print("  no hits - this variant does not appear to match the binary")
            print()
            continue
        for off in sorted(hits)[:40]:
            va = image.va_of_file_offset(off)
            where = f"{va:#x}" if va else "unmapped"
            print(f"  {where}  {', '.join(sorted(set(hits[off]))[:4])}")
        if len(hits) > 40:
            print(f"  ... {len(hits) - 40} more")
        print()

    print("An 8-byte hit is strong evidence the string is hashed by the mod: the")
    print("value is not a guess, it is the exact output of the binary's own hash.")
    print("It is still not a lookup table - confirm the surrounding structure.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())