"""Generate docs/progress.svg from the repository's real state.

The graphic is generated, not hand-drawn, so it cannot drift from what is
actually done. Sources of truth:

  docs/ROADMAP.md                  milestone states and exit-criteria ticks
  recovered/functions/_manifest.*  decompilation counts
  recovered/functions/_*.tsv       index sizes
  notes/*.md, scripts/*.py         durable output counts

Run it after any change to the roadmap or the recovered tree:

    python scripts/make_progress_svg.py
    python scripts/make_progress_svg.py --check     # fail if stale

Output is a plain SVG with no external references, so it renders on the repo
page and in any Markdown viewer.
"""

from __future__ import annotations

import argparse
import pathlib
import re
import sys

REPO = pathlib.Path(__file__).resolve().parent.parent
ROADMAP = REPO / "docs" / "ROADMAP.md"
NOTES = REPO / "notes"
SCRIPTS = REPO / "scripts"
RECOVERED = REPO / "recovered" / "functions"
OUT = REPO / "docs" / "progress.svg"

BG = "#0d1117"
PANEL = "#161b22"
BORDER = "#30363d"
TEXT = "#e6edf3"
MUTED = "#8b949e"
ACCENT = "#58a6ff"
GREEN = "#3fb950"
YELLOW = "#d29922"
GREY = "#484f58"

STATE_COLOUR = {"done": GREEN, "in progress": YELLOW, "not started": GREY}


def read_roadmap() -> list[tuple[str, str]]:
    """Return [(milestone id, title, state)] from the roadmap's milestone table."""
    text = ROADMAP.read_text(encoding="utf-8")
    rows = []
    for m in re.finditer(r"^\|\s*(M\d)\s*\|([^|]+?)\s*\|\s*([a-z ]+?)\s*\|", text, re.M):
        rows.append((m.group(1), m.group(2).strip(), m.group(3).strip()))
    return rows


def manifest() -> dict:
    path = RECOVERED / "_manifest.txt"
    data = {}
    if not path.exists():
        return data
    for line in path.read_text(encoding="utf-8").splitlines():
        if ":" in line:
            k, _, v = line.partition(":")
            data[k.strip()] = v.strip()
    return data


def row_count(name: str) -> int:
    path = RECOVERED / name
    if not path.exists():
        return 0
    return sum(1 for line in path.read_text(encoding="utf-8").splitlines()
               if line and not line.startswith("#"))


def esc(text: str) -> str:
    return (text.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;"))


def build() -> tuple[str, dict]:
    milestones = read_roadmap()
    man = manifest()

    written = int(man.get("function_files", 0) or 0)
    total = written
    # decompile_all.py writes the log next to the output dir, i.e. recovered/.
    log = RECOVERED.parent / "decompile.log"
    if log.exists():
        for line in log.read_text(encoding="utf-8", errors="replace").splitlines():
            m = re.search(r"DONE total=(\d+)", line)
            if m:
                total = int(m.group(1))

    stats = {
        "milestones": milestones,
        "functions_written": written,
        "functions_total": total,
        "wrappers": row_count("_wrappers.tsv"),
        "xrefs": row_count("_xrefs.tsv"),
        "settings": row_count("_settings.tsv"),
        "notes": len([p for p in NOTES.glob("*.md")]),
        "tools": len([p for p in SCRIPTS.glob("*.py")]) + len([p for p in SCRIPTS.glob("*.ps1")]),
    }

    W, H = 900, 470
    p = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" '
         f'viewBox="0 0 {W} {H}" role="img" '
         f'aria-label="Mega Hack decompilation progress">']
    p.append(f'<rect width="{W}" height="{H}" fill="{BG}" rx="10"/>')
    p.append(f'<rect x="0.5" y="0.5" width="{W-1}" height="{H-1}" fill="none" '
             f'stroke="{BORDER}" rx="10"/>')

    p.append(f'<text x="28" y="42" font-family="Segoe UI,DejaVu Sans,sans-serif" '
             f'font-size="19" font-weight="600" fill="{TEXT}">'
             f'Mega Hack v9.1.3 — decompilation progress</text>')
    p.append(f'<text x="28" y="64" font-family="Segoe UI,DejaVu Sans,sans-serif" '
             f'font-size="12" fill="{MUTED}">'
             f'baseline verified against a pristine release package</text>')

    # Decompilation bar
    bar_x, bar_y, bar_w, bar_h = 28, 88, 600, 20
    pct = (written / total * 100) if total else 0
    p.append(f'<text x="{bar_x}" y="{bar_y - 8}" font-family="Segoe UI,sans-serif" '
             f'font-size="12" fill="{MUTED}">functions decompiled</text>')
    p.append(f'<text x="{bar_x + bar_w}" y="{bar_y - 8}" text-anchor="end" '
             f'font-family="Segoe UI,sans-serif" font-size="12" fill="{TEXT}" '
             f'font-weight="600">{written:,} / {total:,}  ({pct:.1f}%)</text>')
    p.append(f'<rect x="{bar_x}" y="{bar_y}" width="{bar_w}" height="{bar_h}" '
             f'fill="#21262d" rx="4"/>')
    fill_w = bar_w * (written / total if total else 0)
    p.append(f'<rect x="{bar_x}" y="{bar_y}" width="{fill_w:.1f}" height="{bar_h}" '
             f'fill="{ACCENT}" rx="4"/>')

    # Milestones
    y = bar_y + 42
    p.append(f'<text x="28" y="{y}" font-family="Segoe UI,sans-serif" font-size="12" '
             f'fill="{MUTED}">milestones</text>')
    y += 10
    for mid, title, state in milestones:
        colour = STATE_COLOUR.get(state, GREY)
        y += 24
        p.append(f'<circle cx="36" cy="{y - 4}" r="5" fill="{colour}"/>')
        p.append(f'<text x="52" y="{y}" font-family="Segoe UI,sans-serif" font-size="13" '
                 f'fill="{TEXT}" font-weight="600">{esc(mid)}</text>')
        p.append(f'<text x="86" y="{y}" font-family="Segoe UI,sans-serif" font-size="13" '
                 f'fill="{TEXT}">{esc(title)}</text>')
        p.append(f'<text x="628" y="{y}" text-anchor="end" font-family="Segoe UI,sans-serif" '
                 f'font-size="12" fill="{colour}">{esc(state)}</text>')

    # Index tiles
    tiles = [
        ("hook sites mapped", stats["wrappers"]),
        ("strings indexed", stats["xrefs"]),
        ("settings indexed", stats["settings"]),
        ("notes", stats["notes"]),
        ("tracked tools", stats["tools"]),
    ]
    tx, ty, tw, th, gap = 664, bar_y, 208, 40, 10
    p.append(f'<text x="{tx}" y="{ty - 8}" font-family="Segoe UI,sans-serif" font-size="12" '
             f'fill="{MUTED}">indexes</text>')
    for i, (label, value) in enumerate(tiles):
        col, row = i % 2, i // 2
        x = tx + col * (tw // 2 + 4)
        yy = ty + 8 + row * (th + gap)
        w = tw // 2 - 4
        p.append(f'<rect x="{x}" y="{yy}" width="{w}" height="{th}" fill="{PANEL}" '
                 f'stroke="{BORDER}" rx="6"/>')
        p.append(f'<text x="{x + 10}" y="{yy + 17}" font-family="Segoe UI,sans-serif" '
                 f'font-size="10" fill="{MUTED}">{esc(label)}</text>')
        p.append(f'<text x="{x + 10}" y="{yy + 32}" font-family="Segoe UI,sans-serif" '
                 f'font-size="14" font-weight="600" fill="{TEXT}">{value:,}</text>')

    y = max(y + 34, bar_y + 8 + 3 * (th + gap) + 16)
    p.append(f'<line x1="28" y1="{y}" x2="{W-28}" y2="{y}" stroke="{BORDER}"/>')
    p.append(f'<text x="28" y="{y + 22}" font-family="Segoe UI,sans-serif" font-size="11" '
             f'fill="{MUTED}">'
             f'{written:,} functions recovered locally · recovered/ is gitignored and never committed'
             f'</text>')
    p.append(f'<text x="28" y="{y + 40}" font-family="Segoe UI,sans-serif" font-size="11" '
             f'fill="{MUTED}">generated by scripts/make_progress_svg.py — do not hand-edit'
             f'</text>')
    p.append("</svg>")
    return "\n".join(p), stats


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--out", type=pathlib.Path, default=OUT)
    parser.add_argument("--check", action="store_true",
                        help="exit 1 if the committed graphic is stale")
    args = parser.parse_args(argv)

    if not ROADMAP.exists():
        raise SystemExit(f"missing {ROADMAP}")
    svg, stats = build()

    if args.check:
        if not args.out.exists():
            print(f"missing {args.out}")
            return 1
        if args.out.read_text(encoding="utf-8").strip() != svg.strip():
            print("progress.svg is stale; run scripts/make_progress_svg.py")
            return 1
        print("progress.svg is up to date")
        return 0

    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(svg + "\n", encoding="utf-8")
    print(f"wrote {args.out}")
    for key, value in stats.items():
        if key != "milestones":
            print(f"  {key}: {value}")
    for mid, title, state in stats["milestones"]:
        print(f"  {mid} {title}: {state}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())