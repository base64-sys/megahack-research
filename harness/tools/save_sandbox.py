"""Backup, install and restore the GD save directory that a runtime-validation run touches.

**This is not isolation, and the tool says so.** The `a` (Steam) and `runtime511` Geode profiles
share one GD save directory: `libcocos2d.dll` builds the writable path from
`SHELL32!SHGetFolderPathA`, so `%LOCALAPPDATA%` cannot be redirected for the child process
(notes/gd-save-format.md, "The isolation this exposed"). What this tool does instead is make the
mutation reversible and the risk explicit: nothing is overwritten until a digest-verified backup
of the current bytes exists, and the restore verifies again before writing back.

True isolation still needs a separate Windows account or a VM. Until then every run that
installs a fixture is *controlled mutation of a live save*, and the trace's
`profile.isolation: disposable` claim is not supported by this tool.

    python harness/tools/save_sandbox.py --backup  --save-dir <dir> --backup-dir <dir>
    python harness/tools/save_sandbox.py --install --save-dir <dir> --backup-dir <dir> --fixture <gmd>
    python harness/tools/save_sandbox.py --status  --save-dir <dir> --backup-dir <dir>
    python harness/tools/save_sandbox.py --restore --save-dir <dir> --backup-dir <dir>

`--save-dir` is GD's own save directory (`...\\GeometryDash`), never the profile directory.
"""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import shutil
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
_spec = importlib.util.spec_from_file_location("gd_save", HERE / "gd_save.py")
gd_save = importlib.util.module_from_spec(_spec)
assert _spec.loader is not None
_spec.loader.exec_module(gd_save)

# The files GD rewrites during a session that touches a level. Listed rather than globbed so a
# backup cannot silently grow to include user media, and so a missing file is visible.
SAVE_FILES = (
    "CCLocalLevels.dat",
    "CCLocalLevels2.dat",
    "CCGameManager.dat",
    "CCGameManager2.dat",
)

RECORD = "backup.json"


class SandboxError(RuntimeError):
    pass


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def snapshot(save_dir: Path) -> dict[str, dict[str, object]]:
    files: dict[str, dict[str, object]] = {}
    for name in SAVE_FILES:
        path = save_dir / name
        if path.is_file():
            files[name] = {"sha256": digest(path), "size": path.stat().st_size}
    return files


def backup(save_dir: Path, backup_dir: Path) -> dict[str, dict[str, object]]:
    if not save_dir.is_dir():
        raise SandboxError(f"{save_dir} is not a directory")
    backup_dir.mkdir(parents=True, exist_ok=True)
    if (backup_dir / RECORD).exists():
        raise SandboxError(
            f"{backup_dir} already holds a backup; refusing to overwrite it with a possibly "
            "already-contaminated save (restore or move it first)"
        )
    files = snapshot(save_dir)
    if not files:
        raise SandboxError(f"no save files found in {save_dir}")
    for name in files:
        shutil.copy2(save_dir / name, backup_dir / name)
    (backup_dir / RECORD).write_text(json.dumps({"files": files}, indent=2), encoding="utf-8")
    return files


def read_record(backup_dir: Path) -> dict[str, dict[str, object]]:
    record = backup_dir / RECORD
    if not record.is_file():
        raise SandboxError(f"no {RECORD} in {backup_dir}: run --backup before touching a save")
    return json.loads(record.read_text(encoding="utf-8"))["files"]


def verify_backup(backup_dir: Path) -> dict[str, dict[str, object]]:
    """The backup's own copies must still match their recorded digests."""
    files = read_record(backup_dir)
    for name, facts in files.items():
        path = backup_dir / name
        if not path.is_file() or digest(path) != facts["sha256"]:
            raise SandboxError(f"backup copy of {name} is missing or altered")
    return files


def verify_live_matches_backup(save_dir: Path, backup_dir: Path) -> None:
    """Refuse to overwrite a save that is not the one we backed up."""
    recorded = verify_backup(backup_dir)
    current = snapshot(save_dir)
    for name, facts in recorded.items():
        if name not in current:
            raise SandboxError(f"{name} was in the backup but is gone from {save_dir}")
        if current[name]["sha256"] != facts["sha256"]:
            raise SandboxError(
                f"{name} has changed since the backup (the game ran); back it up again "
                "rather than restoring over a save nobody recorded"
            )


def install(save_dir: Path, backup_dir: Path, fixture: Path) -> Path:
    verify_live_matches_backup(save_dir, backup_dir)
    xml = fixture.read_bytes()
    if not xml.startswith(b"<?xml"):
        raise SandboxError(f"{fixture} is not the plist XML fixture")
    target = save_dir / "CCLocalLevels.dat"
    target.write_bytes(gd_save.encode(xml))
    return target


def restore(save_dir: Path, backup_dir: Path) -> list[str]:
    files = verify_backup(backup_dir)
    restored: list[str] = []
    for name in files:
        shutil.copy2(backup_dir / name, save_dir / name)
        restored.append(name)
    for name, facts in files.items():
        if digest(save_dir / name) != facts["sha256"]:
            raise SandboxError(f"{name} did not restore to its recorded digest")
    return restored


def status(save_dir: Path, backup_dir: Path) -> dict[str, object]:
    recorded = read_record(backup_dir)
    current = snapshot(save_dir)
    return {
        name: ("same" if current.get(name, {}).get("sha256") == facts["sha256"] else "differs")
        for name, facts in recorded.items()
    }


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument("--backup", action="store_true")
    action.add_argument("--install", action="store_true")
    action.add_argument("--restore", action="store_true")
    action.add_argument("--status", action="store_true")
    parser.add_argument("--save-dir", required=True, type=Path)
    parser.add_argument("--backup-dir", required=True, type=Path)
    parser.add_argument("--fixture", type=Path, help="the .gmd to install (with --install)")
    args = parser.parse_args(argv)

    try:
        if args.backup:
            files = backup(args.save_dir, args.backup_dir)
            print(f"backed up {len(files)} file(s) from {args.save_dir} to {args.backup_dir}")
            for name, facts in files.items():
                print(f"  {name} {facts['size']} bytes {facts['sha256']}")
        elif args.install:
            if args.fixture is None:
                parser.error("--install needs --fixture")
            target = install(args.save_dir, args.backup_dir, args.fixture)
            print(f"installed {args.fixture.name} as {target}")
            print("note: a live save was mutated; run --restore when the session ends")
        elif args.restore:
            names = restore(args.save_dir, args.backup_dir)
            print(f"restored {len(names)} file(s) into {args.save_dir}: {', '.join(names)}")
        else:
            for name, state in status(args.save_dir, args.backup_dir).items():
                print(f"{name}: {state}")
        return 0
    except (OSError, SandboxError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
