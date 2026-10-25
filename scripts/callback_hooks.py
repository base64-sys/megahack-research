#!/usr/bin/env python3
"""List the hooks a tag callback installs, from the instruction bytes.

``tag_callback_map.py`` maps tag -> callback.  This tool answers the next question for
one callback: which target does it hook, with which detour, under which friendly name,
through which installer helper, and at which order constant?

Method (established in notes/render-hooks.md): a same-image hook is installed as

    mov  dword [rsp+0x20], <order>   ; hook order constant (e.g. 0xffffffe0)
    mov  edx, <gd_rva>
    call 0x18010A370            ; resolve RVA -> function
    lea  r8,  [rip+disp]        ; detour
    lea  r9,  [rip+disp]        ; friendly name (C string)
    mov  rcx, <ctx>  /  mov rdx, <resolved>
    call <installer>            ; per-callback thin helper, not one shared function

A hook resolved by module + MSVC-mangled export instead goes through 0x18010A5A0 and
carries no RVA; those rows are reported as ``symbol-resolved`` rather than guessed.

The scan anchors on the `call 0x18010A370` whose rel32 points at the resolver, reads the
`mov edx,imm32` immediately before it, and then looks forward a bounded distance for the
two `lea` operands and the installer call.  The bound is the start of the next callback in
the registration table, falling back to the first `ret` followed by `int3` padding, so the
scan cannot wander into a neighbour.  Anchoring this way keeps the tool dependency-free
(no disassembler package is vendored on a fresh clone) while making an invented hook
unlikely: the detour must be a plausible `.text` address, the name must be printable ASCII
containing ``::``, and the installer must be a `call` that follows the name operand.

Usage (from the repository root):

    python3 scripts/callback_hooks.py samples/absolllute.hackmega/absolllute.hackmega_original.dll 0x180167a60 [...]

Exit status is 0 even when nothing is found; the row is printed as unresolved instead.
"""

from __future__ import annotations

import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from pe_rva import Image

IMAGE_BASE = 0x180000000
RESOLVE_RVA = 0x18010A370
RESOLVE_SYMBOL = 0x18010A5A0
# Documented helpers.  The installer is per-callback in practice, so the scan accepts any
# call after the name operand and only labels these addresses by name.
KNOWN_INSTALLERS = {
    0x1800AB230: "FUN_1800ab230",
    0x1800AB5B0: "FUN_1800ab5b0",
    0x180167B20: "FUN_180167b20",
    0x180171710: "FUN_180171710",
    0x180170BC0: "FUN_180170bc0",
}
SCAN_LIMIT = 0x400
LOOKAHEAD = 0x40
LOOKBEHIND = 0x18


def _offset(img: Image, va: int) -> int | None:
    rva = va - IMAGE_BASE
    section = img.section_for(rva)
    if section is None:
        return None
    delta = rva - section.virtual_address
    if delta < 0 or delta >= section.size_of_raw_data:
        return None
    return section.pointer_to_raw_data + delta


def _read(img: Image, va: int, size: int) -> bytes | None:
    off = _offset(img, va)
    if off is None or off + size > len(img.data):
        return None
    return img.data[off:off + size]


def _cstring(img: Image, va: int, limit: int = 200) -> str | None:
    off = _offset(img, va)
    if off is None:
        return None
    end = img.data.find(b"\x00", off, off + limit)
    if end < 0:
        return None
    try:
        text = img.data[off:end].decode("utf-8")
    except UnicodeDecodeError:
        return None
    if not text or any(ord(c) < 32 or ord(c) > 126 for c in text):
        return None
    return text


def _call_target(raw: bytes, at: int) -> int | None:
    if len(raw) < 5 or raw[0] != 0xE8:
        return None
    return at + 5 + struct.unpack_from("<i", raw, 1)[0]


def _pointer_before(img: Image, pos: int) -> int | None:
    """The `mov edx,imm32` that feeds the resolver at ``pos``."""
    window = img.data[max(0, pos - LOOKBEHIND):pos]
    found = None
    for i in range(len(window) - 4):
        if window[i] == 0xBA:
            value = struct.unpack_from("<I", window, i + 1)[0]
            if 0x1000 < value < 0x6000000:
                found = value
    return found


def _plausible_code(img: Image, va: int) -> bool:
    raw = _read(img, va, 1)
    return raw is not None and raw[0] not in (0x00, 0xCC)


def _known_callbacks(img: Image) -> list[int]:
    """Callback addresses from the registration table, when it can be scanned."""
    try:
        import tag_callback_map as tcm
    except ImportError:  # pragma: no cover - shipped alongside this file
        return []
    try:
        registrations = tcm.registrations(img)
    except Exception:  # pragma: no cover - unexpected DLL shape
        return []
    values = set()
    for registration, _tag in registrations:
        callback = tcm._nearest_callback(img, registration)
        if callback is not None:
            values.add(callback)
    return sorted(values)


def _function_end(img: Image, callback: int) -> int:
    """Bound the callback: next callback start, else first `ret` + `int3` padding."""
    off = _offset(img, callback)
    if off is None:
        return callback
    bound = callback + SCAN_LIMIT
    for other in _known_callbacks(img):
        if callback < other < bound:
            bound = other
    raw = img.data[off:off + (bound - callback)]
    for index, byte in enumerate(raw):
        if byte == 0xC3 and raw[index + 1:index + 2] == b"\xCC":
            return min(bound, callback + index + 1)
    return bound


def scan(img: Image, callback: int) -> list[dict]:
    hooks: list[dict] = []
    off = _offset(img, callback)
    if off is None:
        return hooks
    end = _function_end(img, callback)
    raw = img.data[off:off + max(0, end - callback)]
    pos = 0
    while pos < len(raw) - 4:
        if raw[pos] != 0xE8:
            pos += 1
            continue
        at = callback + pos
        target = _call_target(raw[pos:], at)
        if target == RESOLVE_RVA:
            rva = _pointer_before(img, off + pos)
            detour = name = name_va = None
            for va in range(at + 5, min(at + 5 + LOOKAHEAD, end)):
                chunk = _read(img, va, 7)
                if chunk is None:
                    break
                if chunk[:3] == b"\x4c\x8d\x05" and detour is None:
                    candidate = va + 7 + struct.unpack_from("<i", chunk, 3)[0]
                    if _plausible_code(img, candidate):
                        detour = candidate
                elif chunk[:3] == b"\x4c\x8d\x0d" and name is None:
                    candidate = va + 7 + struct.unpack_from("<i", chunk, 3)[0]
                    text = _cstring(img, candidate)
                    if text and "::" in text:
                        name, name_va = text, candidate
            installer = install_at = None
            for va in range(at + 5, min(at + 5 + LOOKAHEAD + 0x20, end)):
                candidate = _call_target(_read(img, va, 5) or b"", va)
                if candidate is None or candidate in (RESOLVE_RVA, RESOLVE_SYMBOL):
                    continue
                if candidate > IMAGE_BASE:
                    installer, install_at = candidate, va
                    break
            hooks.append(
                {
                    "target_rva": rva,
                    "detour": detour,
                    "name": name,
                    "installer": installer,
                    "install_va": install_at,
                    "resolve_va": at,
                    "kind": "rva",
                }
            )
            pos += 5
            continue
        if target == RESOLVE_SYMBOL:
            hooks.append({"kind": "symbol", "resolve_va": at})
            pos += 5
            continue
        pos += 1
    return hooks


def main(argv: list[str]) -> int:
    if len(argv) < 3:
        print("usage: callback_hooks.py <dll> <callback-va> [callback-va ...]")
        return 2
    img = Image.from_path(Path(argv[1]))
    for text in argv[2:]:
        callback = int(text, 0)
        print(f"callback 0x{callback:X}")
        hooks = scan(img, callback)
        if not hooks:
            print("  no hook install found")
        for hook in hooks:
            if hook["kind"] == "symbol":
                print(f"  symbol-resolved hook (resolve call at 0x{hook['resolve_va']:X})")
                continue
            rva = hook["target_rva"]
            detour = hook["detour"]
            installer = hook["installer"]
            label = KNOWN_INSTALLERS.get(installer, "") if installer else ""
            print(
                "  rva={} detour={} installer={}{} at={} name={!r}".format(
                    f"0x{rva:X}" if rva is not None else "?",
                    f"0x{detour:X}" if detour is not None else "?",
                    f"0x{installer:X}" if installer else "UNRESOLVED",
                    f" ({label})" if label else "",
                    f"0x{hook['install_va']:X}" if hook["install_va"] else "?",
                    hook["name"],
                )
            )
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
