"""Tests for the trace grader: the summary it prints, and that it picks the newest trace."""

from __future__ import annotations

import json
import pathlib
import tempfile

import grade_trace

FIXTURE = pathlib.Path(__file__).resolve().parent.parent / "tests" / "fixtures" / "example_trace.jsonl"


def test_summarize_reports_the_contract_fields() -> None:
    summary = grade_trace.summarize(grade_trace.load(FIXTURE))
    # The example trace is deliberately not evidence-grade: it names what it could not observe.
    assert summary["capture"] in {"complete", "partial", "unavailable"}
    if summary["capture"] != "complete":
        assert summary["missing"], "a non-complete capture must name what is missing"
    assert summary["target_sha256"] and len(summary["target_sha256"]) == 64
    assert summary["scenario"]
    assert summary["events"] == summary["counts"]["events"]
    assert summary["milestones"], "the example trace has milestones"
    assert summary["hooks"], "the example trace has hook events"


def test_summarize_names_the_target_owner_when_present() -> None:
    records = [
        {"type": "meta", "capture": {"status": "partial"}, "target": None, "mods": []},
        {
            "type": "event", "kind": "hook", "thread": {"role": "main"},
            "hook": {"key": "GeometryDash.exe:rva:0x3b8eb0", "target": {"module": "absolllute.hackmega", "priority": -32}},
        },
        {
            "type": "event", "kind": "hook", "thread": {"role": "main"},
            "hook": {"key": "GeometryDash.exe:rva:0x3b8eb0", "target": None},
        },
        {"type": "end", "status": "stopped", "counts": {"events": 2}},
    ]
    summary = grade_trace.summarize(records)
    key = ("GeometryDash.exe:rva:0x3b8eb0", "absolllute.hackmega", -32)
    assert summary["hooks"][key] == 1
    assert summary["hooks"][("GeometryDash.exe:rva:0x3b8eb0", None, None)] == 1
    assert summary["main_thread_events"] == 2


def test_cli_grades_the_newest_trace_in_a_directory() -> None:
    older = FIXTURE.read_text(encoding="utf-8")
    with tempfile.TemporaryDirectory() as tmp:
        directory = pathlib.Path(tmp)
        (directory / "a.jsonl").write_text(older, encoding="utf-8")
        (directory / "b.jsonl").write_text(older, encoding="utf-8")
        # Make `b` newer without depending on write ordering.
        for name in ("a.jsonl", "b.jsonl"):
            (directory / name).touch()
        import os
        import time

        os.utime(directory / "b.jsonl", (time.time(), time.time() + 5))

        import contextlib
        import io

        with contextlib.redirect_stdout(io.StringIO()):
            code = grade_trace.main(["--latest", "--dir", str(directory), "--no-check"])
        assert code == 0


def test_cli_without_arguments_exits_with_usage() -> None:
    import contextlib
    import io

    with contextlib.redirect_stderr(io.StringIO()):
        try:
            grade_trace.main([])
        except SystemExit as exit_code:
            assert exit_code.code == 2
        else:
            raise AssertionError("no arguments should be a usage error")


if __name__ == "__main__":
    for name, function in sorted(globals().items()):
        if name.startswith("test_") and callable(function):
            function()
    print("all grade_trace tests passed")
