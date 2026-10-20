#!/usr/bin/env python3
"""Regression test for scripts/dump_tabs.py."""

from __future__ import annotations

import pathlib
import sys
import unittest

REPO = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO))
DLL = REPO / "samples" / "absolllute.hackmega" / "absolllute.hackmega_original.dll"

from scripts.dump_tabs import extract_tabs


class TestDumpTabs(unittest.TestCase):
    def test_extract_tabs(self):
        if not DLL.exists():
            self.skipTest("sample DLL not present")
        tabs = extract_tabs(DLL)
        self.assertIn("HOME", tabs)
        self.assertIn("KEYBINDS", tabs)

        home = tabs["HOME"]
        self.assertEqual(home["type"], "tab")
        self.assertEqual(home["id"], "HOME")
        self.assertGreater(len(home["elements"]), 5)

        kb = tabs["KEYBINDS"]
        self.assertEqual(kb["type"], "tab")
        self.assertEqual(kb["id"], "KEYBINDS")
        self.assertEqual(len(kb["elements"]), 5)
        elem_ids = [e["id"] for e in kb["elements"]]
        self.assertEqual(elem_ids, ["SHORTCUT", "SET", "VIEW", "REMOVE", "DISABLE_IN_EDITOR"])


if __name__ == "__main__":
    unittest.main()
