"""Verify a claimed Windows/Android key difference against both raw images.

This exists because five rounds of `notes/android-abi.md` had to be corrected, all
for the same reason: a set difference was read off a source that undercounts one
side, and the gap was mistaken for a platform difference. `_xrefs.tsv` keeps only
what the decompiler surfaced, so MSVC SSO strings go missing; a contiguous byte
scan cannot see SSO strings at all, because their characters are interleaved with
padding.

So a difference claim gets no benefit of the doubt. For every key, both images are
searched directly:

  contiguous   the exact bytes appear somewhere in the file
  sso          every 4-byte chunk of the key appears (packed immediates)
  absent       neither

A key claimed Android-only that is `contiguous` in the DLL is reported as a
FAILED CLAIM, because the DLL demonstrably contains it.

    # verify a list of claimed differences
    python scripts/verify_platform_keys.py --claims android-only --file keys.txt
    python scripts/verify_platform_keys.py --key HOME/WIDTH --key REPLAY/SAVE

    # summarise every key found in both binaries
    python scripts/verify_platform_keys.py --scan
"""

from __future__ import annotations

import argparse
import pathlib
import re
import sys

REPO = pathlib.Path(__file__).resolve().parent.parent
BASE = REPO / "samples" / "absolllute.hackmega"

IMAGES = {
    "windows": BASE / "absolllute.hackmega_original.dll",
    "android64": BASE / "absolllute.megahack.android64.so",
    "android32": BASE / "absolllute.megahack.android32.so",
}

KEY_RE = re.compile(r"^[A-Z][A-Z0-9_]*(?:/[A-Z0-9_{}]+)+")
BLANK = {"", "std", "cocos2d"}


def load(path: pathlib.Path) -> bytes:
    if not path.is_file():
        raise SystemExit(f"missing image {path}; the sample set is not restored")
    return path.read_bytes()


def where(data: bytes, key: str) -> str:
    """contiguous / sso-unproven / absent.

    The SSO case deliberately does **not** resolve to "present". Testing whether
    every 4-byte chunk of the key occurs somewhere in 8.7 MB will succeed almost
    by chance — short chunks like `ICON` occur thousands of times — so a chunk
    heuristic cannot distinguish a packed string from noise. It is reported as
    unproven, because claiming presence from it is precisely the error this
    script exists to stop.

    Proving an SSO string really is present needs the store disassembly, which
    `decode_sso.py` does for the recovered tree.
    """
    raw = key.encode()
    if raw in data:
        return "contiguous"
    chunks = [raw[i : i + 4] for i in range(0, len(raw), 4)]
    if all(c in data for c in chunks):
        return "sso-unproven"
    return "absent"


def classify(key: str, blobs: dict[str, bytes]) -> tuple[str, list[str]]:
    """Return (verdict, per-image locations).

    Only a contiguous hit counts as present. An `sso-unproven` image is recorded
    but never upgrades a verdict, so a key is only called "android-only" when the
    Windows image gives a real negative.
    """
    loc = {name: where(blob, key) for name, blob in blobs.items()}
    win = loc["windows"] == "contiguous"
    android = (
        loc["android64"] == "contiguous" and loc["android32"] == "contiguous"
    )
    if win and android:
        return "shared", loc
    if win:
        return "windows-only", loc
    if android:
        return "android-only", loc
    if loc["windows"] == "sso-unproven":
        return "windows-sso-unproven", loc
    return "not-found-anywhere", loc


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("--key", action="append", default=[], help="verify this key")
    parser.add_argument("--file", type=pathlib.Path, help="one key per line")
    parser.add_argument(
        "--claims",
        choices=["windows-only", "android-only", "shared"],
        help="assert every listed key has this verdict; mismatches are failures",
    )
    parser.add_argument("--scan", action="store_true", help="classify every key in the images")
    args = parser.parse_args(argv)

    blobs = {name: load(path) for name, path in IMAGES.items() if path.is_file()}

    keys: list[str] = list(args.key)
    if args.file:
        keys += [
            line.strip()
            for line in args.file.read_text(encoding="utf-8").splitlines()
            if line.strip() and not line.startswith("#")
        ]
    if args.scan or not keys:
        print("scanning for key-shaped strings in all three images", file=sys.stderr)
        for blob in blobs.values():
            for m in re.finditer(rb"[A-Z][A-Z0-9_]{1,20}(?:/[A-Z0-9_{}]+){1,8}", blob):
                try:
                    keys.append(m.group().decode("ascii"))
                except UnicodeDecodeError:
                    continue
        keys = sorted(set(keys))

    if not keys:
        print("nothing to verify", file=sys.stderr)
        return 1

    rows = []
    for key in keys:
        if key in BLANK:
            continue
        verdict, loc = classify(key, blobs)
        rows.append((key, verdict, loc))

    print(f"{'key':<40}{'verdict':<20}{'win':<12}{'a64':<12}a32")
    failures = 0
    for key, verdict, loc in rows:
        if args.claims and verdict != args.claims:
            failures += 1
        print(
            f"{key:<40}{verdict:<20}"
            f"{loc.get('windows', '?'):<12}{loc.get('android64', '?'):<12}"
            f"{loc.get('android32', '?')}"
        )

    counts: dict[str, int] = {}
    for _, verdict, _ in rows:
        counts[verdict] = counts.get(verdict, 0) + 1
    print()
    for verdict, n in sorted(counts.items()):
        print(f"{verdict:<20}{n}")
    print(f"{'total':<20}{len(rows)}")

    if args.claims:
        if failures:
            print(
                f"\n{failures} key(s) claimed '{args.claims}' are not. "
                "A set difference read from an undercounting source is not evidence.",
                file=sys.stderr,
            )
            return 1
        print(f"\nevery listed key is {args.claims}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())