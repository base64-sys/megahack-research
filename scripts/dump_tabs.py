#!/usr/bin/env python3
"""Extract and decompress Mega Hack's embedded tab definitions.

Mega Hack embeds its two built-in tabs ("HOME" and "KEYBINDS") as gzip-compressed
JSON blobs inside .data:
  - Tab 1 ("HOME"): VA 0x1806FE004 (RVA 0x6FE004, file 0x6FD204, size 0x2D5)
  - Tab 2 ("KEYBINDS"): VA 0x1806FE2D9 (RVA 0x6FE2D9, file 0x6FD4D9, size 0xC2)

Usage:
    python scripts/dump_tabs.py
"""

from __future__ import annotations

import gzip
import json
import pathlib
import sys

REPO = pathlib.Path(__file__).resolve().parent.parent
DLL = REPO / "samples" / "absolllute.hackmega" / "absolllute.hackmega_original.dll"

TABS = [
    {
        "id": "HOME",
        "va": "0x1806FE004",
        "file_offset": 0x6FD204,
        "compressed_size": 0x2D5,
    },
    {
        "id": "KEYBINDS",
        "va": "0x1806FE2D9",
        "file_offset": 0x6FD4D9,
        "compressed_size": 0xC2,
    },
]


def extract_tabs(dll_path: pathlib.Path) -> dict[str, dict]:
    results = {}
    with dll_path.open("rb") as f:
        for tab in TABS:
            f.seek(tab["file_offset"])
            blob = f.read(tab["compressed_size"])
            decompressed = gzip.decompress(blob)
            data = json.loads(decompressed.decode("utf-8"))
            results[tab["id"]] = data
    return results


def main() -> int:
    if not DLL.exists():
        print(f"Error: {DLL} not found", file=sys.stderr)
        return 1

    tabs = extract_tabs(DLL)
    for name, content in tabs.items():
        print(f"=== Tab: {name} (type: {content.get('type')}, elements: {len(content.get('elements', []))}) ===")
        print(json.dumps(content, indent=2))
        print()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
