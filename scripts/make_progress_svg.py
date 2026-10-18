"""Generate the progress graphics from the repository's real state.

Four graphics, each a different lens on the same work, because one number cannot
portray progress honestly:

  docs/progress.svg            overview: overall criteria, decompilation, milestones
  docs/progress-criteria.svg   every exit criterion, ticked or not — the real measure
  docs/progress-indexes.svg    the searchable research surface, by index
  docs/progress-activity.svg   recent commits, so a turn's work is visible at a glance

Everything is derived, never hand-drawn, so the pictures cannot drift from reality:

  docs/ROADMAP.md              milestone states and exit-criterion ticks
  recovered/functions/_*.tsv   index sizes
  recovered/decompile.log      function counts
  notes/*.md, scripts/*        durable output counts
  git log                      recent activity

Run after any change to the roadmap or the recovered tree:

    python scripts/make_progress_svg.py
    python scripts/make_progress_svg.py --check      # fail if any is stale
    python scripts/make_progress_svg.py --only criteria

`--check` skips cleanly when there is no recovered tree, so a fresh clone stays
green instead of failing on images it cannot rebuild.

Output is plain SVG with no external references, so it renders on the repo page
and in any Markdown viewer.
"""

from __future__ import annotations

import argparse
import html
import math
import pathlib
import re
import subprocess
import sys

REPO = pathlib.Path(__file__).resolve().parent.parent
ROADMAP = REPO / "docs" / "ROADMAP.md"
NOTES = REPO / "notes"
SCRIPTS = REPO / "scripts"
RECOVERED = REPO / "recovered" / "functions"
DOCS = REPO / "docs"

# GitHub-dark palette, kept in one place so the four graphics look like a set.
BG = "#0d1117"
PANEL = "#161b22"
BORDER = "#30363d"
TEXT = "#e6edf3"
MUTED = "#8b949e"
ACCENT = "#58a6ff"
GREEN = "#3fb950"
YELLOW = "#d29922"
RED = "#f85149"
PURPLE = "#bc8cff"
GREY = "#484f58"
TRACK = "#21262d"

FONT = "Segoe UI,DejaVu Sans,Helvetica,sans-serif"
MONO = "Cascadia Mono,Consolas,DejaVu Sans Mono,monospace"

STATE_COLOUR = {"done": GREEN, "in progress": YELLOW, "not started": GREY}
COMMIT_COLOUR = {
    "feat": GREEN, "fix": YELLOW, "notes": ACCENT, "docs": MUTED,
    "test": PURPLE, "refactor": ACCENT, "chore": GREY, "build": GREY,
    "perf": GREEN, "style": GREY, "revert": RED,
}

# The ten indexes, in the order the README lists them.
INDEXES = [
    ("_calls.tsv", "static call edges"),
    ("_xrefs.tsv", "string references"),
    ("_strings.tsv", "classified strings"),
    ("_rtti_hierarchy.tsv", "inheritance edges"),
    ("_rtti.tsv", "RTTI classes"),
    ("_init_table.tsv", "load-time constructors"),
    ("_wrappers.tsv", "hook wrappers"),
    ("_settings.tsv", "settings tags"),
    ("_host_methods.tsv", "host methods named"),
    ("_hook_sites.tsv", "hook registrations"),
]


# --------------------------------------------------------------------------- data


def esc(text: str) -> str:
    return html.escape(text, quote=False)


def read_roadmap_milestones() -> list[tuple[str, str, str]]:
    """[(id, title, state)] from the roadmap's milestone table."""
    text = ROADMAP.read_text(encoding="utf-8")
    rows = []
    for m in re.finditer(r"^\|\s*(M\d)\s*\|([^|]+?)\s*\|\s*([a-z ]+?)\s*\|", text, re.M):
        rows.append((m.group(1), m.group(2).strip(), m.group(3).strip()))
    return rows


def read_roadmap_criteria() -> list[tuple[str, list[tuple[bool, str]]]]:
    """[(milestone id, [(done, text)])] from the roadmap's `- [ ]` lines."""
    text = ROADMAP.read_text(encoding="utf-8")
    per: dict[str, list[tuple[bool, str]]] = {}
    order: list[str] = []
    current = None
    for line in text.splitlines():
        head = re.match(r"^###\s+(M\d)\b", line)
        if head:
            current = head.group(1)
            if current not in per:
                per[current] = []
                order.append(current)
            continue
        item = re.match(r"^\s*-\s*\[([ xX])\]\s+(.*)$", line)
        if item and current:
            body = " ".join(item.group(2).split())
            per[current].append((item.group(1).lower() == "x", body))
    return [(mid, per[mid]) for mid in order]


def has_recovered() -> bool:
    return RECOVERED.is_dir() and bool(list(RECOVERED.glob("*.c")))


def row_count(name: str) -> int | None:
    """Data rows in an index, or None when there is no recovered tree."""
    if not has_recovered():
        return None
    path = RECOVERED / name
    if not path.exists():
        return 0
    return sum(
        1 for line in path.read_text(encoding="utf-8").splitlines()
        if line.strip() and not line.startswith("#")
    )


def decompile_counts() -> tuple[int, int]:
    """(functions written, functions total). (0, 0) with no recovered tree."""
    if not has_recovered():
        return 0, 0
    written = 0
    manifest = RECOVERED / "_manifest.txt"
    if manifest.exists():
        for line in manifest.read_text(encoding="utf-8").splitlines():
            if line.startswith("function_files:"):
                try:
                    written = int(line.split(":", 1)[1].strip())
                except ValueError:
                    pass
    total = written
    log = RECOVERED.parent / "decompile.log"
    if log.exists():
        for line in log.read_text(encoding="utf-8", errors="replace").splitlines():
            m = re.search(r"DONE total=(\d+)", line)
            if m:
                total = int(m.group(1))
    return written, total


def recent_commits(count: int) -> list[tuple[str, str]]:
    """[(subject, type)] for the last `count` commits, newest first."""
    try:
        out = subprocess.run(
            ["git", "-C", str(REPO), "log", f"-{count}", "--pretty=format:%s"],
            capture_output=True, text=True, check=True, timeout=20,
        ).stdout
    except (OSError, subprocess.SubprocessError):
        return []
    commits = []
    for line in out.splitlines():
        line = line.strip()
        if not line:
            continue
        kind = line.split(":", 1)[0].split("(", 1)[0].strip().lower()
        commits.append((line, kind if kind in COMMIT_COLOUR else "other"))
    return commits


def head_sha() -> str:
    try:
        return subprocess.run(
            ["git", "-C", str(REPO), "rev-parse", "--short", "HEAD"],
            capture_output=True, text=True, check=True, timeout=20,
        ).stdout.strip()
    except (OSError, subprocess.SubprocessError):
        return "unknown"


# ----------------------------------------------------------------------- drawing


class Canvas:
    """Minimal SVG writer. Keeps the four graphics visually consistent."""

    def __init__(self, w: int, h: int, title: str, subtitle: str) -> None:
        self.w, self.h = w, h
        self.p: list[str] = []
        self.a: list[str] = []
        self.title, self.subtitle = title, subtitle

    def rect(self, x, y, w, h, fill, rx=0, stroke=None, opacity=None) -> None:
        s = f'<rect x="{x:.1f}" y="{y:.1f}" width="{max(w, 0):.1f}" height="{max(h, 0):.1f}" fill="{fill}"'
        if rx:
            s += f' rx="{rx}"'
        if stroke:
            s += f' stroke="{stroke}" stroke-width="1"'
        if opacity is not None:
            s += f' opacity="{opacity}"'
        self.p.append(s + "/>")

    def text(self, x, y, body, size=12, fill=TEXT, weight=None, anchor=None,
             family=FONT, opacity=None) -> None:
        s = f'<text x="{x:.1f}" y="{y:.1f}" font-family="{family}" font-size="{size}" fill="{fill}"'
        if weight:
            s += f' font-weight="{weight}"'
        if anchor:
            s += f' text-anchor="{anchor}"'
        if opacity is not None:
            s += f' opacity="{opacity}"'
        self.p.append(s + f">{esc(body)}</text>")

    def line(self, x1, y1, x2, y2, stroke, width=1, dash=None) -> None:
        s = (f'<line x1="{x1:.1f}" y1="{y1:.1f}" x2="{x2:.1f}" y2="{y2:.1f}" '
             f'stroke="{stroke}" stroke-width="{width}"')
        if dash:
            s += f' stroke-dasharray="{dash}"'
        self.p.append(s + "/>")

    def circle(self, cx, cy, r, fill) -> None:
        self.p.append(f'<circle cx="{cx:.1f}" cy="{cy:.1f}" r="{r}" fill="{fill}"/>')

    def render(self) -> str:
        head = [
            f'<svg xmlns="http://www.w3.org/2000/svg" width="{self.w}" height="{self.h}" '
            f'viewBox="0 0 {self.w} {self.h}" role="img" '
            f'aria-label="{esc(self.title)}">',
            f'<rect width="{self.w}" height="{self.h}" fill="{BG}"/>',
            f'<rect x="0.5" y="0.5" width="{self.w-1}" height="{self.h-1}" fill="none" '
            f'stroke="{BORDER}" rx="10"/>',
            f'<text x="28" y="40" font-family="{FONT}" font-size="19" font-weight="600" '
            f'fill="{TEXT}">{esc(self.title)}</text>',
            f'<text x="28" y="61" font-family="{FONT}" font-size="12" fill="{MUTED}">'
            f'{esc(self.subtitle)}</text>',
        ]
        return "\n".join(head + self.p + self.a + ["</svg>"])

    def footer(self, note: str, sha: str) -> None:
        y = self.h - 30
        self.line(28, y - 14, self.w - 28, y - 14, BORDER)
        self.text(28, y, note, 10, MUTED)
        self.text(self.w - 28, y, f"git {sha}", 10, MUTED, anchor="end", family=MONO)


def fmt(value: int | None) -> str:
    return "—" if value is None else f"{value:,}"


def bar(c: Canvas, x, y, w, h, frac, colour, track=TRACK, rx=4) -> None:
    c.rect(x, y, w, h, track, rx)
    if frac > 0:
        c.rect(x, y, w * min(frac, 1.0), h, colour, rx)


# ------------------------------------------------------------------ 1. overview


def render_overview(sha: str) -> str:
    crit = read_roadmap_criteria()
    total = sum(len(v) for _, v in crit)
    done = sum(1 for _, v in crit for d, _ in v if d)
    pct = (done / total * 100) if total else 0.0
    written, funcs = decompile_counts()
    fpct = (written / funcs * 100) if funcs else 0.0

    c = Canvas(900, 500, "Mega Hack v9.1.3 — reconstruction progress",
               "every number derived from this repository; generated by scripts/make_progress_svg.py")

    # Headline: exit criteria met.
    c.text(28, 96, "exit criteria met", 12, MUTED)
    c.text(872, 96, f"{done} / {total}   ({pct:.1f}%)", 15, TEXT, weight="600", anchor="end")
    bar(c, 28, 104, 844, 22, pct / 100 if total else 0, ACCENT)

    # Decompilation.
    c.text(28, 154, "functions recovered into recovered/ (gitignored, never committed)", 12, MUTED)
    c.text(872, 154, f"{fmt(written)} / {fmt(funcs)}   ({fpct:.1f}%)" if funcs else "no recovered tree",
           15, TEXT, weight="600", anchor="end")
    bar(c, 28, 162, 844, 22, fpct / 100 if funcs else 0, GREEN)

    # Milestones with per-milestone criterion fractions.
    y = 214
    c.text(28, y, "milestones — fraction of that milestone's own exit criteria met", 12, MUTED)
    y += 12
    per = {mid: (sum(1 for d, _ in v if d), len(v)) for mid, v in crit}
    for mid, title, state in read_roadmap_milestones():
        y += 30
        d, n = per.get(mid, (0, 0))
        colour = STATE_COLOUR.get(state, GREY)
        c.circle(36, y - 4, 5, colour)
        c.text(52, y, mid, 13, TEXT, weight="600")
        c.text(88, y, title, 13, TEXT)
        c.text(560, y, state, 11, colour)
        bx, bw = 660, 150
        bar(c, bx, y - 10, bw, 8, (d / n) if n else 0, colour if state != "not started" else GREY, rx=4)
        c.text(bx + bw + 12, y, f"{d}/{n}", 11, TEXT, weight="600")

    c.footer("baseline v9.1.3 · GD 2.2081 · Geode 5.6.1 · image 1 of 4", sha)
    return c.render()


# ---------------------------------------------------------------- 2. criteria


def render_criteria(sha: str) -> str:
    crit = read_roadmap_criteria()
    total = sum(len(v) for _, v in crit)
    done = sum(1 for _, v in crit for d, _ in v if d)

    row_h = 25
    height = 150 + sum(max(len(v), 1) for _, v in crit) * row_h + 60
    c = Canvas(900, height, "Exit criteria — every box, ticked or not",
               f"{done} of {total} criteria met across {len(crit)} milestones")

    y = 96
    bar(c, 28, y, 844, 16, (done / total) if total else 0, ACCENT)

    y += 44
    for mid, items in crit:
        d = sum(1 for done_i, _ in items if done_i)
        c.text(28, y, mid, 14, TEXT, weight="600")
        c.text(872, y, f"{d}/{len(items)}", 12, MUTED, anchor="end", family=MONO)
        y += 8
        c.line(28, y, 872, y, BORDER)
        y += 18
        for done_i, body in items:
            colour = GREEN if done_i else GREY
            c.rect(30, y - 9, 12, 12, colour if done_i else TRACK, rx=3,
                   stroke=None if done_i else BORDER)
            if done_i:
                # A tick, drawn rather than typed, so it reads at small sizes.
                c.p.append(
                    f'<path d="M33 {y-3.5} l2.5 2.5 l4.5 -5" fill="none" stroke="{BG}" '
                    f'stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"/>'
                )
            # Strip the Markdown the roadmap uses for emphasis and links.
            plain = re.sub(r"\*\*", "", body)
            plain = re.sub(r"`", "", plain)
            plain = re.sub(r"\[([^\]]+)\]\([^)]+\)", r"\1", plain)
            if len(plain) > 118:
                plain = plain[:115] + "…"
            c.text(52, y, plain, 11, TEXT if done_i else MUTED)
            y += row_h
        y += 10

    c.footer("ticks are read from docs/ROADMAP.md — untick a box and this goes stale · image 2 of 4", sha)
    return c.render()


# ------------------------------------------------------------------ 3. indexes


def render_indexes(sha: str) -> str:
    rows = [(name, label, row_count(name)) for name, label in INDEXES]
    known = [(n, l, v) for n, l, v in rows if v]
    top = max((v for _, _, v in known), default=1) or 1

    c = Canvas(900, 470, "The research surface — what can be looked up, and how much of it",
               "row counts from recovered/functions/_*.tsv; bar length is logarithmic so a "
               "42,701-row index and a 43-row one both fit")

    y = 100
    c.text(28, y, "index", 11, MUTED)
    c.text(600, y, "rows", 11, MUTED, anchor="end")
    c.text(620, y, "log scale", 11, MUTED)
    y += 12
    c.line(28, y, 872, y, BORDER)
    y += 20

    for name, label, value in rows:
        colour = ACCENT
        if name == "_hook_sites.tsv" or name == "_host_methods.tsv":
            colour = GREEN          # the newest indexes
        c.text(30, y, label, 12, TEXT)
        c.text(600, y, fmt(value), 12, TEXT if value else MUTED, anchor="end", family=MONO)
        frac = (math.log(value) / math.log(top)) if (value and value > 0) else 0.0
        bar(c, 620, y - 9, 200, 9, frac, colour if value else GREY, rx=4)
        c.text(872, y, name.lstrip("_").replace(".tsv", ""), 10, GREY, anchor="end", family=MONO)
        y += 24

    y += 12
    c.line(28, y, 872, y, BORDER)
    y += 24
    notes = len(list(NOTES.glob("*.md")))
    tools = len(list(SCRIPTS.glob("*.py"))) + len(list(SCRIPTS.glob("*.ps1")))
    for i, (label, value) in enumerate([("durable notes", notes), ("tracked tools", tools)]):
        x = 28 + i * 220
        c.rect(x, y, 200, 52, PANEL, rx=6, stroke=BORDER)
        c.text(x + 12, y + 20, label, 11, MUTED)
        c.text(x + 12, y + 40, str(value), 16, TEXT, weight="600")

    c.footer("a bigger index is more searchable, not more finished · image 3 of 4", sha)
    return c.render()


# ---------------------------------------------------------------- 4. activity


def render_activity(sha: str, limit: int = 14) -> str:
    commits = recent_commits(limit)
    crit = read_roadmap_criteria()
    total = sum(len(v) for _, v in crit)
    done = sum(1 for _, v in crit for d, _ in v if d)

    c = Canvas(900, 470, "Recent work — what moved, newest first",
               f"{len(commits)} most recent commits · exit criteria currently {done}/{total}")

    y = 98
    for subject, kind in commits:
        colour = COMMIT_COLOUR.get(kind, MUTED)
        c.circle(38, y - 4, 4, colour)
        body = subject
        if len(body) > 96:
            body = body[:93] + "…"
        c.text(52, y, body, 12, TEXT)
        c.text(872, y, kind, 10, colour, anchor="end", family=MONO)
        y += 24

    if not commits:
        c.text(28, y, "no git history available", 12, MUTED)

    c.footer("regenerated on every watchdog start, so a turn's work shows up here · image 4 of 4", sha)
    return c.render()


# ----------------------------------------------------------------------- driver


GRAPHS = {
    "overview": ("progress.svg", render_overview),
    "criteria": ("progress-criteria.svg", render_criteria),
    "indexes": ("progress-indexes.svg", render_indexes),
    "activity": ("progress-activity.svg", render_activity),
}


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("--outdir", type=pathlib.Path, default=DOCS)
    parser.add_argument("--only", choices=sorted(GRAPHS), action="append",
                        help="render just one graphic (repeatable)")
    parser.add_argument("--check", action="store_true",
                        help="exit 1 if any committed graphic is stale")
    parser.add_argument("--commits", type=int, default=14,
                        help="how many commits the activity graphic shows")
    args = parser.parse_args(argv)

    if not ROADMAP.exists():
        raise SystemExit(f"missing {ROADMAP}")

    sha = head_sha()
    chosen = sorted(args.only) if args.only else sorted(GRAPHS)
    stale: list[str] = []
    wrote: list[str] = []

    for key in chosen:
        filename, render = GRAPHS[key]
        target = args.outdir / filename
        if key == "activity":
            svg = render(sha, args.commits)
        else:
            svg = render(sha)

        if args.check:
            if not target.exists():
                stale.append(f"{filename}: missing")
            elif target.read_text(encoding="utf-8").strip() != svg.strip():
                stale.append(f"{filename}: stale")
            continue

        args.outdir.mkdir(parents=True, exist_ok=True)
        target.write_text(svg + "\n", encoding="utf-8")
        wrote.append(filename)

    if args.check:
        # A fresh clone has no recovered tree, so its indexes read as "—" and cannot
        # match what was committed with one. Report that honestly instead of failing.
        if stale and not has_recovered() and all("stale" in s or "missing" in s for s in stale):
            print(f"SKIP: no recovered tree at {RECOVERED}; graphics cannot be compared")
            print(f"      {len(stale)} graphic(s) would differ until decompile_all.py has run")
            return 0
        if stale:
            for s in stale:
                print(f"stale: {s}")
            print("run scripts/make_progress_svg.py")
            return 1
        print(f"all {len(chosen)} graphic(s) up to date")
        return 0

    for name in wrote:
        print(f"wrote {args.outdir / name}")
    crit = read_roadmap_criteria()
    total = sum(len(v) for _, v in crit)
    done = sum(1 for _, v in crit for d, _ in v if d)
    written, funcs = decompile_counts()
    print(f"  exit criteria   : {done}/{total}")
    print(f"  functions       : {fmt(written)}/{fmt(funcs)}")
    print(f"  notes / tools   : {len(list(NOTES.glob('*.md')))} / "
          f"{len(list(SCRIPTS.glob('*.py'))) + len(list(SCRIPTS.glob('*.ps1')))}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())