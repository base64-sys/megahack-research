"""Tests for verify_record_bytes.py.

Two things are worth pinning: the payload the script extracts is exactly what the builders
produce (otherwise "identical" would be a comparison of two trimmed strings), and the
hand-built-record guard actually fires - that guard is the reason the split between
`mh::records` and the mod exists.

    python harness/tools/test_verify_record_bytes.py
"""

from __future__ import annotations

import importlib.util
import sys
import tempfile
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
SCRIPT = HERE / "verify_record_bytes.py"
FIXTURES = HERE.parent / "tests" / "fixtures" / "records_captured.jsonl"

_spec = importlib.util.spec_from_file_location("verify_record_bytes", SCRIPT)
verify = importlib.util.module_from_spec(_spec)
assert _spec.loader is not None
_spec.loader.exec_module(verify)

ENVELOPE_HEAD = (
    '{"schema":"mh-trace/1","type":"event","run_id":"00000000-0000-4000-8000-000000000001",'
    '"run_fingerprint":"' + "0" * 64 + '","seq":1,"t_ns":1,"frame":1,'
    '"thread":{"id":"main","role":"main"},'
)
ENVELOPE_TAIL = ',"capture":{"status":"complete"}}'


def wrapped(payload: str) -> str:
    # A trace line is the envelope *merged* with the payload, so the payload's braces are not
    # its own: `"thread":{...},"kind":"hook"` - not `"thread":{...},{"kind":"hook"`.
    assert payload.startswith("{") and payload.endswith("}")
    return ENVELOPE_HEAD + payload[1:-1] + ENVELOPE_TAIL


class VerifyRecordBytesTest(unittest.TestCase):
    def setUp(self) -> None:
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.dir = Path(self.temp.name)

    def write(self, name: str, text: str) -> Path:
        path = self.dir / name
        path.write_text(text, encoding="utf-8")
        return path

    def run_main(self, trace: Path, observer: Path | None = None) -> int:
        argv = [str(trace)]
        if observer is not None:
            argv += ["--observer", str(observer)]
        return verify.main(argv)

    def test_payload_extraction_keeps_the_record_whole(self) -> None:
        payload = FIXTURES.read_text(encoding="utf-8").splitlines()[0]
        self.assertEqual(verify.payload_of(wrapped(payload)), payload)

    def test_extracted_payload_is_the_fixture(self) -> None:
        payload = FIXTURES.read_text(encoding="utf-8").splitlines()[0]
        self.assertEqual(verify.normalise(verify.payload_of(wrapped(payload))),
                         verify.normalise(payload))

    def test_identical_trace_passes(self) -> None:
        payloads = FIXTURES.read_text(encoding="utf-8").splitlines()
        trace = self.write("t.jsonl", "\n".join(wrapped(p) for p in payloads) + "\n")
        self.assertEqual(self.run_main(trace), 0)

    def test_a_changed_key_fails(self) -> None:
        payloads = FIXTURES.read_text(encoding="utf-8").splitlines()
        changed = payloads[0].replace('"phase":"enter"', '"phase":"exit"')
        trace = self.write("t.jsonl", wrapped(changed) + "\n" + payloads[1] + "\n")
        self.assertEqual(self.run_main(trace), 1)

    def test_a_missing_record_fails(self) -> None:
        payloads = FIXTURES.read_text(encoding="utf-8").splitlines()
        trace = self.write("t.jsonl", wrapped(payloads[0]) + "\n")
        self.assertEqual(self.run_main(trace), 1)

    def test_normalisation_covers_the_two_per_run_fields(self) -> None:
        self.assertEqual(verify.normalise('"va":"0x7FF6762E7F40"'), '"va":"0x0"')
        self.assertIn('"value":"0x0"', verify.normalise(
            '"this":{"state":"observed","type":"ptr","value":"0x2335B9BB010"}'))

    def test_a_hand_built_record_in_the_mod_fails(self) -> None:
        # The guard is a substring check on the mod source: a record assembled in the mod
        # instead of through mh::records is exactly the drift this split prevents.
        payloads = FIXTURES.read_text(encoding="utf-8").splitlines()
        trace = self.write("t.jsonl", "\n".join(wrapped(p) for p in payloads) + "\n")
        offender = self.write("main.cpp", 'body.set("kind", "hook");\n')
        self.assertEqual(self.run_main(trace, offender), 1)

    def test_a_clean_mod_source_passes_the_guard(self) -> None:
        payloads = FIXTURES.read_text(encoding="utf-8").splitlines()
        trace = self.write("t.jsonl", "\n".join(wrapped(p) for p in payloads) + "\n")
        clean = self.write("main.cpp", "// no hand-built record here\nmh::records::hook_event(...);\n")
        self.assertEqual(self.run_main(trace, clean), 0)


if __name__ == "__main__":
    sys.exit(0 if unittest.main(exit=False).result.wasSuccessful() else 1)
