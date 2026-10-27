"""Extract the contract's example scenario manifest into a tracked JSON file.

The manifest in notes/runtime-validation.md section "Scenario manifest" is the only worked
example of `mh-scenario/1`, and it is what `safe-mode-smoke` must mean. Extracting it
mechanically keeps the committed file verifiable against the note instead of being retyped.

The committed file has since diverged on purpose: its digests are real pins, not the note's
all-zero placeholders. So writing is opt-in - running this without `--write` only reports
whether the structure still matches, and never replaces a pinned digest on its own.

    python harness/tools/gen_scenario_manifest.py            # check only
    python harness/tools/gen_scenario_manifest.py --write    # regenerate (destroys pins)
"""

import argparse
import json
import pathlib
import re

REPO = pathlib.Path(__file__).resolve().parents[2]
NOTE = REPO / "notes" / "runtime-validation.md"
OUT = REPO / "harness" / "scenarios" / "safe-mode-smoke.json"


def drift(expected: object, actual: object, path: str = "") -> list[str]:
    """Paths where `actual` fails to carry what the note's example declares.

    The committed file is a superset on purpose - it has `runtime_baseline`, per-hook notes and
    real pins. What must not drift is the *shape the note promises*: every key the example
    names exists, with the same JSON kind. Lists are not descended into, because the example
    shows one element as an illustration and the real file can carry different counts.
    """
    if isinstance(expected, dict):
        if not isinstance(actual, dict):
            return [f"{path or '.'}: expected an object"]
        found: list[str] = []
        for key, item in expected.items():
            child = f"{path}.{key}" if path else key
            if key not in actual:
                found.append(f"{child}: missing")
            else:
                found += drift(item, actual[key], child)
        return found
    if isinstance(expected, list):
        return [] if isinstance(actual, list) else [f"{path or '.'}: expected an array"]
    expected_kind = type(expected).__name__
    if expected_kind != type(actual).__name__ and not (
            expected_kind in ("int", "float") and type(actual).__name__ in ("int", "float")):
        return [f"{path or '.'}: expected {expected_kind}, found {type(actual).__name__}"]
    return []


def extracted() -> str:
    text = NOTE.read_text(encoding="utf-8")
    marker = "## Scenario manifest"
    start = text.index(marker)
    block = re.search(r"```json\n(.*?)\n```", text[start:], re.S)
    if block is None:
        raise SystemExit("no json block after the scenario-manifest heading")

    body = block.group(1)
    if not body.lstrip().startswith("{"):
        raise SystemExit("the example should be one JSON object")
    if body.count("{") != body.count("}"):
        raise SystemExit("unbalanced braces in the extracted manifest")
    return body


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write", action="store_true",
                        help="overwrite the committed manifest (replaces the pinned digests)")
    args = parser.parse_args(argv)

    body = extracted()
    if args.write:
        OUT.parent.mkdir(parents=True, exist_ok=True)
        OUT.write_bytes(body.encode("utf-8"))
        print(f"wrote {OUT.relative_to(REPO)} ({len(body)} bytes)")
        return 0

    committed = json.loads(OUT.read_text(encoding="utf-8"))
    differences = drift(json.loads(body), committed)
    if differences:
        print(f"{OUT.relative_to(REPO)}: drifted from the note's example")
        for line in differences:
            print(f"  {line}")
        return 1
    print(f"{OUT.relative_to(REPO)}: carries the note's shape (digests are pinned; not rewritten)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(__import__("sys").argv[1:]))
