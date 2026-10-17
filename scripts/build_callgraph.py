"""Build a call graph over the recovered tree.

The recovered pseudocode has no cross-references between files, so finding the
callees of a function means grepping the whole tree. This extracts every static
call edge, folds in the hook-wrapper layer so thunks resolve to what they
really call, and reports the most-called functions.

Method: Ghidra names every decompiled call target `FUN_<va>`, so a call edge is
a `FUN_<hex>(` occurrence in a file. That catches direct calls only, which is
the correct scope - indirect calls through vtables or the wrapper layer are
either folded in from _wrappers.tsv or genuinely unresolvable statically.

    python scripts/build_callgraph.py
    python scripts/build_callgraph.py --callers-of 180169eb0
    python scripts/build_callgraph.py --top 40

Output: recovered/functions/_calls.tsv and _callers.tsv, gitignored with the rest.
"""

from __future__ import annotations

import argparse
import collections
import pathlib
import re
import sys

REPO = pathlib.Path(__file__).resolve().parent.parent
FUNCTIONS = REPO / "recovered" / "functions"
IMAGE_BASE = 0x180000000

CALL_RE = re.compile(r"\bFUN_([0-9a-f]{6,16})\s*\(")
DEF_RE = re.compile(r"^([0-9a-f]{8,16})_")
THUNK_RE = re.compile(r"\bFUN_([0-9a-f]{6,16})\s*=\s*\(code\s*\*\)")


def load_wrappers() -> dict[str, str]:
    """thunk stem -> containing function of its real hook target."""
    path = FUNCTIONS / "_wrappers.tsv"
    if not path.exists():
        return {}
    out = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        if line.startswith("#"):
            continue
        parts = line.split("\t")
        if len(parts) >= 5 and parts[4] != "-":
            out[parts[0]] = parts[4].split("+")[0]
    return out


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--functions", type=pathlib.Path, default=FUNCTIONS)
    parser.add_argument("--top", type=int, default=25)
    parser.add_argument("--callers-of", help="show who calls this function (hex address)")
    parser.add_argument("--quiet", action="store_true")
    args = parser.parse_args(argv)

    if not args.functions.is_dir():
        raise SystemExit(f"no recovered tree at {args.functions}; run scripts/decompile_all.py first")

    wrappers = load_wrappers()
    # stem -> VA, and VA -> stem
    va_to_stem: dict[int, str] = {}
    files = sorted(args.functions.glob("*.c"))
    for path in files:
        m = DEF_RE.match(path.stem)
        if m:
            va_to_stem[int(m.group(1), 16)] = path.stem

    edges: set[tuple[str, str]] = set()
    unresolved = 0
    self_edges = 0
    for path in files:
        caller = path.stem
        text = path.read_text(encoding="utf-8", errors="replace")
        # Skip the header comment and the signature, both of which contain the
        # function's own `FUN_<va>(` and would otherwise make every function its
        # own caller. Anchoring on the comment terminator is more robust than
        # looking for a bare "\n{\n", which misses bodies that open with
        # declarations.
        header_end = text.find("*/")
        brace = text.find("{", header_end if header_end >= 0 else 0)
        if brace < 0:
            continue
        body = text[brace:]
        for match in CALL_RE.finditer(body):
            va = int(match.group(1), 16)
            target = va_to_stem.get(va)
            if target is None:
                unresolved += 1
                continue
            if target == caller:
                self_edges += 1
                continue
            edges.add((caller, target))

    # Fold the hook-wrapper layer: a thunk's call edge really points at the
    # function it hooks, which is what a reader wants to follow.
    folded = 0
    for caller, target in list(edges):
        real_target = wrappers.get(target)
        if real_target and real_target != target:
            edges.discard((caller, target))
            edges.add((caller, real_target))
            folded += 1

    out_edges = args.functions / "_calls.tsv"
    with out_edges.open("w", encoding="utf-8") as fh:
        fh.write("# caller\tcallee\n")
        for caller, callee in sorted(edges):
            fh.write(f"{caller}\t{callee}\n")

    callers: dict[str, set[str]] = collections.defaultdict(set)
    callees: dict[str, set[str]] = collections.defaultdict(set)
    for caller, callee in edges:
        callees[caller].add(callee)
        callers[callee].add(caller)

    out_callers = args.functions / "_callers.tsv"
    with out_callers.open("w", encoding="utf-8") as fh:
        fh.write("# function\tdistinct_callers\tcallers\n")
        for fn, srcs in sorted(callers.items()):
            fh.write(f"{fn}\t{len(srcs)}\t{','.join(sorted(srcs))}\n")

    nodes_with_edges = len(set(va_to_stem.values()) & (set(callers) | set(callees)))
    if not args.quiet:
        print(f"functions recovered      : {len(files)}")
        print(f"functions in graph       : {nodes_with_edges}")
        print(f"static call edges        : {len(edges)}")
        print(f"  folded from wrappers   : {folded}")
        print(f"  self-calls skipped     : {self_edges}")
        print(f"targets not in the tree  : {unresolved} (external/indirect)")
        print(f"edges                    : {out_edges}")
        print(f"callers                  : {out_callers}")
        print()
        print(f"=== most-called functions (top {args.top}) ===")
        for fn, srcs in sorted(callers.items(), key=lambda kv: -len(kv[1]))[: args.top]:
            label = fn.split("_", 1)[-1]
            print(f"  {len(srcs):>4}  {label}")

    if args.callers_of:
        needle = args.callers_of.lower().removeprefix("0x")
        print()
        print(f"=== callers of {args.callers_of} ===")
        found = False
        for fn, srcs in callers.items():
            if fn.lower().startswith(needle):
                found = True
                print(f"  {fn}")
                for src in sorted(srcs):
                    print(f"      <- {src}")
        if not found:
            print("  (no static callers found; may be reached indirectly or via a hook)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())