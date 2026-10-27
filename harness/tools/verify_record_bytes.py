"""Check that a captured trace still matches the record builders' fixture.

Why this exists: `harness/tests/test_records.cpp` proves the builders reproduce the committed
fixture byte-for-byte, and `harness/observer/src/main.cpp` builds no record of its own (this
script fails if it tries). Together those two facts are the claim "the observer emits what the
fixture says" - neither is worth much alone.

    python harness/tools/verify_record_bytes.py <trace.jsonl> [--fixture PATH]

Two fields are normalised on both sides before comparing, because they are per-run:
`hook.va` (the runtime base moves with ASLR) and `call.this.value` (an object address). Every
other byte, including key order, must be identical.

Exit codes: 0 identical, 1 a difference or a hand-built record, 2 usage/IO.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

FIXTURE = Path(__file__).resolve().parent.parent / "tests" / "fixtures" / "records_captured.jsonl"
OBSERVER = Path(__file__).resolve().parent.parent / "observer" / "src" / "main.cpp"

_VA = re.compile(r'"va":"0x[0-9A-Fa-f]+"')
_PTR = re.compile(r'"this":\{"state":"observed","type":"ptr","value":"0x[0-9A-Fa-f]+"\}')


def normalise(text: str) -> str:
    """Blank the per-run values, leave every other byte alone."""
    return _PTR.sub('"this":{"state":"observed","type":"ptr","value":"0x0"}',
                    _VA.sub('"va":"0x0"', text))


def payload_of(line: str) -> str | None:
    """The event payload: from `"kind"` up to the writer's own `capture` member."""
    start = line.find(',"kind":')
    end = line.rfind(',"capture":')
    if start < 0 or end < 0 or end < start:
        return None
    return "{" + line[start + 1:end] + "}"


def identity(payload: str) -> tuple[str, str] | None:
    record = json.loads(payload)
    kind = record.get("kind")
    if kind == "hook":
        return (kind, f"{record['hook']['key']}|{record['phase']}")
    if kind == "milestone":
        return (kind, record["name"])
    return (kind, record.get("code", ""))


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("trace")
    parser.add_argument("--fixture", default=str(FIXTURE))
    parser.add_argument("--observer", default=str(OBSERVER))
    args = parser.parse_args(argv)

    source = Path(args.observer).read_text(encoding="utf-8")
    # A record built by hand in the mod is the failure this whole split exists to prevent.
    if '"kind"' in source:
        print("differs: the observer builds a record itself instead of calling mh::records")
        return 1

    expected: dict[tuple[str, str], str] = {}
    for line in Path(args.fixture).read_text(encoding="utf-8").splitlines():
        if line.strip():
            expected[identity(line)] = normalise(line)

    seen: dict[tuple[str, str], str] = {}
    for line in Path(args.trace).read_text(encoding="utf-8").splitlines():
        if not line.startswith('{"schema"'):
            continue
        payload = payload_of(line)
        if payload is None:
            continue
        key = identity(payload)
        if key in expected and key not in seen:
            seen[key] = normalise(payload)

    missing = [name for name in expected if name not in seen]
    if missing:
        print(f"differs: the trace has no record for {missing}")
        return 1

    for key, want in expected.items():
        got = seen[key]
        if got != want:
            print(f"differs: {key[0]} {key[1]}")
            print(f"  fixture: {want}")
            print(f"  trace:   {got}")
            return 1

    print(f"identical: {len(expected)} record shape(s) rebuilt from {Path(args.trace).name}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
