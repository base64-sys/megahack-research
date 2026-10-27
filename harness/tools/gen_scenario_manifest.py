"""Extract the contract's example scenario manifest into a tracked JSON file.

The manifest in notes/runtime-validation.md section "Scenario manifest" is the only worked
example of `mh-scenario/1`, and it is what `safe-mode-smoke` must mean. Extracting it
mechanically keeps the committed file verifiable against the note instead of being retyped.
"""

import pathlib
import re

REPO = pathlib.Path(__file__).resolve().parents[2]
NOTE = REPO / "notes" / "runtime-validation.md"
OUT = REPO / "harness" / "scenarios" / "safe-mode-smoke.json"

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

OUT.parent.mkdir(parents=True, exist_ok=True)
OUT.write_bytes(body.encode("utf-8"))
print(f"wrote {OUT.relative_to(REPO)} ({len(body)} bytes)")
