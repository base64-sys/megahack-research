#!/usr/bin/env python3
"""Map every hackpro tag registration to its callback, from instruction bytes.

Mega Hack's giant initialiser ``FUN_1800afae0`` registers each toggle as

    lea  rcx, [rip + X]      ; the per-tag callback
    mov  [rbp + off], rcx
    ...
    mov  edx, <tag>
    mov  rcx, rax
    mov  r8,  rsi
    call 0x18008f910         ; FNV-1a-64 hash of the tag; bind the callback

The callback is therefore the **nearest preceding** ``lea rcx,[rip+disp]`` whose target
lies in the Mega Hack code region. Reading the block that *follows* a registration takes
the next tag's callback instead -- which is how a note once attributed No Shake to No
Shaders' callback. This script makes the mapping checkable in one command.

Usage (repository root, sample set present)::

    python3 scripts/tag_callback_map.py samples/absolllute.hackmega/absolllute.hackmega_original.dll

No third-party packages: section bytes are read through the tracked ``scripts/pe_rva.py``.
"""

from __future__ import annotations

import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from pe_rva import Image  # noqa: E402  (tracked, dependency-free)

IMAGE_BASE = 0x180000000
REGISTRAR = 0x18008F910  # FNV-1a-64 of the tag, called from the initialiser
INIT_LO = 0x1800AFAE0  # the giant initialiser's range
INIT_HI = 0x1800B2000
CODE_LO = 0x180001000  # plausible callback range (Mega Hack .text)
CODE_HI = 0x180200000

# mov edx,imm32 / mov rcx,rax / mov r8,rsi / call rel32
MARKER_TAIL_OFF = 5
MARKER_TAIL = b"\x48\x89\xc1\x49\x89\xf0"


def _section_bytes(img: Image, name: str) -> tuple[int, bytes] | None:
    for section in img.sections:
        if section.name.rstrip("\x00") != name:
            continue
        start = section.pointer_to_raw_data
        return section.virtual_address + IMAGE_BASE, img.data[start : start + section.size_of_raw_data]
    return None


def _va_to_offset(img: Image, va: int) -> int | None:
    rva = va - IMAGE_BASE
    section = img.section_for(rva)
    if section is None:
        return None
    delta = rva - section.virtual_address
    if delta < 0 or delta >= section.size_of_raw_data:
        return None
    return section.pointer_to_raw_data + delta


def _nearest_callback(img: Image, reg_va: int, limit: int = 0x180) -> int | None:
    """Walk back from the registration to the closest ``lea rcx,[rip+disp]``."""
    for back in range(5, limit):
        off = _va_to_offset(img, reg_va - back)
        if off is None or img.data[off : off + 3] != b"\x48\x8d\x0d":
            continue
        disp = struct.unpack_from("<i", img.data, off + 3)[0]
        target = (reg_va - back) + 7 + disp
        if CODE_LO <= target <= CODE_HI:
            return target
    return None


def registrations(img: Image) -> list[tuple[int, int]]:
    """Every ``mov edx,tag / mov rcx,rax / mov r8,rsi / call registrar`` in ``.text``.

    Until 2026-10-25 this was bounded to the cosmetic block at ``INIT_LO``..``INIT_HI``,
    which made the map look complete at 79 registrations.  The initialiser in fact
    continues past ``INIT_HI`` and other sites exist elsewhere, so the scan covers the
    whole section; the two constants are kept only to name that block in the notes.
    """
    found = _section_bytes(img, ".text")
    if found is None:
        raise SystemExit("no .text section")
    text_va, text = found
    hits: list[tuple[int, int]] = []
    off = 0
    while True:
        off = text.find(b"\xba", off)
        if off < 0:
            break
        va = text_va + off
        if (
            text[off + MARKER_TAIL_OFF : off + MARKER_TAIL_OFF + len(MARKER_TAIL)] == MARKER_TAIL
            and text[off + 11] == 0xE8
            and struct.unpack_from("<i", text, off + 12)[0] + (va + 16) == REGISTRAR
        ):
            hits.append((va, struct.unpack_from("<I", text, off + 1)[0]))
        off += 1
    return hits


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print(__doc__.strip().splitlines()[-1], file=sys.stderr)
        return 2
    img = Image.from_path(argv[1])
    hits = registrations(img)
    print(f"{'tag':>6}  {'registration':<12}  callback")
    for va, tag in sorted(hits, key=lambda h: h[1]):
        cb = _nearest_callback(img, va)
        rendered = hex(cb) if cb is not None else "<none>"
        print(f"0x{tag:04X}  {hex(va):<12}  {rendered}")
    print(f"\n{len(hits)} registrations", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
