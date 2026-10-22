"""Index Mega Hack's settings registry: which ids are read, and from where.

Several notes carry settings as bare numbers - 245, 247, 501, 502, 761, 762,
801 - because they were read straight off a `mov edx, <id>` before a call into the
config accessor. This turns those scattered numbers into one table.

Two bool accessors are recognised:

    FUN_18008f420(this, id) -> bool     isHackEnabled (Map B hack flags)
    FUN_18002de50(this, id) -> bool     getConfigVar<bool> (Map A config values)

There is also a key-based form, FUN_18007cb60(this, out, "KEY"), which indexes
string keys rather than numeric ids; both are emitted.

The ids are 64-bit hackpro::Tag values, not small numbers; see
notes/settings-registry.md for why that matters and why a fourth accessor over a
second registry was invisible here until its argument position was made explicit.

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

# accessor name -> (index of the tag argument, what it means)
#
# The index is explicit because the accessors do NOT share a shape, and guessing
# is what hid two tags from this table twice. `FUN_18008f420(this, id)` and
# `FUN_18002de50(this, id)` take the tag second; `FUN_18002ee40(registry, out,
# tag)` takes it third, behind an out-parameter. A single regex that took
# "the first comma-separated argument that looks numeric" happened to match the
# three-argument form by accident and did not match it reliably, so tag 285 was
# absent from the table with no error anywhere.
#
# The lesson this file already records, hit a second time: an index built from a
# guessed accessor list is a sample of one, not a registry. Enumerate, do not
# assume.
ACCESSORS = {
    # Export identities (2026-10-23): getConfigVar<bool> at 0x180073880 calls
    # FUN_18002de50 (Map A mask +0xD8); isHackEnabled at 0x180072FF0 calls FUN_18008f420
    # (Map B mask +0x160). The setters are FUN_180027c30 (setConfigVar<bool>, Map A) and
    # FUN_18008b490 (setHackEnabled, Map B); they are not value readers, so they are not
    # indexed here.
    "FUN_18008f420": (1, "isHackEnabled"),
    "FUN_18002de50": (1, "getConfigVar<bool>"),
    "FUN_180003d80": (1, "getConfigVar<scalar>"),
    "FUN_18002e4c0": (1, "getConfigVar<int>"),
    "FUN_18002d710": (2, "getConfigVar<std::string>"),
    "FUN_18002ee40": (2, "getConfigVar<hackpro::Bytes>"),
}


def accessor_regex(name: str, index: int) -> re.Pattern[str]:
    """Match `name(...)` and capture the numeric literal in argument `index`.

    Arguments are split on top-level commas so a nested call in an earlier
    argument does not shift the index, and the captured argument must be a bare
    literal rather than anything containing a name.
    """
    return re.compile(
        name + r"\s*\(([^;]*?)\)",
    )


ARG_SPLIT = re.compile(r",")


def numeric_args(call_args: str) -> list[int | None]:
    """Return one entry per top-level argument: its int value, or None."""
    # Nested calls and pointer casts can contain commas, so track depth.
    depth = 0
    parts: list[str] = []
    buf: list[str] = []
    for ch in call_args:
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
        if ch == "," and depth == 0:
            parts.append("".join(buf))
            buf = []
        else:
            buf.append(ch)
    parts.append("".join(buf))

    out: list[int | None] = []
    for part in parts:
        part = part.strip()
        if re.fullmatch(r"0x[0-9a-fA-F]+|\d+", part):
            out.append(int(part, 0))
        else:
            out.append(None)
    return out


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
        for name, (index, label) in ACCESSORS.items():
            for call_args in accessor_regex(name, index).findall(text):
                parsed = numeric_args(call_args)
                if len(parsed) <= index:
                    continue
                value = parsed[index]
                if value is None:
                    continue
                ids[value].add(path.stem)
                which[value].add(label)
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