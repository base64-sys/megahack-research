"""Tests for save_sandbox.py: nothing is overwritten that was not backed up and verified.

    python harness/tools/test_save_sandbox.py
"""

from __future__ import annotations

import importlib.util
import sys
import tempfile
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
_spec = importlib.util.spec_from_file_location("save_sandbox", HERE / "save_sandbox.py")
sandbox = importlib.util.module_from_spec(_spec)
assert _spec.loader is not None
_spec.loader.exec_module(sandbox)

FIXTURE_XML = (
    b'<?xml version="1.0"?><plist version="1.0" gjver="2.0"><dict><k>LLM_01</k><d>'
    b"<k>_isArr</k><t /></d></dict></plist>"
)


def make_save(directory: Path) -> Path:
    save_dir = directory / "GeometryDash"
    save_dir.mkdir()
    (save_dir / "CCLocalLevels.dat").write_bytes(b"player levels")
    (save_dir / "CCGameManager.dat").write_bytes(b"player manager")
    return save_dir


class SandboxTest(unittest.TestCase):
    def setUp(self) -> None:
        self._temp = tempfile.TemporaryDirectory()
        self.root = Path(self._temp.name)
        self.save_dir = make_save(self.root)
        self.backup_dir = self.root / "backup"
        self.fixture = self.root / "fixture.gmd"
        self.fixture.write_bytes(FIXTURE_XML)

    def tearDown(self) -> None:
        self._temp.cleanup()

    def test_backup_records_digests_and_copies(self) -> None:
        files = sandbox.backup(self.save_dir, self.backup_dir)
        self.assertEqual(set(files), {"CCLocalLevels.dat", "CCGameManager.dat"})
        self.assertEqual(
            (self.backup_dir / "CCLocalLevels.dat").read_bytes(), b"player levels"
        )

    def test_install_requires_a_backup(self) -> None:
        with self.assertRaises(sandbox.SandboxError):
            sandbox.install(self.save_dir, self.backup_dir, self.fixture)

    def test_install_replaces_the_levels_save_with_the_container(self) -> None:
        sandbox.backup(self.save_dir, self.backup_dir)
        target = sandbox.install(self.save_dir, self.backup_dir, self.fixture)
        self.assertEqual(sandbox.gd_save.decode(target.read_bytes()), FIXTURE_XML)

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

    def test_restore_returns_the_original_bytes(self) -> None:
        sandbox.backup(self.save_dir, self.backup_dir)
        sandbox.install(self.save_dir, self.backup_dir, self.fixture)
        sandbox.restore(self.save_dir, self.backup_dir)
        self.assertEqual((self.save_dir / "CCLocalLevels.dat").read_bytes(), b"player levels")
        self.assertEqual((self.save_dir / "CCGameManager.dat").read_bytes(), b"player manager")

    def test_restore_refuses_an_altered_backup(self) -> None:
        sandbox.backup(self.save_dir, self.backup_dir)
        (self.backup_dir / "CCLocalLevels.dat").write_bytes(b"tampered")
        with self.assertRaises(sandbox.SandboxError):
            sandbox.restore(self.save_dir, self.backup_dir)
        self.assertEqual((self.save_dir / "CCLocalLevels.dat").read_bytes(), b"player levels")

    def test_second_backup_is_refused(self) -> None:
        sandbox.backup(self.save_dir, self.backup_dir)
        with self.assertRaises(sandbox.SandboxError):
            sandbox.backup(self.save_dir, self.backup_dir)

    def test_status_reports_same_and_differs(self) -> None:
        sandbox.backup(self.save_dir, self.backup_dir)
        self.assertEqual(
            sandbox.status(self.save_dir, self.backup_dir),
            {"CCLocalLevels.dat": "same", "CCGameManager.dat": "same"},
        )
        sandbox.install(self.save_dir, self.backup_dir, self.fixture)
        self.assertEqual(
            sandbox.status(self.save_dir, self.backup_dir),
            {"CCLocalLevels.dat": "differs", "CCGameManager.dat": "same"},
        )

    def test_cli_backup_install_restore(self) -> None:
        args = ["--save-dir", str(self.save_dir), "--backup-dir", str(self.backup_dir)]
        self.assertEqual(sandbox.main(["--backup"] + args), 0)
        self.assertEqual(sandbox.main(["--install", "--fixture", str(self.fixture)] + args), 0)
        self.assertEqual(sandbox.main(["--restore"] + args), 0)
        self.assertEqual((self.save_dir / "CCLocalLevels.dat").read_bytes(), b"player levels")


if __name__ == "__main__":
    sys.exit(0 if unittest.main(exit=False).result.wasSuccessful() else 1)
