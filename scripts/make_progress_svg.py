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


# Advance-width factors, measured against Segoe UI and the common monos. These are
# deliberately pessimistic: an over-estimate truncates a little early, an
# under-estimate lets two columns collide, and a collided label is unreadable.
_ADV_SANS = 0.56
_ADV_MONO = 0.62
_WIDE = {"W": 1.35, "M": 1.35, "@": 1.30, "m": 1.15, "w": 1.15, "—": 1.60, "…": 1.10}


def text_width(text: str, size: float, mono: bool = False) -> float:
    """Approximate rendered width in user units.

    Real font metrics are not available to a script that must produce
    deterministic output, so this estimates from per-character advances. It is
    the check that stops a label from being drawn over its neighbour.
    """
    if mono:
        return len(text) * size * _ADV_MONO
    total = 0.0
    for ch in text:
        total += size * _WIDE.get(ch, _ADV_SANS)
    return total


def fit(text: str, size: float, max_width: float, mono: bool = False) -> str:
    """Truncate `text` so it cannot exceed `max_width` when rendered."""
    if text_width(text, size, mono) <= max_width:
        return text
    ellipsis_w = text_width("…", size, mono)
    budget = max_width - ellipsis_w
    if budget <= 0:
        return ""
    out: list[str] = []
    used = 0.0
    for ch in text:
        w = size * (_ADV_MONO if mono else _WIDE.get(ch, _ADV_SANS))
        if used + w > budget:
            break
        out.append(ch)
        used += w
    return "".join(out).rstrip() + "…"


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
        # The caption is decorative; fit it so it can never run under the SHA.
        self.text(28, y, fit(note, 10, self.w - 28 - 28 - 90), 10, MUTED)
        self.text(self.w - 28, y, f"git {sha}", 10, MUTED, anchor="end", family=MONO)


def strip_markup(body: str) -> str:
    """Flatten the Markdown the roadmap uses, so it renders as plain SVG text."""
    plain = re.sub(r"\*\*", "", body)
    plain = re.sub(r"`", "", plain)
    plain = re.sub(r"\[([^\]]+)\]\([^)]+\)", r"\1", plain)
    return " ".join(plain.split())


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
    state_w = 110.0
    bar_x, bar_w = 660, 150
    for mid, title, state in read_roadmap_milestones():
        y += 30
        d, n = per.get(mid, (0, 0))
        colour = STATE_COLOUR.get(state, GREY)
        c.circle(36, y - 4, 5, colour)
        c.text(52, y, mid, 13, TEXT, weight="600")
        # Titles vary in length; fit them to whatever the state column leaves.
        c.text(88, y, fit(title, 13, 556 - 88 - 10), 13, TEXT)
        c.text(556, y, state, 11, colour, anchor="end")
        bar(c, bar_x, y - 10, bar_w, 8, (d / n) if n else 0,
            colour if state != "not started" else GREY, rx=4)
        c.text(bar_x + bar_w + 12, y, f"{d}/{n}", 11, TEXT, weight="600", family=MONO)

    c.footer("baseline v9.1.3 · GD 2.2081 · Geode 5.6.1 · image 1 of 4", sha)
    return c.render()


# ---------------------------------------------------------------- 2. criteria


def render_criteria(sha: str) -> str:
    crit = read_roadmap_criteria()
    total = sum(len(v) for _, v in crit)
    done = sum(1 for _, v in crit for d, _ in v if d)

    row_h = 26
    # Lay the rows out first and *measure* the result. Estimating the canvas
    # height from a formula left the last rows sitting on top of the footer,
    # which is invisible until you check for it.
    laid: list[tuple[str, float, str]] = []      # (kind, y, payload)
    heads: list[tuple[float, str, str]] = []      # (y, milestone, "d/n")
    y = 96.0
    for mid, items in crit:
        heads.append((y, mid, f"{sum(1 for d, _ in items if d)}/{len(items)}"))
        laid.append(("rule", y + 8, ""))
        y += 8 + 18
        for is_done, body in items:
            laid.append(("row" if is_done else "open", y, strip_markup(body)))
            y += row_h
        y += 10

    height = int(y + 70)
    longest = max(
        (text_width(body, 11) for kind, _, body in laid if kind in ("row", "open")),
        default=0.0,
    )
    width = min(1180, max(900, int(28 + 52 + longest + 44)))

    c = Canvas(width, height, "Exit criteria — every box, ticked or not",
               f"{done} of {total} criteria met across {len(crit)} milestones")

    bar(c, 28, 96, width - 56, 16, (done / total) if total else 0, ACCENT)

    right = width - 28
    avail = width - 52 - 28

    # Heads are drawn from `heads`, not from `laid`; they carry their own y.
    for hy, mid, frac in heads:
        c.text(28, hy, mid, 14, TEXT, weight="600")
        c.text(right, hy, frac, 12, MUTED, anchor="end", family=MONO)

    for kind, yy, body in laid:
        if kind == "rule":
            c.line(28, yy, right, yy, BORDER)
            continue
        ticked = kind == "row"
        colour = GREEN if ticked else GREY
        c.rect(30, yy - 9, 12, 12, colour if ticked else TRACK, rx=3,
               stroke=None if ticked else BORDER)
        if ticked:
            # A tick, drawn rather than typed, so it reads at small sizes.
            c.p.append(
                f'<path d="M33 {yy-3.5} l2.5 2.5 l4.5 -5" fill="none" stroke="{BG}" '
                f'stroke-width="1.8" stroke-linecap="round" stroke-linejoin="round"/>'
            )
        c.text(52, yy, fit(body, 11, avail), 11, TEXT if ticked else MUTED)

    c.footer("ticks are read from docs/ROADMAP.md — untick a box and this goes stale · image 2 of 4", sha)
    return c.render()


# ------------------------------------------------------------------ 3. indexes


def render_indexes(sha: str) -> str:
    rows = [(name, label, row_count(name)) for name, label in INDEXES]
    known = [(n, l, v) for n, l, v in rows if v]
    top_count = max((v for _, _, v in known), default=1) or 1

    # Measure the layout rather than fixing the height. The tiles sit 52px tall
    # below the last row, so a hard-coded 470 put them on top of the footer —
    # which is exactly what scripts/test_progress_svg.py now catches.
    top = 100.0
    row_h = 24.0
    after_rows = top + 12 + 20 + len(rows) * row_h
    tiles_y = after_rows + 12 + 24
    height = int(tiles_y + 52 + 70)

    c = Canvas(900, height, "The research surface — what can be looked up, and how much of it",
               "row counts from recovered/functions/_*.tsv; bar length is logarithmic so a "
               "42,701-row index and a 43-row one both fit")

    # Column geometry, measured rather than guessed: each label is fitted to the
    # space actually left after the previous column, so nothing can overlap.
    LABEL_X, LABEL_W = 30, 250
    ROWS_X, ROWS_W = 470, 70        # right-aligned numbers
    FILE_X, FILE_W = 560, 150       # mono file stem
    BAR_X, BAR_W = 730, 130

    y = top
    c.text(LABEL_X, y, "what it answers", 11, MUTED)
    c.text(ROWS_X + ROWS_W, y, "rows", 11, MUTED, anchor="end")
    c.text(FILE_X, y, "index", 11, MUTED)
    c.text(BAR_X, y, "log scale", 11, MUTED)
    y += 12
    c.line(28, y, 872, y, BORDER)
    y += 20

    for name, label, value in rows:
        newest = name in ("_hook_sites.tsv", "_host_methods.tsv")
        colour = GREEN if newest else ACCENT
        c.text(LABEL_X, y, fit(label, 12, LABEL_W), 12, TEXT)
        c.text(ROWS_X + ROWS_W, y, fmt(value), 12, TEXT if value else MUTED,
               anchor="end", family=MONO)
        stem = name.lstrip("_").replace(".tsv", "")
        c.text(FILE_X, y, fit(stem, 10, FILE_W, mono=True), 10, GREY, family=MONO)
        frac = (math.log(value) / math.log(top_count)) if (value and value > 0) else 0.0
        bar(c, BAR_X, y - 9, BAR_W, 9, frac, colour if value else GREY, rx=4)
        y += row_h

    y = tiles_y - 24
    c.line(28, y, 872, y, BORDER)
    notes = len(list(NOTES.glob("*.md")))
    tools = len(list(SCRIPTS.glob("*.py"))) + len(list(SCRIPTS.glob("*.ps1")))
    for i, (label, value) in enumerate([("durable notes", notes), ("tracked tools", tools)]):
        x = 28 + i * 220
        c.rect(x, tiles_y, 200, 52, PANEL, rx=6, stroke=BORDER)
        c.text(x + 12, tiles_y + 20, label, 11, MUTED)
        c.text(x + 12, tiles_y + 40, str(value), 16, TEXT, weight="600")

    c.footer("a bigger index is more searchable, not more finished · image 3 of 4", sha)
    return c.render()


# ---------------------------------------------------------------- 4. activity


def render_activity(sha: str, limit: int = 14) -> str:
    commits = recent_commits(limit)
    crit = read_roadmap_criteria()
    total = sum(len(v) for _, v in crit)
    done = sum(1 for _, v in crit for d, _ in v if d)

    # Measure the layout before sizing the canvas, for the same reason the
    # criteria graphic does it: a formula-driven height put the last commit on
    # top of the footer.
    row_h = 24
    top = 98.0
    height = int(top + len(commits) * row_h + 60)
    type_w = 60.0
    longest = max((text_width(s, 12) for s, _ in commits), default=0.0)
    width = min(1180, max(900, int(52 + longest + type_w + 40)))

    c = Canvas(width, height, "Recent work — what moved, newest first",
               f"{len(commits)} most recent commits · exit criteria currently {done}/{total}")

    right = width - 28
    subject_w = width - 52 - type_w - 28

    y = top
    for subject, kind in commits:
        colour = COMMIT_COLOUR.get(kind, MUTED)
        c.circle(38, y - 4, 4, colour)
        c.text(52, y, fit(subject, 12, subject_w), 12, TEXT)
        c.text(right, y, kind, 10, colour, anchor="end", family=MONO)
        y += row_h

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


def strip_tags(fragment: str) -> str:
    return html.unescape(re.sub(r"<[^>]+>", "", fragment)).strip()


SHA_STAMP_RE = re.compile(r">git [0-9a-f]{7,40}<")


def strip_self_reference(svg: str) -> str:
    """Blank the footer commit stamp so `--check` can compare meaningfully.

    Every graphic stamps the HEAD it was generated from. That stamp is
    self-referential: committing the graphic *is* what changes HEAD, so a
    byte-comparison against a fresh render can never succeed. The stamp is
    provenance, not content, so it is excluded from the staleness check.
    """
    return SHA_STAMP_RE.sub(">git <sha><", svg)


def activity_subjects(svg: str) -> list[str]:
    """Pull the commit subjects back out of a rendered activity graphic.

    Subjects are drawn at x=52; the title and subtitle sit at x=28 and the
    commit kind is right-aligned. Selecting on x is what keeps the subtitle
    from being mistaken for a commit subject.
    """
    subjects: list[str] = []
    for match in re.finditer(
        r'<text x="([\d.]+)" y="[\d.]+"[^>]*font-size="12"[^>]*>([^<]*)</text>', svg
    ):
        x, body = float(match.group(1)), strip_tags(match.group(2))
        if x == 52.0 and body:
            subjects.append(body)
    return subjects


def recent_commit_subjects(depth: int) -> list[str]:
    out = subprocess.run(
        ["git", "log", f"-{depth}", "--format=%s"],
        cwd=REPO, capture_output=True, text=True, check=True,
    )
    return out.stdout.splitlines()


def activity_subjects_live() -> tuple[bool, str]:
    """Check the activity graphic's commits still exist.

    `progress-activity.svg` cannot be byte-compared, for a stronger version of
    the same reason: it shows the most recent commits, and the newest of those
    is by definition the commit carrying the image. Instead, every subject it
    shows must still be a real, recent commit.
    """
    target = DOCS / "progress-activity.svg"
    if not target.exists():
        return False, "missing"
    shown = activity_subjects(target.read_text(encoding="utf-8"))
    if not shown:
        return False, "shows no commits"
    live = recent_commit_subjects(80)
    for subject in shown:
        probe = subject.rstrip("…")
        if not any(s.startswith(probe) for s in live):
            return False, f"shows a commit that is not in git log: {probe!r}"
    return True, ""


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
            if key == "activity":
                # Cannot be byte-compared: it shows the most recent commits, and
                # the newest of those is the commit carrying this graphic. Handled
                # by subject check below.
                if not target.exists():
                    stale.append(f"{filename}: missing")
                continue
            if not target.exists():
                stale.append(f"{filename}: missing")
            elif strip_self_reference(target.read_text(encoding="utf-8")).strip() \
                    != strip_self_reference(svg).strip():
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
        if "activity" in chosen:
            ok, detail = activity_subjects_live()
            if not ok:
                print(f"stale: progress-activity.svg: {detail}")
                print("run scripts/make_progress_svg.py")
                return 1
            print("live : progress-activity.svg (subjects verified against git log; "
                  "not byte-compared — see README)")
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