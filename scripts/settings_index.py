"""Index Mega Hack's settings registry: which ids are read, and from where.

Several notes carry settings as bare numbers - 245, 247, 501, 502, 761, 762,
801 - because they were read straight off a `mov edx, <id>` before a call into the
config accessor. This turns those scattered numbers into one table.

Two accessors are recognised:

    FUN_18008f420(this, id) -> bool     getConfigVar<bool>
    FUN_18002de50(this, id) -> bool     a second boolean accessor

There is also a key-based form, FUN_18007cb60(this, out, "KEY"), which indexes
string keys rather than numeric ids; both are emitted.

    python scripts/settings_index.py
    python scripts/settings_index.py --grep 245
"""

from __future__ import annotations

import argparse
import collections
import pathlib
import re
import sys

REPO = pathlib.Path(__file__).resolve().parent.parent
FUNCTIONS = REPO / "recovered" / "functions"

# accessor name -> what it means
ACCESSORS = {
    "FUN_18008f420": "getConfigVar<bool>",
    "FUN_18002de50": "getFlag",
    "FUN_180003d80": "getConfigVar<scalar>",
}
NUMERIC_RE = re.compile(
    r"\b(" + "|".join(sorted(ACCESSORS)) + r")\s*\([^;]*?,\s*(0x[0-9a-f]+|\d+)\s*\)"
)
KEYED_RE = re.compile(r"\bFUN_18007cb60\s*\([^;]*?,\s*&([A-Za-z_0-9]+)\s*\)")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--functions", type=pathlib.Path, default=FUNCTIONS)
    parser.add_argument("--grep", help="only report ids containing this substring")
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args(argv)

    if not args.functions.is_dir():
        raise SystemExit(f"no recovered tree at {args.functions}; run scripts/decompile_all.py first")

    files = sorted(args.functions.glob("*.c"))
    if not files:
        raise SystemExit(f"no .c files in {args.functions}")

    ids: dict[int, set[str]] = collections.defaultdict(set)
    which: dict[int, set[str]] = collections.defaultdict(set)
    keyed: dict[str, set[str]] = collections.defaultdict(set)

    for path in files:
        text = path.read_text(encoding="utf-8", errors="replace")
        for name, raw in NUMERIC_RE.findall(text):
            value = int(raw, 0)
            ids[value].add(path.stem)
            which[value].add(ACCESSORS[name])
        for symbol in KEYED_RE.findall(text):
            keyed[symbol].add(path.stem)

    out = args.functions / "_settings.tsv"
    with out.open("w", encoding="utf-8") as fh:
        fh.write("# id\taccessors\treads\tfunctions\n")
        for value in sorted(ids):
            fh.write(
                f"{value}\t{'|'.join(sorted(which[value]))}\t{len(ids[value])}\t"
                f"{','.join(sorted(ids[value]))}\n"
            )
        fh.write("#\n# keyed lookups: FUN_18007cb60(this, out, KEY)\n")
        for symbol in sorted(keyed):
            fh.write(f"{symbol}\tkeyed\t{len(keyed[symbol])}\t{','.join(sorted(keyed[symbol]))}\n")

    if not args.quiet:
        total_reads = sum(len(v) for v in ids.values())
        print(f"recovered functions    : {len(files)}")
        print(f"distinct numeric ids   : {len(ids)}")
        print(f"total id reads         : {total_reads}")
        print(f"id range               : {min(ids)}..{max(ids)}")
        print(f"distinct keyed lookups : {len(keyed)}")
        print(f"table                  : {out}")
        print()
        for value in sorted(ids):
            marker = "  <-- documented but still unnamed" if value in TRACKED else ""
            print(f"  {value:<6} {len(ids[value]):>4} reads  {'|'.join(sorted(which[value]))}{marker}")
        if keyed:
            print()
            print("keyed lookups:")
            for symbol in sorted(keyed):
                print(f"  {symbol:<28} {len(keyed[symbol])} reads")

    if args.grep:
        needle = args.grep.lower()
        print()
        print(f"=== ids containing {args.grep!r} ===")
        for value in sorted(ids):
            if needle in str(value):
                print(f"  {value}: {len(ids[value])} reads")
                for fn in sorted(ids[value])[:10]:
                    print(f"      {fn}")
    return 0


TRACKED = {245, 247, 501, 502, 761, 762, 801}


if __name__ == "__main__":
    raise SystemExit(main())