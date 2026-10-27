"""Grade a captured trace: the reader's verdict plus the facts a comparison depends on.

The verdict comes from `mh_trace_check` rather than being re-derived here, so there is one
reader. Everything else is a summary of what the file says: capture status, target identity,
counts, milestones, and - the part that matters once a second variant exists - which mod owns
the hook on each GD function.

    python harness/tools/grade_trace.py --latest
    python harness/tools/grade_trace.py <trace.jsonl> [--no-check]
"""

from __future__ import annotations

import argparse
import json
import pathlib
import subprocess
import sys
from collections import Counter

DEFAULT_TRACE_DIR = pathlib.Path(
    r"C:\Users\Robert_\AppData\Local\GeometryDash\geode\mods\megahack.trace\traces"
)
DEFAULT_CHECKER = pathlib.Path(__file__).resolve().parent.parent / "build" / "mh_trace_check.exe"


def load(path: pathlib.Path) -> list[dict]:
    records = []
    with path.open(encoding="utf-8") as handle:
        for line in handle:
            if line.strip():
                records.append(json.loads(line))
    return records


def summarize(records: list[dict]) -> dict:
    meta = next(r for r in records if r.get("type") == "meta")
    end = next(r for r in records if r.get("type") == "end")
    events = [r for r in records if r.get("type") == "event"]

    targets: dict[tuple[str, str | None, int | None], int] = Counter()
    for event in events:
        hook = event.get("hook")
        if not hook:
            continue
        target = hook.get("target")
        if isinstance(target, dict):
            targets[(hook["key"], target.get("module"), target.get("priority"))] += 1
        else:
            targets[(hook["key"], None, None)] += 1

    target_meta = meta.get("target") or {}
    return {
        "run_id": meta.get("run_id"),
        "capture": meta.get("capture", {}).get("status"),
        "missing": meta.get("capture", {}).get("missing") or [],
        "target_role": target_meta.get("role"),
        "target_sha256": target_meta.get("sha256"),
        "scenario": (meta.get("scenario") or {}).get("id"),
        "fingerprint": meta.get("run_fingerprint"),
        "mods": [m.get("id") for m in meta.get("mods") or []],
        "counts": end.get("counts", {}),
        "end_status": end.get("status"),
        "milestones": dict(Counter(r.get("name") for r in events if r.get("kind") == "milestone")),
        "hooks": dict(targets),
        "main_thread_events": sum(
            1 for r in events if (r.get("thread") or {}).get("role") == "main"
        ),
        "events": len(events),
    }


def verdict(checker: pathlib.Path, path: pathlib.Path) -> str:
    result = subprocess.run(
        [str(checker), str(path)], capture_output=True, text=True, check=False
    )
    return result.stdout.strip() or result.stderr.strip()


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("trace", nargs="?", help="trace file to grade")
    parser.add_argument("--latest", action="store_true", help="grade the newest trace")
    parser.add_argument("--dir", default=str(DEFAULT_TRACE_DIR), help="where to look for --latest")
    parser.add_argument("--checker", default=str(DEFAULT_CHECKER))
    parser.add_argument("--no-check", action="store_true", help="skip mh_trace_check")
    args = parser.parse_args(argv)

    if args.latest:
        candidates = list(pathlib.Path(args.dir).glob("*.jsonl"))
        if not candidates:
            print(f"no traces in {args.dir}")
            return 1
        path = max(candidates, key=lambda p: p.stat().st_mtime)
    elif args.trace:
        path = pathlib.Path(args.trace)
    else:
        parser.error("give a trace path or --latest")
        return 2

    checker = pathlib.Path(args.checker)
    if not args.no_check and checker.is_file():
        print(verdict(checker, path))
    elif not checker.is_file():
        print(f"verdict: skipped, no checker at {checker}")

    summary = summarize(load(path))
    print(f"run      : {summary['run_id']}")
    print(f"capture  : {summary['capture']}"
          + (f" (missing {len(summary['missing'])})" if summary["missing"] else ""))
    sha = summary["target_sha256"]
    print(f"target   : {summary['target_role'] or 'none'} {sha[:16] + '...' if sha else ''}".rstrip())
    print(f"scenario : {summary['scenario']} | fingerprint {str(summary['fingerprint'])[:16]}")
    print(f"mods     : {', '.join(summary['mods']) or 'none'}")

    counts = summary["counts"]
    print(
        f"counts   : {counts.get('events')} events, {counts.get('attempted')} attempted, "
        f"{counts.get('dropped')} dropped, {counts.get('unclosed')} unclosed, "
        f"{counts.get('hook_enter')}/{counts.get('hook_exit')} enter/exit"
    )
    print(f"thread   : {summary['main_thread_events']}/{summary['events']} events on main")
    print(f"milestones: {summary['milestones'] or 'none'}")

    print("hooks:")
    for (key, module, priority), count in sorted(summary["hooks"].items(), key=lambda kv: (kv[0][0], str(kv[0][1]))):
        owner = f"{module} priority={priority}" if module else "no target hook observed"
        print(f"  {key:34s} {count:6d}  {owner}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
