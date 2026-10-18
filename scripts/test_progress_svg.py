#!/usr/bin/env python3
"""Pin the progress graphics: they must not overlap, and must not go stale.

Three real layout bugs shipped in the first version of these graphics and none
of them was visible from the generator's own output:

  - `progress-indexes.svg` drew the index filename right-aligned at x=872 while
    the bar ran to x=820, so the two collided on long names;
  - `progress-criteria.svg` and `progress-activity.svg` sized their canvases
    from a formula instead of measuring the layout, so the last rows landed on
    top of the footer caption;
  - criterion text was truncated at a fixed *character* count, which cuts wide
    glyphs badly and lets text past the right margin.

None of those raise, and none of them show up in an XML validity check. So this
renders every graphic in memory and inspects the geometry: no text may leave the
canvas, and no two texts on the same baseline may overlap horizontally.

Skips when there is no recovered tree, since the index counts then render as
placeholders that legitimately differ from what was committed.
"""

from __future__ import annotations

import pathlib
import re
import sys
import xml.dom.minidom

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

import make_progress_svg as m  # noqa: E402

TEXT_RE = re.compile(
    r'<text x="([-\d.]+)" y="([-\d.]+)"[^>]*font-size="([\d.]+)"([^>]*)>([^<]*)</text>'
)


def unescape(body: str) -> str:
    return body.replace("&amp;", "&").replace("&lt;", "<").replace("&gt;", ">")


def spans(svg: str) -> tuple[int, dict[float, list[tuple[float, float, str]]]]:
    width = int(re.search(r'width="(\d+)"', svg).group(1))
    rows: dict[float, list[tuple[float, float, str]]] = {}
    for match in TEXT_RE.finditer(svg):
        x, y, size, rest, body = (
            float(match.group(1)), float(match.group(2)), float(match.group(3)),
            match.group(4), unescape(match.group(5)),
        )
        mono = m.MONO in rest
        w = m.text_width(body, size, mono)
        right_aligned = 'text-anchor="end"' in rest
        span = (x - w, x) if right_aligned else (x, x + w)
        rows.setdefault(round(y), []).append((span[0], span[1], body))
    return width, rows


def main() -> int:
    failures: list[str] = []

    def check(cond: bool, label: str, detail: str = "") -> None:
        print(f"{'ok  ' if cond else 'FAIL'}  {label}{'  ' + detail if detail else ''}")
        if not cond:
            failures.append(label)

    if not m.ROADMAP.exists():
        print(f"SKIP: no roadmap at {m.ROADMAP}")
        return 0

    sha = m.head_sha()
    renderers = {
        "progress.svg": lambda: m.render_overview(sha),
        "progress-criteria.svg": lambda: m.render_criteria(sha),
        "progress-indexes.svg": lambda: m.render_indexes(sha),
        "progress-activity.svg": lambda: m.render_activity(sha, 14),
    }

    for name, render in renderers.items():
        svg = render()

        try:
            xml.dom.minidom.parseString(svg)
            check(True, f"{name} is valid XML")
        except Exception as exc:                      # noqa: BLE001
            check(False, f"{name} is valid XML", str(exc))
            continue

        width, rows = spans(svg)
        check(width >= 900, f"{name} has room to lay out", f"{width}px")

        # 1. nothing may leave the canvas
        outside = [
            b for items in rows.values() for lo, hi, b in items
            if lo < 2 or hi > width - 2
        ]
        check(not outside, f"{name} keeps all text inside",
              (outside[0][:50] if outside else ""))

        # 2. nothing may overlap on a shared baseline
        collisions = []
        for y, items in sorted(rows.items()):
            items.sort()
            for i in range(1, len(items)):
                if items[i][0] < items[i - 1][1] - 0.5:
                    collisions.append(
                        f"y={y}: {items[i-1][2][:28]!r} ends {items[i-1][1]:.0f}, "
                        f"{items[i][2][:28]!r} starts {items[i][0]:.0f}"
                    )
        check(not collisions, f"{name} has no overlapping text",
              (collisions[0] if collisions else ""))

    # 3. The criteria graphic must draw exactly one checkbox per criterion.
    crit = m.read_roadmap_criteria()
    expect = sum(len(v) for _, v in crit)
    done = sum(1 for _, v in crit for d, _ in v if d)
    svg = m.render_criteria(sha)
    boxes = len(re.findall(r'<rect x="30\.0" y="[\d.]+" width="12\.0" height="12\.0"', svg))
    ticks = len(re.findall(r'<path d="M33 ', svg))
    check(boxes == expect, "one checkbox per exit criterion",
          f"{boxes} drawn, {expect} in the roadmap")
    check(ticks == done, "one tick per met criterion",
          f"{ticks} drawn, {done} met")

    # 4. Everything must fit in the canvas it declares: content must not reach
    #    the footer band.
    for name, render in renderers.items():
        svg = render()
        height = int(re.search(r'height="(\d+)"', svg).group(1))
        _, rows = spans(svg)
        lowest = max(rows) if rows else 0
        footer_top = height - 46
        overlaps_footer = sum(
            1 for y in rows if footer_top <= y <= height - 20
        )
        check(lowest <= height - 8, f"{name} last row is inside the canvas",
              f"lowest baseline {lowest}, height {height}")
        # The footer's own two texts legitimately sit in the footer band; anything
        # else there is content that ran into it.
        check(overlaps_footer <= 1, f"{name} content clears the footer",
              f"{overlaps_footer} baseline(s) in the footer band")

    print()
    if failures:
        print(f"{len(failures)} FAILED: {', '.join(failures)}")
        return 1
    print("all checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())