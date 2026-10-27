#!/usr/bin/env python3
"""Check that relative links and ``#anchors`` in tracked Markdown resolve.

The notes cross-reference each other heavily: ``notes/feature-inventory.md`` alone
links into a dozen subsystem notes, usually at a section anchor. A renamed heading
silently turns those links into dead ends, and a link written by hand can point at
a section that never existed. This tool walks the Markdown tree, resolves every
relative link, and recomputes each target heading's GitHub anchor.

Slug rule (approximated, not the GitHub implementation): lowercase, drop every
character that is not alphanumeric, space, hyphen or underscore, then turn spaces
into hyphens; repeated headings get ``-1``, ``-2`` suffixes in document order.
This matches every anchor used in this repository; a link using raw HTML ids or
GitHub-only extensions could still be reported as missing, so the output names the
file and link rather than asserting the target is wrong.

External links (``http:``/``https:``/``mailto:``) are not fetched -- this is a
structural check, not a link-rot crawl.

Usage (repository root; needs no sample set)::

    python3 scripts/check_doc_links.py [directory]

Exit code 1 when a relative target file or anchor is missing.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

LINK_RE = re.compile(r"\[[^\]]*\]\(([^)\s]+)\)")
HEADING_RE = re.compile(r"^(#{1,6})\s+(.*?)\s*$", re.M)
SKIP_DIRS = {".git", "node_modules", "samples", "recovered", ".oc", ".idea", ".pytest_cache",
             "build", "_deps"}


def slug(text: str) -> str:
    """GitHub-style anchor for one heading line."""
    lowered = text.strip().lower()
    kept = "".join(c for c in lowered if c.isalnum() or c in "-_ ")
    return kept.replace(" ", "-")


def anchors(path: Path) -> set[str]:
    """Every anchor a Markdown file defines, including duplicate suffixes."""
    text = path.read_text(encoding="utf-8", errors="replace")
    seen: dict[str, int] = {}
    found: set[str] = set()
    for match in HEADING_RE.finditer(text):
        base = slug(match.group(2))
        count = seen.get(base, 0)
        found.add(base if count == 0 else f"{base}-{count}")
        seen[base] = count + 1
    return found


def markdown_files(root: Path) -> list[Path]:
    files = []
    for path in root.rglob("*.md"):
        if any(part in SKIP_DIRS for part in path.relative_to(root).parts):
            continue
        files.append(path)
    return sorted(files)


def check(root: Path) -> list[str]:
    problems: list[str] = []
    cache: dict[Path, set[str]] = {}
    for path in markdown_files(root):
        text = path.read_text(encoding="utf-8", errors="replace")
        for raw in LINK_RE.findall(text):
            if raw.startswith(("http://", "https://", "mailto:", "#!")):
                continue
            target, _, anchor = raw.partition("#")
            if target.startswith("<") or target.endswith(">"):
                continue
            dest = (path.parent / target).resolve() if target else path
            if not dest.exists():
                problems.append(f"{path.relative_to(root)}: missing target {raw}")
                continue
            if not anchor or dest.suffix != ".md":
                continue
            if dest not in cache:
                cache[dest] = anchors(dest)
            if anchor not in cache[dest]:
                problems.append(f"{path.relative_to(root)}: missing anchor {raw}")
    return problems


def main(argv: list[str]) -> int:
    root = Path(argv[1]).resolve() if len(argv) > 1 else Path(".").resolve()
    if not root.is_dir():
        print(f"not a directory: {root}", file=sys.stderr)
        return 2
    problems = check(root)
    for problem in problems:
        print(problem)
    if problems:
        print(f"{len(problems)} unresolved relative link(s)")
        return 1
    print(f"all relative links and anchors resolve under {root}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
