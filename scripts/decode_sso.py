"""Decode MSVC small-string-optimised literals out of the recovered tree.

When Ghidra sees `std::string s = "ICONIC/GENERAL";` on the stack it renders the
string as a chain of typed locals, because that is literally how MSVC materialises
a short std::string: an inline 16-byte SSO buffer packed with the bytes, then a
size and a capacity of 15.

The result is a function that looks unreadable but is fully recoverable - the text
sits in the immediates of the stores that fill the buffer:

    local_58 = 0x4e4f4349;   // "ICON"
    uStack_54 = 0x4349;      // "IC"
    uStack_52 = 0x2f;        // "/"
    uStack_51 = 0x41534944;  // "DISA"
    uStack_4d = 0x454c42;    // "BLE"

This script extracts those immediates in declaration order and prints the string,
so keys like the four `FUN_18007cb60` lookups that `settings_index.py` could only
report as `local_58` become readable again.

    python scripts/decode_sso.py
    python scripts/decode_sso.py --grep ICONIC
    python scripts/decode_sso.py --file recovered/functions/18011ffe0_FUN_18011ffe0.c
"""

from __future__ import annotations

import argparse
import collections
import pathlib
import re
import struct
import sys

REPO = pathlib.Path(__file__).resolve().parent.parent
FUNCTIONS = REPO / "recovered" / "functions"

# local_XX = 0x...;   /  uStack_XX = 0x...;   /  local_XX._0_1_ = 0x...
STORE_RE = re.compile(
    r"^\s*(\w+)\s*(?:\._\d+_\d+)?\s*=\s*(0x[0-9a-fA-F]+|\d+)\s*;"
)
# a literal Ghidra already decoded
LITERAL_RE = re.compile(r'"((?:[^"\\\n]|\\.){2,})"')
STRNCPY_RE = re.compile(r'builtin_strncpy\([^,]+,\s*"([^"]+)"')


def immediate_bytes(value: int, width: int = 4) -> bytes:
    """Little-endian bytes for an integer that Ghidra typed from a char array.

    Widen rather than overflow: Ghidra occasionally merges adjacent stores into a
    single 64-bit immediate, and truncating it would silently corrupt the string.
    """
    width = max(1, min(width, 8))
    while width < 8 and value >= (1 << (width * 8)):
        width += 1
    return value.to_bytes(width, "little", signed=False)


def decode_from_file(path: pathlib.Path) -> list[tuple[str, str]]:
    """Return [(local, decoded_text)] for each SSO literal in one file.

    All immediate stores in the file are concatenated into one stream in
    declaration order, which is the order MSVC emitted the string bytes, and the
    printable runs are pulled out of it. Grouping per contiguous block instead
    loses strings: Ghidra interleaves unrelated statements between the stores of
    a single literal, so a per-group scan splits ICONIC/DISABLE into two pieces.
    The zero bytes of the SSO tail and the size/capacity fields separate one
    literal from the next, so they act as natural delimiters.
    """
    text = path.read_text(encoding="utf-8", errors="replace")
    results: list[tuple[str, str]] = []

    for literal in STRNCPY_RE.findall(text):
        results.append(("strncpy", literal))

    stream: list[tuple[str, bytes]] = []
    for line in text.splitlines():
        m = STORE_RE.match(line)
        if not m:
            continue
        name, raw = m.group(1), m.group(2)
        value = int(raw, 0)
        if not (0 < value < (1 << 64)):
            continue
        stream.append((name, immediate_bytes(value)))

    blob = bytearray()
    for _, raw in stream:
        # Each store is a 4- or 8-byte immediate holding a few characters
        # followed by zero padding. Dropping only that per-store padding keeps
        # a literal whole; the zero bytes that separate one literal from the
        # next come from the buffer initialisation and the size/capacity fields,
        # which are separate stores and survive here.
        blob.extend(raw.rstrip(b"\x00"))

    current = bytearray()
    start_name = stream[0][0] if stream else ""
    for byte in blob:
        if 32 <= byte < 127:
            if not current:
                start_name = "sso"
            current.append(byte)
            continue
        if len(current) >= 3:
            results.append((start_name, current.decode("ascii", errors="replace")))
        current = bytearray()

    return results


def decode(parts: list[tuple[str, bytes]]) -> str:
    """Longest printable ASCII run in an SSO group.

    A group is not just the string bytes: MSVC also emits the initialisation
    sentinel (0xfffffffffffffffe), the zero-filled tail of the buffer, and the
    size/capacity fields, all as stores. Taking the whole group and splitting at
    the first NUL truncates at the sentinel, so scan for the longest printable
    run instead.
    """
    blob = bytearray()
    for _, raw in parts:
        blob.extend(raw)
    best = ""
    current = bytearray()
    for byte in blob:
        if 32 <= byte < 127:
            current.append(byte)
            continue
        if len(current) > len(best):
            best = current.decode("ascii", errors="replace")
        current = bytearray()
    if len(current) > len(best):
        best = current.decode("ascii", errors="replace")
    return best


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--functions", type=pathlib.Path, default=FUNCTIONS)
    parser.add_argument("--file", type=pathlib.Path, help="decode one specific recovered .c file")
    parser.add_argument("--grep", help="only show decoded strings containing this")
    parser.add_argument("--limit", type=int, default=60)
    args = parser.parse_args(argv)

    if args.file:
        targets = [args.file]
    else:
        if not args.functions.is_dir():
            raise SystemExit(f"no recovered tree at {args.functions}; run scripts/decompile_all.py first")
        targets = sorted(args.functions.glob("*.c"))

    needle = args.grep.lower() if args.grep else None
    shown = 0
    seen: collections.Counter = collections.Counter()
    for path in targets:
        for local, text in decode_from_file(path):
            if not text or len(text) < 3:
                continue
            printable = sum(1 for c in text if 32 <= ord(c) < 127)
            if printable / len(text) < 0.9:
                continue
            seen[text] += 1
            if needle and needle not in text.lower():
                continue
            if shown >= args.limit:
                return 0
            shown += 1
            print(f"{path.stem:<44} {local:<16} {text!r}")

    print()
    print(f"decoded {sum(seen.values())} literals, {len(seen)} distinct", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())