#!/usr/bin/env python3
"""Index every hook Mega Hack installs: the GD method, the target, and the callback.

Mega Hack reaches Geometry Dash through `GetModuleHandleA(NULL)` plus hard-coded
RVAs into the host image, and through name resolution against the host's mangled
symbols. It registers each one through Geode under a convention its debug types
call `TulipConvention` -- see notes/hook-dispatch.md. That gives a fixed,
greppable call shape:

    FUN_1800ab850(FUN_180108960(), uVar2, CALLBACK, "PlayLayer::resetLevel",
                  0xffffffe0, 1)

with `uVar2` assigned just above from either

    uVar2 = FUN_18010a370(uVar2, 0x3b8eb0);          # hard-coded RVA
or
    uVar2 = FUN_18010a5a0(uVar2, &mangled, &module);  # resolved by name

so the method name, the callback and (where applicable) the host RVA all sit
together in the recovered text. The registration helper itself varies by
callback signature -- there are over a hundred of them, all calling
`geode::hook::createConvention` -- but the call shape is the same, so the hook
surface is enumerable without naming the helpers individually.

This writes `_hook_sites.tsv`: one row per registration site.

    python scripts/hook_sites.py
    python scripts/hook_sites.py --method resetLevel
"""

from __future__ import annotations

import argparse
import csv
import pathlib
import re
import sys

REPO = pathlib.Path(__file__).resolve().parent.parent
FUNCTIONS = REPO / "recovered" / "functions"
OUT = FUNCTIONS / "_hook_sites.tsv"

REGISTER_RE = re.compile(
    r'(FUN_180[0-9a-f]{6})\s*\(([^)]*"[^"]+::[^"]+"[^)]*)\)\s*;', re.S
)
NAME_RE = re.compile(r'"([^"]+)"')
CALLBACK_RE = re.compile(
    r"&?(LAB_[0-9a-f]+|thunk_FUN_180[0-9a-f]{6}|FUN_180[0-9a-f]{6}|DAT_180[0-9a-f]{6})\b"
)
RVA_RE = re.compile(r"FUN_18010a370\s*\([^,]+,\s*(0x[0-9a-fA-F]+)\s*\)")
NAME_RESOLVER_RE = re.compile(r"FUN_18010a5a0\s*\(")
ORDER_RE = re.compile(r"(0x[0-9a-fA-F]+|-?\d+)\s*,\s*1\s*$")


def func_va(filename: str) -> str:
    stem = filename.split("_", 1)[0]
    try:
        return "0x" + stem.upper()
    except ValueError:  # pragma: no cover
        return "?"


def scan(path: pathlib.Path) -> list[dict[str, str]]:
    text = path.read_text(encoding="utf8", errors="replace")
    if not REGISTER_RE.search(text):
        return []

    rows: list[dict[str, str]] = []
    pending_rva = "?"
    pending_kind = "unknown"
    cursor = 0
    for match in REGISTER_RE.finditer(text):
        between = text[cursor : match.start()]
        rvas = RVA_RE.findall(between)
        if rvas:
            pending_rva = rvas[-1]
            pending_kind = "rva"
        elif NAME_RESOLVER_RE.search(between):
            pending_rva = "name-resolved"
            pending_kind = "name"

        args = match.group(2)
        name = NAME_RE.search(args)
        if not name or "::" not in name.group(1):
            cursor = match.end()
            continue
        order = ORDER_RE.search(args.strip())
        if not order:
            cursor = match.end()
            continue
        callback = CALLBACK_RE.search(args)
        cb_text = callback.group(1) if callback else "?"
        if cb_text.startswith("LAB_"):
            kind = "label"
        elif cb_text.startswith("DAT_"):
            kind = "data"
        else:
            kind = "function"
        rows.append(
            {
                "registrar_va": func_va(path.name),
                "gd_method": name.group(1),
                "gd_rva": pending_rva,
                "target_kind": pending_kind,
                "callback": cb_text,
                "callback_kind": kind,
                "helper": match.group(1),
                "order": order.group(1),
            }
        )
        cursor = match.end()

    return rows


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("--functions", type=pathlib.Path, default=FUNCTIONS)
    parser.add_argument("--out", type=pathlib.Path, default=OUT)
    parser.add_argument("--method", help="only methods containing this")
    parser.add_argument("--limit", type=int, default=0)
    args = parser.parse_args(argv)

    if not args.functions.is_dir():
        print(f"missing {args.functions}", file=sys.stderr)
        return 1

    all_rows: list[dict[str, str]] = []
    for path in sorted(args.functions.glob("*.c")):
        all_rows.extend(scan(path))

    methods = sorted({r["gd_method"] for r in all_rows})
    print(f"scanned {args.functions}")
    print(f"  registration sites : {len(all_rows)}")
    print(f"  distinct GD methods: {len(methods)}")
    by_method: dict[str, int] = {}
    for row in all_rows:
        by_method[row["gd_method"]] = by_method.get(row["gd_method"], 0) + 1
    for name, count in sorted(by_method.items(), key=lambda kv: (-kv[1], kv[0]))[:20]:
        print(f"    {count:>3}  {name}")
    name_resolved = sum(1 for r in all_rows if r["target_kind"] == "name")
    print(f"  name-resolved targets: {name_resolved}")

    if args.method:
        hits = [r for r in all_rows if args.method in r["gd_method"]]
        print(f"\n{len(hits)} matching sites (index at {args.out} left untouched)")
        for row in hits:
            print(
                f"  {row['registrar_va']}  {row['gd_method']:32} "
                f"rva={row['gd_rva']:16} {row['callback']}"
            )
        return 0

    rows = all_rows
    if args.limit:
        rows = rows[: args.limit]

    args.out.parent.mkdir(parents=True, exist_ok=True)
    fields = [
        "registrar_va",
        "gd_method",
        "gd_rva",
        "target_kind",
        "callback",
        "callback_kind",
        "helper",
        "order",
    ]
    with args.out.open("w", encoding="utf8", newline="") as handle:
        handle.write("# " + "\t".join(fields) + "\n")
        handle.write("#\n")
        handle.write("# Every hook Mega Hack registers. The method name and host target come from\n")
        handle.write("# the registration call; see notes/hook-dispatch.md. `target_kind=rva` means\n")
        handle.write("# the target is a hard-coded host RVA (positional pairing with\n")
        handle.write("# FUN_18010a370(..., rva) in the same function); `name` means it is\n")
        handle.write("# resolved at runtime from a mangled host symbol (FUN_18010a5a0).\n")
        handle.write("# callback_kind=label/data means Ghidra did not split that callback into\n")
        handle.write("# its own function, so it has no recovered .c to read.\n")
        writer = csv.DictWriter(handle, fieldnames=fields, delimiter="\t", lineterminator="\n")
        writer.writerows(rows)

    print(f"\nwrote {len(rows)} rows to {args.out}")
    labels = sum(1 for r in rows if r["callback_kind"] == "label")
    data = sum(1 for r in rows if r["callback_kind"] == "data")
    if labels or data:
        print(f"  {labels} callbacks are Ghidra labels, {data} are data slots")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
