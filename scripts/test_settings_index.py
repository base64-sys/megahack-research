#!/usr/bin/env python3
"""Pin the settings-index facts that have silently gone wrong twice.

Two tags have been missing from `_settings.tsv` at different times, both times with
no error anywhere and a perfectly plausible table:

- tag 501, because the original sweep recognised only two accessors;
- tag 285, because `FUN_18002ee40` takes its tag as the *third* argument and the
  matcher guessed "the first numeric-looking argument".

So the guards here are deliberately two-sided. The argument splitter is tested as
a pure function, because that is the part that broke and it needs no sample set to
test. The recovered-tree assertions pin the specific tags so a future accessor
cannot go missing without the suite noticing.

Skips the tree-dependent checks when no recovered tree is present, so a fresh clone
stays green.
"""

from __future__ import annotations

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from settings_index import (  # noqa: E402
    ACCESSORS,
    FUNCTIONS,
    accessor_regex,
    numeric_args,
)

# Tags that were absent at some point and must never be again.
PINNED_TAGS = {
    501: "getConfigVar<scalar>",
    285: "second-registry accessor",  # 0x11d, the three-argument accessor
    309: "getConfigVar<int>",        # 0x135, Int accessor (FUN_18002e4c0)
    871: "getConfigVar<std::string>",  # 0x367, String accessor (FUN_18002d710)
}

# FUN_1801904F0 reads six consecutive tags into six locals; see
# notes/settings-registry.md.
TAG_GROUP = [0x353, 0x354, 0x355, 0x356, 0x357, 0x358]


def check_splitter(failures: list[str]) -> None:
    def check(cond: bool, label: str, detail: str = "") -> None:
        print(f"{'ok  ' if cond else 'FAIL'}  {label}{'  ' + detail if detail else ''}")
        if not cond:
            failures.append(label)

    # Two-argument form: the tag is second.
    check(numeric_args("param_1,0x11d") == [None, 0x11D], "two-arg call splits", str(numeric_args("param_1,0x11d")))

    # Three-argument form behind an out-parameter: the tag is third.
    three = numeric_args("uVar3,local_38,0x11d")
    check(three == [None, None, 0x11D], "three-arg call splits", str(three))

    # A comma inside a cast must not shift the index. This call really occurs.
    cast = numeric_args("param_1,(vector<unsigned_int,std::allocator<unsigned_int>_> *)0,0x123")
    check(
        cast[2] == 0x123,
        "comma inside a cast does not shift the index",
        str(cast),
    )

    # A comma inside a nested call must not shift the index either.
    nested = numeric_args("FUN_1800abc(a,b),out,0x7")
    check(nested[2] == 0x7, "comma inside a nested call does not shift", str(nested))

    # Only bare literals count; a name is not a tag.
    check(numeric_args("uVar1,&local_70,param_2")[2] is None, "a name is not a tag")
    check(numeric_args("uVar1,&local_70,3")[2] == 3, "decimal literal is a tag")
    check(numeric_args("x") == [None], "single non-numeric argument")
    check(numeric_args("0") == [0], "zero is a tag, not a missing one")

    # The accessors must keep their recorded positions; this is the exact thing
    # that was guessed before.
    check(
        ACCESSORS["FUN_18008f420"][0] == 1,
        "getConfigVar<bool> takes the tag second",
    )
    check(
        ACCESSORS["FUN_18002ee40"][0] == 2,
        "second-registry accessor takes the tag third",
    )


def check_tree(failures: list[str]) -> None:
    def check(cond: bool, label: str, detail: str = "") -> None:
        print(f"{'ok  ' if cond else 'FAIL'}  {label}{'  ' + detail if detail else ''}")
        if not cond:
            failures.append(label)

    table = FUNCTIONS / "_settings.tsv"
    if not table.exists():
        print(f"SKIP  no _settings.tsv at {table}")
        return

    rows: dict[int, tuple[str, str]] = {}
    keyed = 0
    for line in table.read_text(encoding="utf-8").splitlines():
        if line.startswith("#") or not line.strip():
            continue
        parts = line.split("\t")
        if parts[1] == "keyed":
            keyed += 1
            continue
        rows[int(parts[0])] = (parts[1], parts[2])

    check(len(rows) >= 149, "at least 149 tags", f"got {len(rows)}")
    check(keyed == 4, "4 keyed lookups", f"got {keyed}")

    for tag, accessor in sorted(PINNED_TAGS.items()):
        present = tag in rows
        check(present, f"tag {tag} present", "" if present else "MISSING - accessor lost?")
        if present:
            check(
                accessor in rows[tag][0],
                f"tag {tag} attributed to {accessor}",
                rows[tag][0],
            )

    for tag in TAG_GROUP:
        present = tag in rows
        check(present, f"tag {tag:#x} present", "" if present else "MISSING")
        if present:
            check(
                "second-registry" in rows[tag][0],
                f"tag {tag:#x} is a second-registry tag",
                rows[tag][0],
            )

    # The 284 -> 292 gap that made the bug visible must stay closed.
    check(285 in rows, "the 284 -> 292 gap is closed")

    # Sanity: the splitter still agrees with a real call in the recovered tree.
    sample = FUNCTIONS / "1801904f0_FUN_1801904f0.c"
    if sample.exists():
        text = sample.read_text(encoding="utf-8", errors="replace")
        found = []
        for name, (index, _) in ACCESSORS.items():
            for call in accessor_regex(name, index).findall(text):
                parsed = numeric_args(call)
                if len(parsed) > index and parsed[index] is not None:
                    found.append(parsed[index])
        check(
            all(t in rows for t in found) and found,
            "FUN_1801904F0's tags all reach the table",
            f"{sorted(set(found))}",
        )


def main() -> int:
    failures: list[str] = []

    print("argument splitter (no sample set needed)")
    check_splitter(failures)

    print()
    print("recovered tree")
    check_tree(failures)

    print()
    if failures:
        print(f"{len(failures)} FAILED: {', '.join(failures)}")
        return 1
    print("all checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())