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

# Statically-linked library code (OpenSSL, zlib, ...) begins after a 0x528a0-byte
# gap at 0x180352640; the first function in that region is at 0x1803A4EE0.
# Mega Hack's own code lives below it. A snake_case string referenced only by
# functions at/above this boundary is a library-internal name, not a Mega Hack
# identifier. Source: recovered/functions/*.c filename address distribution
# (bounds.py analysis, 2026-10-21): largest gap in the function set is
# 0x1802FFDA0 -> 0x180352640 -> 0x1803A4EE0.
LIBRARY_REGION_START = 0x1803A4EE0

# Ordered rules. First match wins.
SETTING_KEY = re.compile(
    r"^(?:[A-Z][A-Z0-9_]*(?:/[A-Z0-9_{}]+)+|"
    r"SHORTCUT_[A-Z0-9_]+|OPTIONS_[A-Z0-9_]+|"
    r"CURRENT_PROFILE|ALL_PROFILES|CHEAT_SAFETY|UNKNOWN_KEY|"
    r"V_(?:BOOL|BYTES|INT|STRING|DECIMAL)|"
    r"(?:tag|action|nosave|default|dropdown|radio|decimal|rules|elements|single|dual|live))$"
)
API_PATH = re.compile(r"^/(?:api|v\d)/")
LANG_KEY = re.compile(r"^[a-z]{2,3}(?:\.[A-Za-z_]+)+$")
# `{}`, `:` and `,` are allowed inside the stem because Mega Hack names assets with
# fmtlib placeholders (`player_{:02}.png`). Without them every such asset fell
# through to `other`. `.dll`/`.so` are here because host and engine names such as
# `libcocos2d.dll` are paths by the same reasoning.
FILE_PATH = re.compile(
    r"^[A-Za-z0-9_{}:,.\\/-]*\.(?:png|json|dt|bin|dat|txt|xml|zip|plist|dll|so|ttc)$"
)
# fmtlib braces, e.g. `{:02}`, `{:g}`. The old form only matched `{word}`, so
# every numeric placeholder was missed. `*` is in the flag class because OpenSSL
# prompt strings are `%*sPath Length Constraint:` — width-by-asterisk — and were
# falling through to `other` without it.
FORMAT_STR = re.compile(r"%[-+ #0-9.*]*[a-zA-Z]|\{[^}]*\}")
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
MANGLED = re.compile(r"^\??[A-Za-z0-9_]+@|^\?\?[0-9]")  # MSVC ?f@ns@@ and ??1Class
URL = re.compile(r"^https?://")
UI_ELEMENT = re.compile(
    r"^(?:CHECKBOX|COLOUR|BUTTON|LABEL|TOGGLE|SPINNER|SLIDER|TAB|ICON|PROGRESS|MENU|TEXT)"
    r"_[A-Z0-9_]+$"
)
# Statically linked OpenSSL and libc++. Matched by known prefixes plus a general
# snake_case-with-mixed-case shape, since the library's own names are not
# consistent about casing (`EVP_DigestFinalXOF`, `i2o_SCT`, `OSSL_PARAM_*`,
# `includedir`, `prime-field`).
OPENSSL_PREFIX = re.compile(
    r"^(?:OSSL_|EVP_|X509_|PKCS|DH_|DSA_|RSA_|ASN1_|EC_|BN_|BIO_|SSL_|i2[od]_|o2[i]_[A-Za-z]"
    r"|d2i_|i2d_|CRYPTO_|ENGINE_|HMAC_|CMAC_|OCSP_|TS[A-Z]|CMS_|SRP_|CTX_"
    r"|OPENSSL_|DIR_|LIST_|PKEY_|SO_PATH|EXTRACT_|EXPAND_|ssl3-ms|xoflen)"
)
SNAKE = re.compile(r"^[a-z][A-Za-z0-9]*(?:_[A-Za-z0-9]+)+$")
FLAG = re.compile(r"^(?:HACK_[A-Z0-9_]+[+]?)$")
ERROR_TEXT = re.compile(
    r"^(?:\[json\.exception|invalid|unexpected|expected|assertion failed|failed to|cannot|error:|missing |malformed |syntax error).*|"
    r".*(?:too long|subscript|presentation type)$|"
    r"^[a-z].*\s.*[.:]$",
    re.I,
)
# Lowercase diagnostic prose with a space.
LIBRARY_TEXT = re.compile(r"^[a-z][a-z0-9_ ,'()<>[\]/&.:+-]*\s[a-z0-9_ ,'()<>[\]/&.:+-]+$")

CATEGORIES = [
    ("setting-key", SETTING_KEY),
    ("api-path", API_PATH),
    ("url", URL),
    ("language-key", LANG_KEY),
    ("file-path", FILE_PATH),
    ("mangled-symbol", MANGLED),
    ("ui-element", UI_ELEMENT),
    ("host-method", HOST_METHOD),
    # Third-party requires a *known* library prefix. A generic snake_case shape
    # was tried here and removed: it swept in 974 of Mega Hack's own identifiers
    # (icons_0, override_cols, player_ball, no_bar) and put 78% of the bucket in
    # the wrong place. A bucket that is mostly wrong is worse than one that is
    # small and honest, so the generic shape gets its own category instead.
    ("third-party", OPENSSL_PREFIX),
    ("mod-identifier", SNAKE),
    ("flag", FLAG),
    ("format-string", FORMAT_STR),
    ("library-text", LIBRARY_TEXT),
    ("error-text", ERROR_TEXT),
    ("user-label", NATURAL_LABEL),
]


# Loaded lazily in main(); kept module-global so classify() can use it.
_VA_FUNCS: dict[str, list[str]] = {}
_SSO_FUNCS: dict[str, list[str]] = {}


def _base_addr(func_name: str) -> int | None:
    m = re.match(r"([0-9a-fA-F]+)_", func_name)
    if not m:
        return None
    try:
        return int(m.group(1), 16)
    except ValueError:
        return None


def classify(text: str, va: str = "") -> str:
    for name, pattern in CATEGORIES:
        if pattern.search(text):
            cat = name
            break
    else:
        cat = "other"

    # Promote `mod-identifier` / `other` to `third-party` when every referencing
    # function sits in the statically-linked library region. This is the OpenSSL
    # case the prefix list cannot enumerate: hundreds of distinct stems (aes_,
    # asn1_, bn_, ...) whose only common feature is where their code lives.
    # Prefix guessing was explicitly rejected in the note; the address region is
    # evidence instead. 0x1803A4EE0 is the first function after the 0x528a0-byte
    # gap at 0x180352640; Mega Hack's own code lives below it.
    if cat in ("mod-identifier", "other"):
        if va.startswith("0x"):
            funcs = _VA_FUNCS.get(va, [])
        elif va == "sso":
            funcs = _SSO_FUNCS.get(text, [])
        else:
            funcs = []
        if funcs and all(
            (a := _base_addr(f)) is not None and a >= LIBRARY_REGION_START
            for f in funcs
        ):
            cat = "third-party"
    return cat


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

    # Build VA -> referencing functions and text -> referencing functions (SSO)
    # from the index, so classify() can apply the library-region rule.
    global _VA_FUNCS, _SSO_FUNCS
    in_sso = False
    for line in args.index.read_text(encoding="utf-8").splitlines():
        if line.startswith("#"):
            if "stack-built" in line:
                in_sso = True
            continue
        if not line.strip():
            continue
        parts = line.split("\t")
        if len(parts) < 4:
            continue
        if in_sso or parts[0] == "sso":
            _SSO_FUNCS.setdefault(parts[3].strip().strip("'"), []).extend(
                parts[2].split(",")
            )
        else:
            _VA_FUNCS.setdefault(parts[0], []).extend(parts[2].split(","))

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
        rows.append((classify(text, va), va, text))

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
    print(f"index: {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
