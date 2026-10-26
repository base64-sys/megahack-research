#!/usr/bin/env python3
"""Tests for check_doc_links.py: anchors, missing targets, skipped dirs.

Builds a throwaway Markdown tree, so no sample set or repository state is needed.
"""

from __future__ import annotations

import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from check_doc_links import check, slug  # noqa: E402

GOOD = """# Top

## M1 - A heading (done)

See [other](other.md#duplicate-1) and [self](#m1---a-heading-done).
"""

OTHER = """# Other

## Duplicate

## Duplicate

Same-file [anchor](#duplicate-1) and [top](#other).
"""


def build(root: Path) -> None:
    (root / "a.md").write_text(GOOD, encoding="utf-8")
    (root / "other.md").write_text(OTHER, encoding="utf-8")
    nested = root / "notes"
    nested.mkdir()
    (nested / "b.md").write_text("[up](../a.md#top)\n", encoding="utf-8")
    skipped = root / "samples"
    skipped.mkdir()
    (skipped / "c.md").write_text("[broken](nowhere.md)\n", encoding="utf-8")


def test_slug() -> None:
    assert slug("M1 - A heading (done)") == "m1---a-heading-done", slug("M1 - A heading (done)")
    assert slug("Tag 438 and the coin post-pass") == "tag-438-and-the-coin-post-pass"
    assert slug("Player 1 on Top") == "player-1-on-top"


def test_resolves_and_skips() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        build(root)
        problems = check(root)
        assert problems == [], problems


def test_reports_missing_target_and_anchor() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)
        build(root)
        (root / "bad.md").write_text(
            "[gone](nowhere.md)\n[x](other.md#no-such-heading)\n[y](#nope)\n",
            encoding="utf-8",
        )
        problems = check(root)
        assert len(problems) == 3, problems
        assert any("missing target" in p for p in problems), problems
        assert sum("missing anchor" in p for p in problems) == 2, problems


if __name__ == "__main__":
    test_slug()
    test_resolves_and_skips()
    test_reports_missing_target_and_anchor()
    print("all check_doc_links tests passed")
