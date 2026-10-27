"""Geometry Dash 2.2081 save container: decode and encode.

The `.dat` files GD writes (`CCLocalLevels.dat`, `CCGameManager.dat`, `CCGameManager2.dat`)
are one container, recovered by observation on 2026-10-27 (notes/gd-save-format.md):

    file = XOR( base64url_nopad( gzip( plist_xml ) ), 0x0B )

Three independent facts pin it:

1. `cipher[0..3] ^ 0x0B` is `H4sI`, the standard base64 prefix of a gzip stream with a zero
   timestamp. Every byte of the file is `plain ^ 0x0B`, so 0x0B is the smallest byte present.
2. The result decodes only with the **urlsafe** alphabet: the plaintext contains `-` and `_`
   and no `+`, `/` or padding.
3. The decompressed bytes are `<?xml version="1.0"?><plist version="1.0" gjver="2.0">` - GD's
   own plist dialect (`<k>/<d>/<s>/<i>/<r>/<t/>`), not Apple's.

Confirmed on two files written by two different code paths: `CCLocalLevels.dat`
(3,086,667 bytes of XML) and `CCGameManager2.dat` (5,377,449 bytes).

Encoding is deterministic on purpose: zero mtime, the XFL/OS bytes GD itself writes, and
`zlib` level 6. The same XML therefore always yields the same container, which is what makes a
generated fixture usable as evidence.

    python harness/tools/gd_save.py --check <file.dat>
    python harness/tools/gd_save.py --decode <in.dat> <out.xml>
    python harness/tools/gd_save.py --encode <in.xml> <out.dat>
"""

from __future__ import annotations

import argparse
import base64
import binascii
import re
import struct
import sys
import zlib
from pathlib import Path

XOR_KEY = 0x0B

# GD's own gzip header: magic, deflate, no flags, zero mtime, XFL 0, OS 10.
_GZIP_HEADER = b"\x1f\x8b\x08\x00" + b"\x00\x00\x00\x00" + b"\x00" + b"\x0a"

# The urlsafe alphabet with padding stripped: what GD writes after the XOR.
_B64 = re.compile(rb"[^A-Za-z0-9\-_]")


class SaveFormatError(ValueError):
    """The bytes are not a GD save container."""


def decode(data: bytes) -> bytes:
    """Container bytes -> the plist XML GD wrote."""
    if not data:
        raise SaveFormatError("empty file")
    plain = bytes(byte ^ XOR_KEY for byte in data)
    stripped = _B64.sub(b"", plain).rstrip(b"=")
    if not stripped:
        raise SaveFormatError("no base64 payload after the XOR")
    try:
        compressed = base64.urlsafe_b64decode(stripped + b"=" * (-len(stripped) % 4))
    except (binascii.Error, ValueError) as error:
        raise SaveFormatError(f"payload is not base64url: {error}") from error
    try:
        return zlib.decompress(compressed, wbits=16 + zlib.MAX_WBITS)
    except zlib.error as error:
        raise SaveFormatError(f"payload is not a gzip stream: {error}") from error


def encode(xml: bytes) -> bytes:
    """Plist XML -> container bytes. Deterministic: same input, same bytes."""
    compressor = zlib.compressobj(6, zlib.DEFLATED, -zlib.MAX_WBITS)
    deflate = compressor.compress(xml) + compressor.flush()
    stream = (
        _GZIP_HEADER
        + deflate
        + struct.pack("<I", zlib.crc32(xml) & 0xFFFFFFFF)
        + struct.pack("<I", len(xml) & 0xFFFFFFFF)
    )
    text = base64.urlsafe_b64encode(stream).rstrip(b"=")
    return bytes(byte ^ XOR_KEY for byte in text)


def decode_file(path: Path) -> bytes:
    return decode(path.read_bytes())


def summarise(xml: bytes) -> dict[str, object]:
    """A few facts a reader can check without opening 3 MB of XML."""
    head = xml[:200].decode("utf-8", "replace")
    keys = re.findall(r"<k>([^<]+)</k>", head)
    return {
        "xml_bytes": len(xml),
        "xml_sha256": __import__("hashlib").sha256(xml).hexdigest(),
        "root_keys": keys[:6],
        "is_gd_plist": head.startswith('<?xml version="1.0"?><plist version="1.0" gjver='),
    }


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument("--check", metavar="DAT", help="decode and report")
    group.add_argument("--decode", nargs=2, metavar=("DAT", "XML"))
    group.add_argument("--encode", nargs=2, metavar=("XML", "DAT"))
    args = parser.parse_args(argv)

    try:
        if args.check:
            facts = summarise(decode_file(Path(args.check)))
            for name, value in facts.items():
                print(f"{name}: {value}")
            return 0 if facts["is_gd_plist"] else 1
        if args.decode:
            source, target = Path(args.decode[0]), Path(args.decode[1])
            target.write_bytes(decode_file(source))
            print(f"decoded {source.name} -> {target.name} ({target.stat().st_size} bytes)")
            return 0
        source, target = Path(args.encode[0]), Path(args.encode[1])
        target.write_bytes(encode(source.read_bytes()))
        print(f"encoded {source.name} -> {target.name} ({target.stat().st_size} bytes)")
        return 0
    except (OSError, SaveFormatError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
