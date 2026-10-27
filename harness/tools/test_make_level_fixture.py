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

    def test_carries_the_observed_key_set_in_order(self) -> None:
        keys = [key for key, _, _ in fixture.level_dict("n", "a", OBJECTS)]
        self.assertEqual(len(keys), 43)
        self.assertEqual(keys[:6], ["kCEK", "k18", "k36", "k23", "k2", "k4"])
        self.assertEqual(keys[-1], "13")

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

    def test_installs_a_container_that_decodes_to_the_same_xml(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            save_dir = Path(directory) / "GeometryDash"
            xml = render()
            target = fixture.install(xml, save_dir)
            self.assertEqual(fixture.gd_save.decode(target.read_bytes()), xml)

    def test_refuses_to_replace_an_existing_save_without_force(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            save_dir = Path(directory)
            (save_dir / "CCLocalLevels.dat").write_bytes(b"somebody's levels")
            with self.assertRaises(SystemExit):
                fixture.install(render(), save_dir)
            self.assertEqual((save_dir / "CCLocalLevels.dat").read_bytes(), b"somebody's levels")

    def test_force_keeps_a_backup(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            save_dir = Path(directory)
            original = b"somebody's levels"
            (save_dir / "CCLocalLevels.dat").write_bytes(original)
            fixture.install(render(), save_dir, force=True)
            self.assertEqual((save_dir / "CCLocalLevels.dat.bak").read_bytes(), original)
            self.assertNotEqual((save_dir / "CCLocalLevels.dat").read_bytes(), original)

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
