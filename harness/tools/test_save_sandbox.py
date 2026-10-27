"""Tests for save_sandbox.py: nothing is overwritten that was not backed up and verified.

Everything here runs on synthetic directories under `tempfile`; no real save is touched, and the
process guard is patched so the result never depends on what happens to be running.

    python harness/tools/test_save_sandbox.py
"""

from __future__ import annotations

import importlib.util
import io
import json
import sys
import tempfile
import unittest
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path
from unittest import mock

HERE = Path(__file__).resolve().parent
_spec = importlib.util.spec_from_file_location("save_sandbox", HERE / "save_sandbox.py")
sandbox = importlib.util.module_from_spec(_spec)
assert _spec.loader is not None
_spec.loader.exec_module(sandbox)

FIXTURE_XML = (
    b'<?xml version="1.0"?><plist version="1.0" gjver="2.0"><dict><k>LLM_01</k><d>'
    b"<k>_isArr</k><t /></d></dict></plist>"
)

ORIGINAL_LEVELS = b"player levels"
ORIGINAL_MANAGER = b"player manager"


def make_save(directory: Path) -> Path:
    save_dir = directory / "GeometryDash"
    save_dir.mkdir()
    (save_dir / "CCLocalLevels.dat").write_bytes(ORIGINAL_LEVELS)
    # GD keeps the previous good levels save beside the primary and restores from it, so a
    # realistic save directory has both.
    (save_dir / "CCLocalLevels2.dat").write_bytes(ORIGINAL_LEVELS)
    (save_dir / "CCGameManager.dat").write_bytes(ORIGINAL_MANAGER)
    return save_dir


class SandboxTest(unittest.TestCase):
    def setUp(self) -> None:
        self._temp = tempfile.TemporaryDirectory()
        self.addCleanup(self._temp.cleanup)
        self.root = Path(self._temp.name)
        self.save_dir = make_save(self.root)
        self.backup_dir = self.root / "backup"
        self.fixture = self.root / "fixture.gmd"
        self.fixture.write_bytes(FIXTURE_XML)
        # The guard is real code; the tests decide what "running" means instead of asking the
        # machine, so they behave the same on a developer box and in CI.
        self._not_running = mock.patch.object(sandbox, "running_image_names", return_value=[])
        self._not_running.start()
        self.addCleanup(self._not_running.stop)

    def game_is_running(self) -> None:
        self._not_running.stop()
        self._running = mock.patch.object(
            sandbox, "running_image_names", return_value=["explorer.exe", "GeometryDash.exe"]
        )
        self._running.start()
        self.addCleanup(self._running.stop)

    # --- process guard --------------------------------------------------------------------

    def test_process_guard_matches_the_game_case_insensitively(self) -> None:
        self.assertEqual(sandbox.game_processes(["geometrydash.EXE"]), ["geometrydash.EXE"])
        self.assertEqual(
            sandbox.game_processes(["GeometryDashLauncher.exe", "explorer.exe"]), []
        )
        with self.assertRaises(sandbox.SandboxError):
            sandbox.assert_game_not_running(["GeometryDash.exe"])

    def test_backup_refuses_while_the_game_is_running(self) -> None:
        self.game_is_running()
        with self.assertRaises(sandbox.SandboxError) as raised:
            sandbox.backup(self.save_dir, self.backup_dir)
        self.assertIn("Geometry Dash is running", str(raised.exception))
        self.assertFalse(self.backup_dir.exists())

    def test_install_refuses_while_the_game_is_running(self) -> None:
        sandbox.backup(self.save_dir, self.backup_dir)
        self.game_is_running()
        with self.assertRaises(sandbox.SandboxError):
            sandbox.install(self.save_dir, self.backup_dir, self.fixture)
        self.assertEqual((self.save_dir / "CCLocalLevels.dat").read_bytes(), ORIGINAL_LEVELS)

    def test_restore_refuses_while_the_game_is_running(self) -> None:
        sandbox.backup(self.save_dir, self.backup_dir)
        sandbox.install(self.save_dir, self.backup_dir, self.fixture)
        installed = (self.save_dir / "CCLocalLevels.dat").read_bytes()
        self.game_is_running()
        with self.assertRaises(sandbox.SandboxError):
            sandbox.restore(self.save_dir, self.backup_dir)
        self.assertEqual((self.save_dir / "CCLocalLevels.dat").read_bytes(), installed)

    # --- backup ---------------------------------------------------------------------------

    def test_backup_records_digests_and_copies(self) -> None:
        files = sandbox.backup(self.save_dir, self.backup_dir)
        self.assertEqual(
            set(files),
            {"CCLocalLevels.dat", "CCLocalLevels2.dat", "CCGameManager.dat"},
        )
        self.assertEqual((self.backup_dir / "CCLocalLevels.dat").read_bytes(), ORIGINAL_LEVELS)
        self.assertTrue((self.backup_dir / sandbox.RECORD).is_file())

    def test_install_writes_both_levels_files(self) -> None:
        # Regression: GD restores CCLocalLevels.dat from CCLocalLevels2.dat at startup, so an
        # install that wrote only the primary was undone 1.3 s after the game launched.
        sandbox.backup(self.save_dir, self.backup_dir)
        sandbox.install(self.save_dir, self.backup_dir, self.fixture)
        expected = sandbox.gd_save.encode(FIXTURE_XML)
        for name in sandbox.LEVEL_FILES:
            self.assertEqual((self.save_dir / name).read_bytes(), expected, name)

    def test_install_is_a_no_op_only_when_both_files_already_match(self) -> None:
        sandbox.backup(self.save_dir, self.backup_dir)
        sandbox.install(self.save_dir, self.backup_dir, self.fixture)
        # A retry needs no fresh backup once the pair is consistent...
        sandbox.install(self.save_dir, self.backup_dir, self.fixture)
        # ...but a half-installed pair (primary only) must not be mistaken for done.
        (self.save_dir / "CCLocalLevels2.dat").write_bytes(ORIGINAL_LEVELS)
        sandbox.install(self.save_dir, self.backup_dir, self.fixture)
        expected = sandbox.gd_save.encode(FIXTURE_XML)
        self.assertEqual((self.save_dir / "CCLocalLevels2.dat").read_bytes(), expected)

    def test_restore_returns_both_levels_files(self) -> None:
        sandbox.backup(self.save_dir, self.backup_dir)
        sandbox.install(self.save_dir, self.backup_dir, self.fixture)
        sandbox.restore(self.save_dir, self.backup_dir)
        for name in sandbox.LEVEL_FILES:
            self.assertEqual((self.save_dir / name).read_bytes(), ORIGINAL_LEVELS, name)

    def test_backup_verifies_its_own_copies(self) -> None:
        def truncating_copy(source: Path, target: Path) -> None:
            target.write_bytes(b"short")

        with mock.patch.object(sandbox, "_copy_atomic", side_effect=truncating_copy):
            with self.assertRaises(sandbox.SandboxError) as raised:
                sandbox.backup(self.save_dir, self.backup_dir)
        self.assertIn("does not match", str(raised.exception))
        # No record: an interrupted backup must not vouch for bytes it does not have.
        self.assertFalse((self.backup_dir / sandbox.RECORD).exists())

    def test_backup_refuses_a_second_backup(self) -> None:
        sandbox.backup(self.save_dir, self.backup_dir)
        with self.assertRaises(sandbox.SandboxError):
            sandbox.backup(self.save_dir, self.backup_dir)

    def test_backup_refuses_an_empty_save_directory(self) -> None:
        empty = self.root / "empty"
        empty.mkdir()
        with self.assertRaises(sandbox.SandboxError):
            sandbox.backup(empty, self.root / "empty-backup")

    # --- install --------------------------------------------------------------------------

    def test_install_requires_a_backup(self) -> None:
        with self.assertRaises(sandbox.SandboxError):
            sandbox.install(self.save_dir, self.backup_dir, self.fixture)

    def test_install_replaces_the_levels_save_with_the_container(self) -> None:
        sandbox.backup(self.save_dir, self.backup_dir)
        target = sandbox.install(self.save_dir, self.backup_dir, self.fixture)
        self.assertEqual(sandbox.gd_save.decode(target.read_bytes()), FIXTURE_XML)

    def test_install_is_idempotent_so_a_retry_needs_no_new_backup(self) -> None:
        sandbox.backup(self.save_dir, self.backup_dir)
        first = sandbox.install(self.save_dir, self.backup_dir, self.fixture)
        second = sandbox.install(self.save_dir, self.backup_dir, self.fixture)
        self.assertEqual(first.read_bytes(), second.read_bytes())

    def test_install_refuses_a_save_that_changed_since_the_backup(self) -> None:
        sandbox.backup(self.save_dir, self.backup_dir)
        (self.save_dir / "CCLocalLevels.dat").write_bytes(b"changed by a game run")
        with self.assertRaises(sandbox.SandboxError):
            sandbox.install(self.save_dir, self.backup_dir, self.fixture)

    def test_install_refuses_a_file_that_is_not_the_fixture(self) -> None:
        sandbox.backup(self.save_dir, self.backup_dir)
        self.fixture.write_bytes(b"not xml")
        with self.assertRaises(sandbox.SandboxError):
            sandbox.install(self.save_dir, self.backup_dir, self.fixture)

    def test_install_reads_back_what_it_wrote_and_rolls_back_a_mismatch(self) -> None:
        sandbox.backup(self.save_dir, self.backup_dir)

        def corrupting_write(target: Path, payload: bytes) -> None:
            target.write_bytes(b"corrupt")

        with mock.patch.object(sandbox, "_write_atomic", side_effect=corrupting_write):
            with self.assertRaises(sandbox.SandboxError) as raised:
                sandbox.install(self.save_dir, self.backup_dir, self.fixture)
        self.assertIn("did not verify", str(raised.exception))
        self.assertEqual((self.save_dir / "CCLocalLevels.dat").read_bytes(), ORIGINAL_LEVELS)

    def test_install_is_atomic_when_the_rename_fails(self) -> None:
        sandbox.backup(self.save_dir, self.backup_dir)
        with mock.patch.object(sandbox.os, "replace", side_effect=OSError("file is locked")):
            with self.assertRaises(OSError):
                sandbox.install(self.save_dir, self.backup_dir, self.fixture)
        self.assertEqual((self.save_dir / "CCLocalLevels.dat").read_bytes(), ORIGINAL_LEVELS)
        self.assertEqual(list(self.save_dir.glob("*.tmp-*")), [])

    def test_install_leaves_no_temporary_files(self) -> None:
        sandbox.backup(self.save_dir, self.backup_dir)
        sandbox.install(self.save_dir, self.backup_dir, self.fixture)
        self.assertEqual(list(self.save_dir.glob("*.tmp-*")), [])
        self.assertEqual(list(self.backup_dir.glob("*.tmp-*")), [])

    # --- restore --------------------------------------------------------------------------

    def test_restore_returns_the_original_bytes(self) -> None:
        sandbox.backup(self.save_dir, self.backup_dir)
        sandbox.install(self.save_dir, self.backup_dir, self.fixture)
        sandbox.restore(self.save_dir, self.backup_dir)
        self.assertEqual((self.save_dir / "CCLocalLevels.dat").read_bytes(), ORIGINAL_LEVELS)
        self.assertEqual((self.save_dir / "CCGameManager.dat").read_bytes(), ORIGINAL_MANAGER)

    def test_restore_refuses_an_altered_backup_without_writing(self) -> None:
        sandbox.backup(self.save_dir, self.backup_dir)
        (self.backup_dir / "CCLocalLevels.dat").write_bytes(b"tampered")
        with self.assertRaises(sandbox.SandboxError):
            sandbox.restore(self.save_dir, self.backup_dir)
        self.assertEqual((self.save_dir / "CCLocalLevels.dat").read_bytes(), ORIGINAL_LEVELS)

    def test_restore_reports_the_exact_files_when_one_fails(self) -> None:
        sandbox.backup(self.save_dir, self.backup_dir)
        sandbox.install(self.save_dir, self.backup_dir, self.fixture)
        installed = (self.save_dir / "CCLocalLevels.dat").read_bytes()
        real_copy = sandbox._copy_atomic

        def failing_copy(source: Path, target: Path) -> None:
            if target.name == "CCLocalLevels.dat":
                raise OSError("file is locked by another process")
            real_copy(source, target)

        with mock.patch.object(sandbox, "_copy_atomic", side_effect=failing_copy):
            with self.assertRaises(sandbox.RestoreError) as raised:
                sandbox.restore(self.save_dir, self.backup_dir)
        report = raised.exception
        self.assertEqual([name for name, _ in report.failed], ["CCLocalLevels.dat"])
        self.assertIn("locked", report.failed[0][1])
        self.assertEqual(report.untouched, ["CCLocalLevels.dat"])
        self.assertEqual(sorted(report.restored), ["CCGameManager.dat", "CCLocalLevels2.dat"])
        # The failed file was never replaced, so its bytes are still the installed ones.
        self.assertEqual((self.save_dir / "CCLocalLevels.dat").read_bytes(), installed)

    def test_restore_is_idempotent_after_a_partial_failure(self) -> None:
        sandbox.backup(self.save_dir, self.backup_dir)
        sandbox.install(self.save_dir, self.backup_dir, self.fixture)
        real_copy = sandbox._copy_atomic

        def failing_copy(source: Path, target: Path) -> None:
            if target.name == "CCLocalLevels.dat":
                raise OSError("file is locked by another process")
            real_copy(source, target)

        with mock.patch.object(sandbox, "_copy_atomic", side_effect=failing_copy):
            with self.assertRaises(sandbox.RestoreError):
                sandbox.restore(self.save_dir, self.backup_dir)
        restored = sandbox.restore(self.save_dir, self.backup_dir)
        self.assertEqual(sorted(restored),
                         ["CCGameManager.dat", "CCLocalLevels.dat", "CCLocalLevels2.dat"])
        self.assertEqual(sandbox.status(self.save_dir, self.backup_dir)["files"],
                         {"CCLocalLevels.dat": "same",
                          "CCLocalLevels2.dat": "same",
                          "CCGameManager.dat": "same"})

    def test_restore_leaves_files_created_after_the_backup_in_place(self) -> None:
        sandbox.backup(self.save_dir, self.backup_dir)
        late = self.save_dir / "CCGameManager2.dat"
        late.write_bytes(b"written by the game after the backup")
        sandbox.restore(self.save_dir, self.backup_dir)
        self.assertEqual(late.read_bytes(), b"written by the game after the backup")

    # --- status ---------------------------------------------------------------------------

    def test_status_reports_missing_separately_from_differs(self) -> None:
        sandbox.backup(self.save_dir, self.backup_dir)
        (self.save_dir / "CCLocalLevels.dat").unlink()
        (self.save_dir / "CCGameManager.dat").write_bytes(b"changed")
        report = sandbox.status(self.save_dir, self.backup_dir)
        self.assertEqual(report["files"],
                         {"CCLocalLevels.dat": "missing",
                          "CCLocalLevels2.dat": "same",
                          "CCGameManager.dat": "differs"})

    def test_status_reports_files_created_after_the_backup(self) -> None:
        sandbox.backup(self.save_dir, self.backup_dir)
        (self.save_dir / "CCGameManager2.dat").write_bytes(b"late")
        report = sandbox.status(self.save_dir, self.backup_dir)
        self.assertEqual(report["extras"], ["CCGameManager2.dat"])
        self.assertEqual(report["files"],
                         {"CCLocalLevels.dat": "same",
                          "CCLocalLevels2.dat": "same",
                          "CCGameManager.dat": "same"})

    def test_status_after_install_shows_the_fixture_and_the_same_manager(self) -> None:
        sandbox.backup(self.save_dir, self.backup_dir)
        sandbox.install(self.save_dir, self.backup_dir, self.fixture)
        self.assertEqual(sandbox.status(self.save_dir, self.backup_dir)["files"],
                         {"CCLocalLevels.dat": "differs",
                          "CCLocalLevels2.dat": "differs",
                          "CCGameManager.dat": "same"})

    # --- records --------------------------------------------------------------------------

    def test_a_corrupt_backup_record_is_refused_without_a_traceback(self) -> None:
        self.backup_dir.mkdir()
        record = self.backup_dir / sandbox.RECORD
        for payload in ('{"files": ', '[]', '{"files": {}}',
                        '{"files": {"CCLocalLevels.dat": "not facts"}}'):
            record.write_text(payload, encoding="utf-8")
            with self.assertRaises(sandbox.SandboxError):
                sandbox.restore(self.save_dir, self.backup_dir)
            with self.assertRaises(sandbox.SandboxError):
                sandbox.status(self.save_dir, self.backup_dir)

    def test_cli_reports_a_corrupt_record_as_an_error_not_a_crash(self) -> None:
        self.backup_dir.mkdir()
        (self.backup_dir / sandbox.RECORD).write_text("nonsense", encoding="utf-8")
        error = io.StringIO()
        with redirect_stderr(error):
            code = sandbox.main(["--status", "--save-dir", str(self.save_dir),
                                 "--backup-dir", str(self.backup_dir)])
        self.assertEqual(code, 2)
        self.assertIn("not readable JSON", error.getvalue())

    # --- CLI ------------------------------------------------------------------------------

    def test_cli_backup_install_restore(self) -> None:
        args = ["--save-dir", str(self.save_dir), "--backup-dir", str(self.backup_dir)]
        with redirect_stdout(io.StringIO()):
            self.assertEqual(sandbox.main(["--backup"] + args), 0)
            self.assertEqual(
                sandbox.main(["--install", "--fixture", str(self.fixture)] + args), 0)
            self.assertEqual(sandbox.main(["--restore"] + args), 0)
        self.assertEqual((self.save_dir / "CCLocalLevels.dat").read_bytes(), ORIGINAL_LEVELS)

    def test_cli_status_lists_extras_and_the_policy(self) -> None:
        with redirect_stdout(io.StringIO()):
            sandbox.main(["--backup", "--save-dir", str(self.save_dir),
                          "--backup-dir", str(self.backup_dir)])
        (self.save_dir / "CCGameManager2.dat").write_bytes(b"late")
        output = io.StringIO()
        with redirect_stdout(output):
            self.assertEqual(
                sandbox.main(["--status", "--save-dir", str(self.save_dir),
                              "--backup-dir", str(self.backup_dir)]), 0)
        text = output.getvalue()
        self.assertIn("extra: CCGameManager2.dat", text)
        self.assertIn("left in place", text)

    def test_cli_install_without_a_fixture_is_a_usage_error(self) -> None:
        with redirect_stderr(io.StringIO()), self.assertRaises(SystemExit) as raised:
            sandbox.main(["--install", "--save-dir", str(self.save_dir),
                          "--backup-dir", str(self.backup_dir)])
        self.assertEqual(raised.exception.code, 2)


if __name__ == "__main__":
    sys.exit(0 if unittest.main(exit=False).result.wasSuccessful() else 1)
