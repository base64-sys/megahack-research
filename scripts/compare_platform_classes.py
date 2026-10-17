"""Compare the Windows and Android class sets across the two C++ ABIs.

The Windows build is MSVC and carries MSVC RTTI: NUL-terminated `.?AVFoo@@`
type descriptors reachable from Complete Object Locators, which
`rtti_index.py` walks. The Android builds are clang and use the **Itanium** C++
ABI, where type names live inside `_ZN...` symbols instead.

That difference is the finding. It means the 497-class inventory cannot be
carried across to Android by reusing the same walker - the RTTI layout is a
different structure entirely - and M5 needs a separate pass rather than a rerun.

What is comparable across both is the set of *class names*, so that is what this
extracts: namespaces and class names from each ABI, then the overlap.

    python scripts/compare_platform_classes.py
    python scripts/compare_platform_classes.py --grep hackpro
"""

from __future__ import annotations

import argparse
import collections
import pathlib
import re
import sys

REPO = pathlib.Path(__file__).resolve().parent.parent
BASE = REPO / "samples" / "absolllute.hackmega"

WINDOWS = BASE / "absolllute.hackmega_original.dll"
ANDROID = {
    "android64": BASE / "absolllute.megahack.android64.so",
    "android32": BASE / "absolllute.megahack.android32.so",
}

# MSVC type descriptor: `.?AVName@ns@@` or `.?AU...`
MSVC_RE = re.compile(rb"\.\?[AU]([A-Za-z_$?<>][^\x00]{1,200}?)(?=@@)")

# Itanium symbol component lengths are decimal digits then the identifier.
ITANIUM_RE = re.compile(rb"_ZN([0-9A-Za-z_$.]+)")


def itanium_components(sym: bytes) -> list[str]:
    """Split an Itanium `_ZN...` body into its `<len><name>` components.

    Stops at the first component that is not a plain identifier, which is where
    the nested-name encoding gives way to template arguments or a function
    parameter list. Good enough to recover class and namespace names, which is
    what a cross-platform comparison needs.
    """
    parts: list[str] = []
    i = 0
    while i < len(sym):
        j = i
        while j < len(sym) and sym[j : j + 1].isdigit():
            j += 1
        if j == i or j >= len(sym):
            break
        n = int(sym[i:j])
        name = sym[j : j + n]
        if len(name) != n:
            break
        try:
            decoded = name.decode("ascii")
        except UnicodeDecodeError:
            break
        if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", decoded):
            break
        parts.append(decoded)
        i = j + n
    return parts


def scan(path: pathlib.Path, abi: str) -> set[str]:
    data = path.read_bytes()
    if abi == "msvc":
        return {
            m.group(1).decode("ascii", errors="replace")
            for m in MSVC_RE.finditer(data)
        }
    out: set[str] = set()
    for m in ITANIUM_RE.finditer(data):
        comps = itanium_components(m.group(1))
        if comps:
            out.add("::".join(comps))
    return out


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("--grep", help="only show classes containing this")
    args = parser.parse_args(argv)

    if not WINDOWS.is_file():
        print("baseline DLL missing; nothing to compare", file=sys.stderr)
        return 1

    win = scan(WINDOWS, "msvc")
    android = {k: scan(v, "itanium") for k, v in ANDROID.items() if v.is_file()}

    print(f"{'binary':<34}{'ABI':<9}{'class names'}")
    print(f"{WINDOWS.name:<34}{'MSVC':<9}{len(win)}")
    for k, v in android.items():
        print(f"{ANDROID[k].name:<34}{'Itanium':<9}{len(v)}")
    print()

    # Compare on the innermost identifier, since the two ABIs encode namespaces
    # differently (`hackpro::extensions::Home` vs `hackpro.extensions.Home`) and
    # the Android set is polluted by system libraries the Windows build links
    # differently.
    def leaf(names: set[str]) -> set[str]:
        return {n.rsplit("::", 1)[-1] for n in names if len(n) > 2}

    win_leaf = leaf(win)
    for k, v in android.items():
        a_leaf = leaf(v)
        shared = win_leaf & a_leaf
        print(f"{k}: {len(shared)} class names shared with Windows, "
              f"{len(win_leaf - a_leaf)} Windows-only")
    print()

    if args.grep:
        needle = args.grep.lower()
        print(f"=== classes containing '{args.grep}' ===")
        print(f"windows  : {sorted(n for n in win if needle in n.lower())[:20]}")
        for k, v in android.items():
            hits = sorted(n for n in v if needle in n.lower())
            print(f"{k:<9}: {hits[:20]}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())