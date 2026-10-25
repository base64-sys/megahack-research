#!/usr/bin/env python3
"""Resolve an import thunk in the sample DLL to ``dll!symbol``.

Detours in this image reach cocos2d and the CRT almost exclusively through
one-instruction thunks at the end of ``.text``::

    0x180532410  jmp qword ptr [rip + 0x111312]   ; -> IAT slot 0x180643728

Reading a detour therefore gives call targets such as ``0x180532410`` that are
meaningless on their own.  This tool turns them into the imported name by
parsing the PE import directory and matching each ``FF 25`` stub to its IAT
slot.  Nothing is hardcoded about the sample, and no third-party package is
used: the headers and thunks are read through ``scripts/pe_rva.py``.

Method
------
1. Read ``IMAGE_DIRECTORY_ENTRY_IMPORT`` from the optional header.
2. Walk the import descriptors; for each, record ``FirstThunk + i * 8`` (the
   IAT slot) together with the DLL name and the name/ordinal from the ILT.
3. Walk ``.text`` for ``FF 25 <disp32>`` stubs, compute the slot each stub
   jumps through, and join the two tables.

Usage (from the repository root)::

    python3 scripts/import_names.py samples/absolllute.hackmega/absolllute.hackmega_original.dll 0x180532410
    python3 scripts/import_names.py samples/absolllute.hackmega/absolllute.hackmega_original.dll --summary

A stub VA, an IAT slot VA or a raw IAT RVA is accepted.  ``--summary`` prints
the per-DLL import counts, which is also the cheapest self-check of the parser:
the totals are expected to match the figures in ``notes/host-abi.md``.
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from pe_rva import Image

IMPORT_DIRECTORY_INDEX = 1
DESCRIPTOR_SIZE = 20


def _import_directory(img: Image) -> tuple[int, int]:
    (lfanew,) = struct.unpack_from("<I", img.data, 0x3C)
    optional = lfanew + 4 + 20
    directory = optional + (0x70 if img.pe32_plus else 0x60)
    entry = directory + IMPORT_DIRECTORY_INDEX * 8
    rva, size = struct.unpack_from("<II", img.data, entry)
    return rva, size


def _cstring(img: Image, rva: int) -> str:
    offset = img.to_offset(rva) if hasattr(img, "to_offset") else None
    if offset is None:
        # pe_rva exposes section_for(); fall back to it.
        section = img.section_for(rva)
        if section is None:
            return f"<unreadable RVA {rva:#x}>"
        offset = section.pointer_to_raw_data + (rva - section.virtual_address)
    end = img.data.find(b"\x00", offset)
    return img.data[offset:end].decode("ascii", errors="replace")


def imports(img: Image) -> dict[int, tuple[str, str]]:
    """Map IAT slot RVA -> (dll, symbol)."""
    directory_rva, _ = _import_directory(img)
    if directory_rva == 0:
        return {}
    table: dict[int, tuple[str, str]] = {}
    section = img.section_for(directory_rva)
    if section is None:
        return {}
    base = section.pointer_to_raw_data + (directory_rva - section.virtual_address)
    index = 0
    while True:
        descriptor = base + index * DESCRIPTOR_SIZE
        if descriptor + DESCRIPTOR_SIZE > len(img.data):
            break
        ilt, _, _, name_rva, iat = struct.unpack_from("<IIIII", img.data, descriptor)
        if ilt == 0 and iat == 0 and name_rva == 0:
            break
        dll = _cstring(img, name_rva)
        lookup = ilt or iat
        slot = 0
        while True:
            (thunk,) = struct.unpack_from("<Q", img.data, img.section_for(lookup).pointer_to_raw_data
                                          + (lookup - img.section_for(lookup).virtual_address) + slot * 8)
            if thunk == 0:
                break
            if thunk & (1 << 63):
                symbol = f"#{thunk & 0xFFFF}"
            else:
                hint_name = img.section_for(thunk).pointer_to_raw_data + (
                    thunk - img.section_for(thunk).virtual_address)
                end = img.data.find(b"\x00", hint_name + 2)
                symbol = img.data[hint_name + 2:end].decode("ascii", errors="replace")
            table[iat + slot * 8] = (dll, symbol)
            slot += 1
        index += 1
    return table


def stubs(img: Image) -> dict[int, int]:
    """Map thunk VA -> IAT slot RVA for every ``FF 25`` stub in the image."""
    out: dict[int, int] = {}
    for section in img.sections:
        if section.name != ".text":
            continue
        body = img.data[section.pointer_to_raw_data:
                        section.pointer_to_raw_data + section.size_of_raw_data]
        position = body.find(b"\xff\x25")
        while position != -1:
            (disp,) = struct.unpack_from("<i", body, position + 2)
            va = img.image_base + section.virtual_address + position
            slot = va + 6 + disp - img.image_base
            out[va] = slot
            position = body.find(b"\xff\x25", position + 1)
    return out


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("image", help="path to the sample DLL")
    parser.add_argument("addresses", nargs="*", help="thunk VA, IAT slot VA or IAT slot RVA")
    parser.add_argument("--summary", action="store_true", help="print per-DLL import counts")
    args = parser.parse_args(argv)

    img = Image.from_path(args.image)
    table = imports(img)
    thunks = stubs(img)

    if args.summary:
        per_dll: dict[str, int] = {}
        for dll, _ in table.values():
            per_dll[dll] = per_dll.get(dll, 0) + 1
        print(f"imports: {len(table)} across {len(per_dll)} DLLs; FF 25 stubs found: {len(thunks)}")
        for dll in sorted(per_dll):
            print(f"  {per_dll[dll]:4d}  {dll}")
        return 0

    if not args.addresses:
        parser.error("give at least one address, or --summary")
    status = 0
    for text in args.addresses:
        value = int(text, 16)
        relative = value - img.image_base
        slot = thunks.get(value)
        via = "stub"
        if slot is None:
            slot = relative if relative in table else value
            via = "iat"
        found = table.get(slot)
        if found is None:
            print(f"{value:#x}: no import thunk or IAT slot at this address")
            status = 1
            continue
        dll, symbol = found
        print(f"{value:#x} -> {dll}!{symbol}   [via {via}, IAT slot {img.image_base + slot:#x}]")
    return status


if __name__ == "__main__":
    raise SystemExit(main())
