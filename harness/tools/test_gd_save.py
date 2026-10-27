"""Tests for gd_save.py: the container is what we think it is, and encoding is deterministic.

No user save file is read here - the fixtures are built by the encoder itself, so the test runs
on a fresh clone. The decode of a real GD-written file is a documented manual check
(notes/gd-save-format.md), not a test, because the file is user data.

    python harness/tools/test_gd_save.py
"""

from __future__ import annotations

import importlib.util
import struct
import sys
import tempfile
import unittest
import zlib
from pathlib import Path

HERE = Path(__file__).resolve().parent
_spec = importlib.util.spec_from_file_location("gd_save", HERE / "gd_save.py")
gd = importlib.util.module_from_spec(_spec)
assert _spec.loader is not None
_spec.loader.exec_module(gd)

XML = (
    b'<?xml version="1.0"?><plist version="1.0" gjver="2.0"><dict>'
    b"<k>LLM_01</k><d><k>_isArr</k><t /><k>k_0</k><d><k>kCEK</k><i>4</i>"
    b"<k>k2</k><s>fixture</s><k>k4</k><s>AA==</s></d></d></dict></plist>"
)


class ContainerTest(unittest.TestCase):
    def test_round_trip(self) -> None:
        self.assertEqual(gd.decode(gd.encode(XML)), XML)

    def test_encoding_is_deterministic(self) -> None:
        self.assertEqual(gd.encode(XML), gd.encode(XML))

    def test_header_matches_the_one_gd_writes(self) -> None:
        # GD's own header, observed on two files: zero mtime, XFL 0, OS 10. The container's
        # first four bytes are "C?xB", which is "H4sI" with the 0x0B XOR applied.
        self.assertTrue(gd.encode(XML).startswith(b"C?xB"))
        plain = bytes(b ^ gd.XOR_KEY for b in gd.encode(XML))
        self.assertTrue(plain.startswith(b"H4sI"))

    def test_gzip_fields_are_the_observed_ones(self) -> None:
        plain = bytes(b ^ gd.XOR_KEY for b in gd.encode(XML))
        import base64

        stream = base64.urlsafe_b64decode(plain + b"=" * (-len(plain) % 4))
        self.assertEqual(stream[:10], b"\x1f\x8b\x08\x00\x00\x00\x00\x00\x00\x0a")
        self.assertEqual(struct.unpack("<I", stream[-4:])[0], len(XML) & 0xFFFFFFFF)

    def test_payload_uses_the_urlsafe_alphabet_without_padding(self) -> None:
        plain = bytes(b ^ gd.XOR_KEY for b in gd.encode(XML))
        self.assertNotIn(b"=", plain)
        self.assertNotIn(b"+", plain)
        self.assertNotIn(b"/", plain)

    def test_decode_accepts_padding_and_whitespace(self) -> None:
        # GD writes no padding, but a hand-edited file may carry either; both are the same save.
        container = gd.encode(XML)
        plain = bytes(b ^ gd.XOR_KEY for b in container)
        padded = plain + b"==" + b"\r\n"
        self.assertEqual(gd.decode(bytes(b ^ gd.XOR_KEY for b in padded)), XML)

    def test_empty_file_is_rejected(self) -> None:
        with self.assertRaises(gd.SaveFormatError):
            gd.decode(b"")

    def test_not_base64_is_rejected(self) -> None:
        with self.assertRaises(gd.SaveFormatError):
            gd.decode(bytes(b ^ gd.XOR_KEY for b in b"\x00\x01\x02\x03"))

    def test_not_gzip_is_rejected(self) -> None:
        import base64

        junk = base64.urlsafe_b64encode(b"not a gzip stream")
        with self.assertRaises(gd.SaveFormatError):
            gd.decode(bytes(b ^ gd.XOR_KEY for b in junk))

    def test_truncated_stream_is_rejected(self) -> None:
        container = gd.encode(XML)
        with self.assertRaises(gd.SaveFormatError):
            gd.decode(container[:-8])

    def test_summary_reports_a_gd_plist(self) -> None:
        facts = gd.summarise(XML)
        self.assertTrue(facts["is_gd_plist"])
        self.assertEqual(facts["xml_bytes"], len(XML))

    def test_cli_check_round_trip(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "save.dat"
            path.write_bytes(gd.encode(XML))
            self.assertEqual(gd.main(["--check", str(path)]), 0)


if __name__ == "__main__":
    sys.exit(0 if unittest.main(exit=False).result.wasSuccessful() else 1)
