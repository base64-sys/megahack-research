"""Classify every recovered string into a category, anchored to its VA.

M1 asks for a string inventory split into user-visible labels, setting keys, file
paths, language keys and format strings, each anchored to a VA. `_xrefs.tsv`
already has the VAs; what is missing is the category.

Categories are assigned by **most specific rule first**, so a setting key that
also looks like a path lands in `setting-key`. Order matters and is why this is
a function rather than a dict of regexes: the first match wins, and a different
order silently produces a different inventory.

Rows carrying an `sso` marker have no VA, because those bytes exist only in a
stack frame. They are still classified - the text is real - but flagged, since
an SSO-derived key name inherits the split/merge error modes documented in
[sso-decoder.md](../notes/sso-decoder.md).

    python scripts/classify_strings.py
    python scripts/classify_strings.py --category setting-key --limit 20
"""

from __future__ import annotations

import argparse
import collections
import pathlib
import re
import sys

REPO = pathlib.Path(__file__).resolve().parent.parent
FUNCTIONS = REPO / "recovered" / "functions"
INDEX = FUNCTIONS / "_xrefs.tsv"

# Ordered rules. First match wins.
SETTING_KEY = re.compile(r"^[A-Z][A-Z0-9_]*(?:/[A-Z0-9_{}]+)+$")
API_PATH = re.compile(r"^/(?:api|v\d)/")
LANG_KEY = re.compile(r"^[a-z]{2,3}(?:\.[A-Za-z_]+)+$")
FILE_PATH = re.compile(r"^[A-Za-z0-9_.\\/-]*\.(?:png|json|dt|bin|dat|txt|xml|zip|plist)$")
FORMAT_STR = re.compile(r"%[-+ #0-9.]*[a-zA-Z]|\{[A-Za-z0-9_]*\}")
NATURAL_LABEL = re.compile(r"^[A-Z][A-Za-z0-9 ,.'!?()\-]*$")

# Added after sampling the leftovers: three distinct groups were 2,000-odd rows
# of "other", and a bucket that swallows three quarters of the inventory is not
# a category.
#
#   host-method   `PlayLayer::createCheckpoint`, `cocos2d::CCDirector::popScene`
#   third-party   OpenSSL and libc++ symbols, statically linked
#   flag          `HACK_COPY_HACK+`, `glow_1`, `onlyuser` - config toggles and
#                 asset-state names, snake_case or HACK_ prefixed
HOST_METHOD = re.compile(r"[A-Za-z_][A-Za-z0-9_]*(?:::[A-Za-z_~][A-Za-z0-9_]*)+")
THIRD_PARTY = re.compile(
    r"^(?:[a-z0-9]+_)+(?:[a-z0-9]+)$"   # OpenSSL/libc snake_case with no spaces
    r"|OPENSSL_[A-Za-z0-9_]+"
    r"|_?[A-Z][A-Za-z0-9_]*@?OPENSSL"
)
FLAG = re.compile(r"^(?:HACK_[A-Z0-9_]+[+]?|[a-z][a-z0-9]*(?:_[a-z0-9]+)+)$")
ERROR_TEXT = re.compile(r"^[a-z].*\s.*[.:]$")

CATEGORIES = [
    ("setting-key", SETTING_KEY),
    ("api-path", API_PATH),
    ("language-key", LANG_KEY),
    ("file-path", FILE_PATH),
    ("host-method", HOST_METHOD),
    ("third-party", THIRD_PARTY),
    ("flag", FLAG),
    ("format-string", FORMAT_STR),
    ("user-label", NATURAL_LABEL),
]


def classify(text: str) -> str:
    for name, pattern in CATEGORIES:
        if pattern.search(text):
            return name
    return "other"


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("--index", type=pathlib.Path, default=INDEX)
    parser.add_argument("--out", type=pathlib.Path, default=FUNCTIONS / "_strings.tsv")
    parser.add_argument("--category", help="only show this category")
    parser.add_argument("--limit", type=int, default=40)
    args = parser.parse_args(argv)

    if not args.index.is_file():
        print(f"no {args.index}; run scripts/build_xref_index.py first", file=sys.stderr)
        return 1

    rows: list[tuple[str, str, str]] = []  # (category, va, text)
    for line in args.index.read_text(encoding="utf-8").splitlines():
        if line.startswith("#"):
            continue
        parts = line.split("\t")
        if len(parts) < 4:
            continue
        va, text = parts[0], parts[3].strip().strip("'")
        if not text:
            continue
        rows.append((classify(text), va, text))

    counts = collections.Counter(c for c, _, _ in rows)
    va_backed = sum(1 for c, va, _ in rows if va.startswith("0x"))

    with args.out.open("w", encoding="utf-8") as fh:
        fh.write("# category\tstring_va\ttext\tanchored\n")
        for cat, va, text in sorted(rows, key=lambda r: (r[0], r[1])):
            anchored = "va" if va.startswith("0x") else "sso"
            fh.write(f"{cat}\t{va}\t{text}\t{anchored}\n")

    print(f"classified   : {len(rows)}")
    print(f"VA-anchored  : {va_backed}")
    print(f"SSO-only     : {len(rows) - va_backed}")
    print()
    width = max(len(n) for n, _ in CATEGORIES) + 4
    for name, count in counts.most_common():
        print(f"{name:<{width}}{count:>6}")

    if args.category:
        print()
        print(f"=== {args.category} ===")
        shown = [r for r in rows if r[0] == args.category][: args.limit]
        for _, va, text in shown:
            print(f"  {va:<14} {text!r}")
    if not args.quiet if hasattr(args, "quiet") else True:
        print()
        print(f"index: {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())