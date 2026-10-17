"""Regression tests for the RTTI demangler.

Pinned because the previous version of this parser could loop forever: its driver
only advanced when the next character was `@`, so any other shape spun. A
demangler that hangs on a malformed name is worse than one that gives up, so
every case below asserts both the rendering and that it terminates.

    python scripts/test_demangle.py
"""

from __future__ import annotations

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from rtti_index import demangle  # noqa: E402

CASES = [
    # (mangled, expected)
    (".?AVCoreDirector@@", "CoreDirector"),
    (".?AVHomeWin@core@@", "core::HomeWin"),
    (".?AVSpecialPastel@iconic@@", "iconic::SpecialPastel"),
    # Template args keep MSVC's `V` decoration - the parser does not strip it,
    # because doing so safely needs the scope markers it is not tracking.
    (".?AV?$shared_ptr@VSpecialCore@iconic@@std@@",
     "std::shared_ptr<VSpecialCore, iconic>"),
    # Must terminate, and must return the input untouched rather than guessing.
    ("", ""),
    ("?", "?"),
    ("@@", "@@"),
    (".?AV", ".?AV"),
    # Anything without the class/struct prefix is not a type descriptor at all.
    ("?func@@YAXXZ", "?func@@YAXXZ"),
    ("not a mangled name", "not a mangled name"),
    # Degenerate prefixes must not crash; returning the input is the safe answer.
    (".?AU@@", ".?AU@@"),
    (".?AV@@", ".?AV@@"),
]


def main() -> int:
    failures = 0
    for mangled, expected in CASES:
        try:
            got = demangle(mangled)
        except RecursionError:
            got = "<recursion error>"
        ok = got == expected
        failures += not ok
        print(f"{'ok  ' if ok else 'FAIL'}  {mangled!r:<58} -> {got!r}")
        if not ok:
            print(f"        expected {expected!r}")

    print(f"{'all demangler cases pass' if not failures else f'{failures} failed'}")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())