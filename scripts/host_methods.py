#!/usr/bin/env python3
"""Export the Geometry Dash host-method inventory Mega Hack resolves by name.

Mega Hack does not statically import GD gameplay symbols — `notes/host-abi.md`
records that zero GD classes are imported. It resolves them at runtime by string
name instead, so the *string* is the only handle available for finding the call.

This writes `_host_methods.tsv`: every code-referenced string that classifies as
`host-method`, its VA, and which functions name it. That is the host ABI surface
Mega Hack actually touches, in one table, without guessing at class layouts.

The category comes from `classify_strings.py`'s rule; this script only joins it to
`_xrefs.tsv`, so the two cannot disagree about what counts as a host method.

    python scripts/host_methods.py
    python scripts/host_methods.py --grep PlayLayer
"""

from __future__ import annotations

import argparse
import csv
import pathlib
import sys

REPO = pathlib.Path(__file__).resolve().parent.parent
FUNCTIONS = REPO / "recovered" / "functions"
STRINGS = FUNCTIONS / "_strings.tsv"
XREFS = FUNCTIONS / "_xrefs.tsv"
OUT = FUNCTIONS / "_host_methods.tsv"


def read_tsv(path: pathlib.Path) -> list[dict[str, str]]:
    """Read a TSV that may carry `#` comment lines, header included.

    `build_xref_index.py` writes its header as `# string_va\t...`, so a plain
    DictReader silently treats the first data row as the header and every
    lookup by column name raises KeyError.
    """
    header: list[str] | None = None
    rows: list[dict[str, str]] = []
    with path.open(encoding="utf8") as handle:
        for line in handle:
            line = line.rstrip("\n")
            if not line:
                continue
            if line.startswith("#"):
                if header is None:
                    header = line.lstrip("# ").split("\t")
                continue
            if header is None:
                header = line.split("\t")
                continue
            values = line.split("\t")
            values += [""] * (len(header) - len(values))
            rows.append(dict(zip(header, values)))
    return rows


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("--strings", type=pathlib.Path, default=STRINGS)
    parser.add_argument("--xrefs", type=pathlib.Path, default=XREFS)
    parser.add_argument("--out", type=pathlib.Path, default=OUT)
    parser.add_argument("--grep", help="only show host methods containing this")
    parser.add_argument("--limit", type=int, default=0)
    args = parser.parse_args(argv)

    if not args.strings.exists():
        print(f"missing {args.strings}; run classify_strings.py first", file=sys.stderr)
        return 1
    if not args.xrefs.exists():
        print(f"missing {args.xrefs}; run build_xref_index.py first", file=sys.stderr)
        return 1

    referrers: dict[str, list[str]] = {}
    for row in read_tsv(args.xrefs):
        va = row.get("string_va", "")
        # `functions` is one comma-joined cell, so a single row can name several
        # referrers. Counting rows instead of names understates the surface:
        # `AppDelegate::trySaveGame` has one row and two referrers.
        for name in row.get("functions", "").split(","):
            if name:
                referrers.setdefault(va, []).append(name)

    hosts = [
        row
        for row in read_tsv(args.strings)
        if row.get("category") == "host-method"
    ]

    out_rows = []
    for row in hosts:
        va = row.get("string_va", "")
        # `_strings.tsv` already writes VAs as `0x…`; don't double the prefix.
        if va and not va.startswith("0x"):
            va = "0x" + va
        name = row.get("text", "")
        if args.grep and args.grep not in name:
            continue
        funcs = referrers.get(va, [])
        # `_xrefs.tsv` keeps one row per string, so its `refs` column counts
        # strings, not referencing functions. The useful number here is how many
        # functions name this method, so it is recounted from `functions`
        # rather than copied — `AppDelegate::trySaveGame` has one row but two
        # referrers, and reporting 1 would understate the surface.
        out_rows.append(
            {
                "va": va,
                "funcs": len(funcs),
                "functions": ",".join(funcs),
                "method": name,
            }
        )

    out_rows.sort(key=lambda r: (-r["funcs"], r["method"]))
    if args.limit:
        out_rows = out_rows[: args.limit]

    def emit(rows: list[dict[str, object]]) -> None:
        with args.out.open("w", encoding="utf8", newline="") as handle:
            handle.write("# host_method_va\tfuncs\tfunctions\tmethod\n")
            handle.write("#\n")
            handle.write("# Runtime-resolved Geometry Dash host methods. Mega Hack imports zero\n")
            handle.write("# GD gameplay symbols, so these strings are the only handle on the host ABI.\n")
            handle.write("# `funcs` counts referencing functions, recounted here rather than taken\n")
            handle.write("# from _xrefs.tsv, whose `refs` column counts strings.\n")
            writer = csv.DictWriter(
                handle,
                fieldnames=["va", "funcs", "functions", "method"],
                delimiter="\t",
                lineterminator="\n",
            )
            writer.writerows(rows)

    args.out.parent.mkdir(parents=True, exist_ok=True)

    if args.grep:
        # A filtered query must not overwrite the index it was asked about: the
        # first version wrote whatever survived --grep, so `--grep zzz` silently
        # truncated _host_methods.tsv to zero rows and the next reader got an
        # empty index with no error.
        print(f"{len(out_rows)} matching host methods (index at {args.out} left untouched)")
        print()
        for row in out_rows:
            print(f"  {row['va']}  {row['funcs']:>3} funcs  {row['method']}")
            if row["functions"]:
                print(f"        {row['functions']}")
        return 0

    emit(out_rows)

    print(f"wrote {len(out_rows)} host methods to {args.out}")
    print(f"  distinct methods : {len(hosts)}")
    print(f"  total functions naming them : {sum(r['funcs'] for r in out_rows)}")
    multi = sum(1 for r in out_rows if r["funcs"] > 1)
    print(f"  named by >1 function : {multi}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())