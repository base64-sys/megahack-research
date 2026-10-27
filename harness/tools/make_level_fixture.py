"""Generate the runtime-validation level fixture deterministically.

The fixture is a **generated** level, never a user-created or downloaded one: the contract in
notes/runtime-validation.md forbids those, and a level nobody can regenerate is not evidence.

Two artifacts come out of one declarative spec:

* `<name>.gmd` - the canonical form: GD's own plist dialect, plain text, diffable, and the file
  whose SHA-256 the scenario manifest pins. It is *our* interchange artifact; GD does not read
  `.gmd` files.
* `CCLocalLevels.dat` - the container GD actually loads, produced by
  `--install <save-dir> --backup-dir <dir>` from the same XML through `gd_save.encode`. The
  write goes through `save_sandbox`, so it is guarded, atomic, read back, and only happens
  against a digest-verified backup.

Both come from the same in-memory level, so the installed save and the hashed fixture cannot
disagree.

The level dict carries the full key set observed in a level GD 2.2081 wrote (43 keys). That is
deliberate: a guessed *minimum* key set would be a claim about GD's loader that nothing here
can support. Values for keys whose meaning is unknown are ordinary defaults and are labelled
UNVERIFIED in notes/gd-save-format.md.

    python harness/tools/make_level_fixture.py --out harness/fixtures/runtime/safe-mode-smoke.gmd
    python harness/tools/make_level_fixture.py --install <save-dir> --backup-dir <dir>
"""

from __future__ import annotations

import argparse
import base64
import importlib.util
import sys
from pathlib import Path
from typing import Iterable
from xml.sax.saxutils import escape

HERE = Path(__file__).resolve().parent


def _load(name: str):
    spec = importlib.util.spec_from_file_location(name, HERE / f"{name}.py")
    module = importlib.util.module_from_spec(spec)
    assert spec.loader is not None
    spec.loader.exec_module(module)
    return module


gd_save = _load("gd_save")
save_sandbox = _load("save_sandbox")

# GD writes its own plist dialect: <k> for a key, <d> dict, <s> string, <i> int, <r> real,
# <t /> the empty value. Confirmed against two decoded save files (notes/gd-save-format.md).
PLIST_OPEN = '<?xml version="1.0"?><plist version="1.0" gjver="2.0">'
PLIST_CLOSE = "</plist>"

# One colour channel, in the form GD writes at the head of a level's object string. Without it
# a level has no colour definition for its objects; it is structural, not decorative.
COLOUR_CHANNELS = (
    "kS38,1_0_2_0_3_0_11_255_12_255_13_255_4_-1_6_1000_7_1.000000_15_1.000000_18_0_8_1"
)


def level_string(objects: Iterable[tuple[int, int, int]]) -> str:
    """Build a 2.2081 `k4` payload.

    Each object is `(object_id, x, y)` in GD's 30-unit grid; the encoding is
    `1,<id>,2,<x>,3,<y>;`, which is the key/value form GD itself writes.
    """
    parts = [COLOUR_CHANNELS]
    for object_id, x, y in objects:
        parts.append(f"1,{object_id},2,{x},3,{y}")
    return ";".join(parts) + ";"


def encode_level_string(text: str) -> str:
    """`k4` is base64url(no padding) of gzip(text), with the same header `gd_save` writes."""
    return base64.urlsafe_b64encode(gd_save.gzip_bytes(text.encode("utf-8"))).rstrip(b"=").decode("ascii")


def decode_level_string(value: str) -> str:
    padded = value.rstrip("=") + "=" * (-len(value.rstrip("=")) % 4)
    return gd_save.gunzip(base64.urlsafe_b64decode(padded)).decode("utf-8")


def _value(kind: str, value: object) -> str:
    if kind == "t":
        return "<t />"
    if kind == "d":
        return "<d></d>"
    if kind == "i":
        return f"<i>{int(value)}</i>"
    if kind == "r":
        return f"<r>{float(value):.6f}</r>"
    return f"<s>{escape(str(value))}</s>"


def level_dict(name: str, author: str, objects: Iterable[tuple[int, int, int]]) -> list[tuple[str, str, object]]:
    """The 43 keys GD 2.2081 wrote for a level, in the order it wrote them.

    Key meanings are not established here; only `k2` (name), `k4` (objects), `k5` (author) and
    `kCEK` (object class) are read as what they obviously are. Everything else is a default
    whose correctness a run has to confirm.
    """
    object_string = level_string(objects)
    return [
        ("kCEK", "i", 4),
        ("k18", "i", 0),
        ("k36", "i", 0),
        ("k23", "i", 0),
        ("k2", "s", name),
        ("k4", "s", encode_level_string(object_string)),
        ("k5", "s", author),
        ("k95", "i", 0),
        ("k101", "s", ""),
        ("k11", "i", 0),
        ("k13", "t", None),
        ("k21", "i", 2),
        ("k16", "i", 0),
        ("k80", "i", 0),
        ("k81", "i", 0),
        ("k72", "t", None),
        ("k27", "i", 0),
        ("k42", "i", 0),
        ("k45", "i", 0),
        ("k50", "i", 0),
        ("k47", "t", None),
        ("k48", "i", 0),
        ("k104", "s", ""),
        ("k106", "i", 0),
        ("kI1", "r", 0.0),
        ("kI2", "r", 0.0),
        ("kI3", "r", 0.0),
        ("kI7", "i", 0),
        ("kI6", "d", None),
        *[(str(slot), "s", "") for slot in range(14)],
    ]


def render_level(name: str, author: str, objects: Iterable[tuple[int, int, int]]) -> bytes:
    """The fixture XML: GD's `LLM_01` wrapper around exactly one level."""
    body = ["<k>LLM_01</k><d><k>_isArr</k><t />", "<k>k_0</k><d>"]
    for key, kind, value in level_dict(name, author, objects):
        body.append(f"<k>{escape(key)}</k>")
        body.append(_value(kind, value))
    body.append("</d></d>")
    return (PLIST_OPEN + "<dict>" + "".join(body) + "</dict>" + PLIST_CLOSE).encode("utf-8")


DEFAULT_OBJECTS: tuple[tuple[int, int, int], ...] = (
    # Coordinates are 30-pixel grid units, and the player covers roughly 10 units per second,
    # so these are ~4.8 s, ~9.6 s and ~14.5 s into the run. The first version of this fixture
    # used x = 150/300/600 - about 14.5 s before the first obstacle - and the level looked
    # empty for the whole of a short session, which is a fixture defect, not a loading failure.
    # Keep the first obstacle inside the first few seconds so "is anything there" is visible.
    (1, 50, 15),
    (1, 100, 15),
    (1, 150, 15),
)


def install(xml: bytes, save_dir: Path, backup_dir: Path) -> Path:
    """Install `xml` through `save_sandbox`, so a fixture install is never weaker than a backup.

    This used to write `CCLocalLevels.dat` directly behind a `--force` flag and a `.dat.bak`
    copy, which bypassed every invariant the sandbox enforces (process guard, digest-verified
    backup, atomic write, read-back). There is now one implementation of "install a save".
    """
    return save_sandbox.install_xml(xml, save_dir, backup_dir)


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", metavar="GMD", help="write the canonical XML fixture here")
    parser.add_argument("--install", metavar="SAVE_DIR",
                        help="write CCLocalLevels.dat here (needs --backup-dir)")
    parser.add_argument("--backup-dir", metavar="DIR",
                        help="a verified save_sandbox backup that must exist before installing")
    parser.add_argument("--name", default="safe-mode-smoke fixture")
    parser.add_argument("--author", default="megahack-research")
    args = parser.parse_args(argv)
    if not args.out and not args.install:
        parser.error("nothing to do: pass --out and/or --install")
    if args.install and not args.backup_dir:
        parser.error(
            "--install needs --backup-dir: run save_sandbox.py --backup first, so the save is "
            "recoverable"
        )

    xml = render_level(args.name, args.author, DEFAULT_OBJECTS)
    if args.out:
        path = Path(args.out)
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(xml)
        print(f"wrote {path} ({len(xml)} bytes)")
    if args.install:
        target = install(xml, Path(args.install), Path(args.backup_dir))
        print(f"installed {target} ({target.stat().st_size} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
