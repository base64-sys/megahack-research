"""Rehearse the whole four-run comparison offline, before anyone spends four real runs.

The unit tests pin the gate's parts. This test pins the *integration*: two runs per variant, the
real scenario manifest read from disk, the real CLI, and the status a person will read. It builds
synthetic traces from `tests/fixtures/example_trace.jsonl` (already a known-valid trace) and
rewrites the fields a run would differ in - target identity, harness and scenario digests, and
the owning module in `hook.target`.

Three cases, all with temporary files:

* the committed manifest still blocks on its level placeholder, and prints no claim;
* a manifest whose `level.sha256` is filled produces a `pass` and a `claim: surface` line;
* one reconstruction run that is missing a required hook fails, so the pass above is not vacuous.

    python harness/tools/test_compare_rehearsal.py

Skipped when `harness/build/mh_trace_compare.exe` has not been built; the binary is the subject.
"""

from __future__ import annotations

import copy
import hashlib
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
COMPARE = HERE.parent / "build" / "mh_trace_compare.exe"
MANIFEST = HERE.parent / "scenarios" / "safe-mode-smoke.json"
TEMPLATE = HERE.parent / "tests" / "fixtures" / "example_trace.jsonl"

# The four hook keys the pilot marks required, in the order the observer installs them.
REQUIRED_KEYS = ("0x3b8eb0", "0x3b4ba0", "0x397f40", "0x3981d0")

HEX64 = "a" * 64


def template() -> tuple[dict, dict]:
    lines = [json.loads(line) for line in TEMPLATE.read_text(encoding="utf-8").splitlines()]
    return lines[0], lines[-1]


def build_trace(*, run_id: str, role: str, artifact: str, target_sha256: str, module: str,
                scenario_sha256: str, harness_sha256: str, level_sha256: str,
                fingerprint: str, omit_keys: tuple[str, ...] = ()) -> str:
    meta, end = template()
    meta = copy.deepcopy(meta)
    meta.update(run_id=run_id, run_fingerprint=fingerprint, seq=0, t_ns=0)
    meta["target"].update(role=role, artifact=artifact, sha256=target_sha256)
    meta["scenario"]["sha256"] = scenario_sha256
    meta["environment"]["harness"]["sha256"] = harness_sha256
    meta["environment"]["gd"]["sha256"] = HEX64
    meta["environment"]["geode"]["sha256"] = HEX64
    meta["settings"]["snapshot_sha256"] = HEX64
    meta["level"]["sha256"] = level_sha256
    meta["capture"] = {"status": "complete"}

    events: list[dict] = []
    sequence = 0

    def event(kind: str, **payload: object) -> None:
        nonlocal sequence
        sequence += 1
        record = {
            "schema": "mh-trace/1", "type": "event", "run_id": run_id,
            "run_fingerprint": fingerprint, "seq": sequence, "t_ns": sequence * 10,
            "frame": sequence, "thread": {"id": "main", "role": "main"},
            "kind": kind, "capture": {"status": "complete"},
        }
        record.update(payload)
        events.append(record)

    event("milestone", name="level_started",
          data={"state": "redacted", "reason": "level name is player data"})
    event("milestone", name="attempt_started", data={"state": "null"})
    call = 0
    for key in REQUIRED_KEYS:
        if key in omit_keys:
            continue
        call += 1
        for phase in ("enter", "exit"):
            event(
                "hook",
                phase=phase,
                hook={
                    "observer": "gd_method", "key": "GeometryDash.exe:rva:" + key,
                    "module": "GeometryDash.exe", "method": "method", "resolution": "rva",
                    "rva": "0x" + key.upper(), "va": "0x7FF600000000", "target_name": None,
                    "target": {"module": module, "priority": None}, "convention": "unknown",
                },
                call={
                    "call_id": f"c{call}", "depth": 1, "parent_call_id": None,
                    "this": {"state": "observed", "type": "ptr", "value": "0x0"},
                    "args": {"state": "observed", "items": []},
                    "return": {"state": "null"},
                },
            )

    trailer = copy.deepcopy(end)
    trailer.update(run_id=run_id, run_fingerprint=fingerprint, seq=sequence + 1,
                   t_ns=sequence * 10 + 10)
    trailer["counts"] = {
        "events": len(events), "attempted": len(events), "dropped": 0, "unclosed": 0,
        "hook_enter": sum(1 for e in events if e.get("phase") == "enter"),
        "hook_exit": sum(1 for e in events if e.get("phase") == "exit"),
        "errors": 0,
    }
    trailer["capture"] = {"status": "complete"}

    records = [meta, *events, trailer]
    return "\n".join(json.dumps(record, separators=(",", ":")) for record in records) + "\n"


class RehearsalTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        if not COMPARE.is_file():
            raise unittest.SkipTest(f"{COMPARE} is not built")

    def setUp(self) -> None:
        self._temp = tempfile.TemporaryDirectory()
        self.addCleanup(self._temp.cleanup)
        self.directory = Path(self._temp.name)
        self.manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
        self.release_sha = self.manifest["targets"]["release"]["sha256"]
        self.reconstruction_sha = self.manifest["targets"]["reconstruction"]["sha256"]
        self.manifest_sha = hashlib.sha256(MANIFEST.read_bytes()).hexdigest()

    def write_variant(self, manifest: Path, manifest_sha: str, level_sha: str,
                      *, omit_in_last_reconstruction: tuple[str, ...] = (),
                      omit_in_every_reconstruction: tuple[str, ...] = ()) -> list[Path]:
        """Two runs per variant, against `manifest`, with that manifest's digests in the meta."""
        harness_sha = json.loads(manifest.read_text(encoding="utf-8"))["harness"]["sha256"]
        paths: list[Path] = []
        for index, (role, artifact, target_sha, module) in enumerate([
            ("release", "absolllute.hackmega_original.dll", self.release_sha,
             "absolllute.hackmega.dll"),
            ("release", "absolllute.hackmega_original.dll", self.release_sha,
             "absolllute.hackmega.dll"),
            ("reconstruction", "megahack.reconstruction.dll", self.reconstruction_sha,
             "megahack.reconstruction"),
            ("reconstruction", "megahack.reconstruction.dll", self.reconstruction_sha,
             "megahack.reconstruction"),
        ]):
            fingerprint = hashlib.sha256(f"{role}|{manifest_sha}|{harness_sha}".encode()).hexdigest()
            omit = omit_in_last_reconstruction if index == 3 else ()
            if role == "reconstruction":
                omit = tuple(set(omit) | set(omit_in_every_reconstruction))
            text = build_trace(
                run_id=f"00000000-0000-4000-8000-00000000001{index}", role=role,
                artifact=artifact, target_sha256=target_sha, module=module,
                scenario_sha256=manifest_sha, harness_sha256=harness_sha,
                level_sha256=level_sha, fingerprint=fingerprint, omit_keys=omit,
            )
            path = self.directory / f"{role}-{index}.jsonl"
            path.write_text(text, encoding="utf-8")
            paths.append(path)
        return paths

    def compare(self, manifest: Path, paths: list[Path]) -> subprocess.CompletedProcess[str]:
        command = [str(COMPARE), str(manifest)]
        for path in paths[:2]:
            command += ["--release", str(path)]
        for path in paths[2:]:
            command += ["--reconstruction", str(path)]
        return subprocess.run(command, capture_output=True, text=True)

    def test_the_committed_manifest_blocks_on_its_level_placeholder(self) -> None:
        paths = self.write_variant(MANIFEST, self.manifest_sha, "0" * 64)
        result = self.compare(MANIFEST, paths)
        self.assertEqual(result.returncode, 1)
        self.assertIn("status: blocked | gate: blocked", result.stdout)
        self.assertIn("blocked level:", result.stdout)
        self.assertIn("placeholder", result.stdout)
        self.assertNotIn("claim:", result.stdout)

    def test_a_filled_level_digest_produces_a_surface_pass(self) -> None:
        level_sha = hashlib.sha256(b"rehearsal fixture").hexdigest()
        filled = copy.deepcopy(self.manifest)
        filled["level"]["sha256"] = level_sha
        manifest = self.directory / "manifest-filled.json"
        manifest.write_text(json.dumps(filled, indent=2), encoding="utf-8")
        manifest_sha = hashlib.sha256(manifest.read_bytes()).hexdigest()

        paths = self.write_variant(manifest, manifest_sha, level_sha)
        result = self.compare(manifest, paths)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("status: pass", result.stdout)
        self.assertIn("claim: surface", result.stdout)
        self.assertIn("lifecycle surface only", result.stdout)

    def test_one_run_missing_a_required_hook_is_unstable(self) -> None:
        # One run of a pair disagreeing is "the two runs are not comparable", not a named gate
        # finding - the comparator says so but not yet which key differs. Pinned as behaviour,
        # with the diagnostic gap recorded in notes/runtime-validation.md.
        level_sha = hashlib.sha256(b"rehearsal fixture").hexdigest()
        filled = copy.deepcopy(self.manifest)
        filled["level"]["sha256"] = level_sha
        manifest = self.directory / "manifest-filled.json"
        manifest.write_text(json.dumps(filled, indent=2), encoding="utf-8")
        manifest_sha = hashlib.sha256(manifest.read_bytes()).hexdigest()

        paths = self.write_variant(manifest, manifest_sha, level_sha,
                                   omit_in_last_reconstruction=("0x397f40",))
        result = self.compare(manifest, paths)
        self.assertEqual(result.returncode, 1)
        self.assertIn("status: unstable", result.stdout)
        self.assertIn("unstable: yes", result.stdout)
        self.assertNotIn("status: pass", result.stdout)
        # The verdict alone leaves the operator guessing; the diagnostic names the key and run.
        self.assertIn("unstable reconstruction variant:", result.stdout)
        self.assertIn("0x397f40", result.stdout)
        self.assertIn("present in run 1, absent here", result.stdout)

    def test_a_hook_missing_from_a_whole_variant_is_a_named_failure(self) -> None:
        level_sha = hashlib.sha256(b"rehearsal fixture").hexdigest()
        filled = copy.deepcopy(self.manifest)
        filled["level"]["sha256"] = level_sha
        manifest = self.directory / "manifest-filled.json"
        manifest.write_text(json.dumps(filled, indent=2), encoding="utf-8")
        manifest_sha = hashlib.sha256(manifest.read_bytes()).hexdigest()

        paths = self.write_variant(manifest, manifest_sha, level_sha,
                                   omit_in_every_reconstruction=("0x397f40",))
        result = self.compare(manifest, paths)
        self.assertEqual(result.returncode, 1)
        self.assertIn("status: fail", result.stdout)
        self.assertIn("fail min_count", result.stdout)
        self.assertIn("0x397f40", result.stdout)
        self.assertNotIn("claim:", result.stdout)

    def test_a_scenario_digest_that_does_not_match_the_manifest_is_invalid(self) -> None:
        paths = self.write_variant(MANIFEST, "b" * 64, "0" * 64)
        result = self.compare(MANIFEST, paths)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("invalid", result.stdout)
        self.assertIn("scenario.sha256 does not match", result.stdout)


if __name__ == "__main__":
    sys.exit(0 if unittest.main(exit=False).result.wasSuccessful() else 1)
