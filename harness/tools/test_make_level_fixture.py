"""Tests for make_level_fixture.py: deterministic, self-consistent, and refused when unsafe.

    python harness/tools/test_make_level_fixture.py
"""

from __future__ import annotations

import importlib.util
import re
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

HERE = Path(__file__).resolve().parent
_spec = importlib.util.spec_from_file_location("make_level_fixture", HERE / "make_level_fixture.py")
fixture = importlib.util.module_from_spec(_spec)
assert _spec.loader is not None
_spec.loader.exec_module(fixture)

OBJECTS = ((1, 150, 15), (1, 600, 15))


def render() -> bytes:
    return fixture.render_level("fixture", "megahack-research", OBJECTS)


class LevelStringTest(unittest.TestCase):
    def test_object_string_shape(self) -> None:
        text = fixture.level_string(OBJECTS)
        self.assertTrue(text.startswith("kS38,"))
        self.assertIn("1,1,2,150,3,15;", text)
        self.assertTrue(text.endswith(";"))

    def test_k4_round_trip(self) -> None:
        text = fixture.level_string(OBJECTS)
        self.assertEqual(fixture.decode_level_string(fixture.encode_level_string(text)), text)

    def test_k4_is_url_safe_and_unpadded(self) -> None:
        encoded = fixture.encode_level_string(fixture.level_string(OBJECTS))
        self.assertEqual(encoded, encoded.rstrip("="))
        self.assertNotIn("+", encoded)
        self.assertNotIn("/", encoded)


class FixtureTest(unittest.TestCase):
    def test_deterministic(self) -> None:
        self.assertEqual(render(), render())

    def test_is_a_gd_plist_with_one_level(self) -> None:
        xml = render().decode("utf-8")
        self.assertTrue(xml.startswith(fixture.PLIST_OPEN))
        self.assertTrue(xml.endswith(fixture.PLIST_CLOSE))
        self.assertEqual(xml.count("<k>LLM_01</k>"), 1)
        self.assertEqual(xml.count("<k>k_0</k>"), 1)

    def test_carries_only_the_keys_whose_meaning_is_established(self) -> None:
        # Regression: emitting all 43 observed keys with invented zeros (including an empty
        # `<d></d>` for kI6) made GD show no levels at all. Only kCEK, k2, k4 and k5 are
        # understood, so only those are written.
        keys = [key for key, _, _ in fixture.level_dict("n", "a", OBJECTS)]
        self.assertEqual(keys, ["kCEK", "k2", "k4", "k5"])

    def test_name_and_author_are_escaped(self) -> None:
        xml = fixture.render_level("a<b&c", "x>y", OBJECTS).decode("utf-8")
        self.assertIn("<s>a&lt;b&amp;c</s>", xml)
        self.assertIn("<s>x&gt;y</s>", xml)

    def test_level_string_contains_every_object(self) -> None:
        xml = render().decode("utf-8")
        k4 = re.search(r"<k>k4</k><s>([^<]+)</s>", xml)
        assert k4 is not None
        text = fixture.decode_level_string(k4.group(1))
        self.assertEqual(text.count("1,1,2,"), len(OBJECTS))

    def _backup(self, save_dir: Path, backup_dir: Path) -> None:
        # The process guard is real code; decide "not running" here rather than asking the box.
        with mock.patch.object(fixture.save_sandbox, "running_image_names", return_value=[]):
            fixture.save_sandbox.backup(save_dir, backup_dir)

    def test_installs_a_container_that_decodes_to_the_same_xml(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            save_dir = Path(directory) / "GeometryDash"
            save_dir.mkdir()
            (save_dir / "CCLocalLevels.dat").write_bytes(b"somebody's levels")
            backup_dir = Path(directory) / "backup"
            self._backup(save_dir, backup_dir)
            xml = render()
            with mock.patch.object(fixture.save_sandbox, "running_image_names", return_value=[]):
                target = fixture.install(xml, save_dir, backup_dir)
            self.assertEqual(fixture.gd_save.decode(target.read_bytes()), xml)

    def test_install_refuses_without_a_verified_backup(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            save_dir = Path(directory)
            (save_dir / "CCLocalLevels.dat").write_bytes(b"somebody's levels")
            with mock.patch.object(fixture.save_sandbox, "running_image_names", return_value=[]):
                with self.assertRaises(fixture.save_sandbox.SandboxError):
                    fixture.install(render(), save_dir, Path(directory) / "no-backup")
            self.assertEqual((save_dir / "CCLocalLevels.dat").read_bytes(), b"somebody's levels")

    def test_install_keeps_the_save_recoverable_from_the_sandbox_backup(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            save_dir = Path(directory)
            original = b"somebody's levels"
            (save_dir / "CCLocalLevels.dat").write_bytes(original)
            backup_dir = Path(directory) / "backup"
            sandbox = fixture.save_sandbox
            with mock.patch.object(sandbox, "running_image_names", return_value=[]):
                sandbox.backup(save_dir, backup_dir)
                fixture.install(render(), save_dir, backup_dir)
                self.assertNotEqual((save_dir / "CCLocalLevels.dat").read_bytes(), original)
                sandbox.restore(save_dir, backup_dir)
            self.assertEqual((save_dir / "CCLocalLevels.dat").read_bytes(), original)

    def test_cli_writes_the_fixture(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            out = Path(directory) / "fixture.gmd"
            self.assertEqual(fixture.main(["--out", str(out)]), 0)
            expected = fixture.render_level(
                "safe-mode-smoke fixture", "megahack-research", fixture.DEFAULT_OBJECTS
            )
            self.assertEqual(out.read_bytes(), expected)


if __name__ == "__main__":
    sys.exit(0 if unittest.main(exit=False).result.wasSuccessful() else 1)
