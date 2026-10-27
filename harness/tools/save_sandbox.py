"""Backup, install and restore the GD save directory that a runtime-validation run touches.

**This is not isolation, and the tool says so.** The `a` (Steam) and `runtime511` Geode profiles
share one GD save directory: `libcocos2d.dll` builds the writable path from
`SHELL32!SHGetFolderPathA`, so `%LOCALAPPDATA%` cannot be redirected for the child process
(notes/gd-save-format.md, "The isolation this exposed"). What this tool does instead is make the
mutation reversible and the risk explicit.

The guarantees, in the order they matter:

1. Nothing is written while Geometry Dash is running. The check is by process image name, taken
   once at the start of the operation; it is a guard, not a lock (see the risks in
   notes/gd-save-format.md).
2. Nothing is overwritten until a digest-verified backup of the current bytes exists, and the
   backup's own copies are re-verified before every install and restore.
3. Every write is atomic per file: bytes go to a temporary file beside the target, are flushed
   and fsynced, then renamed over it. A crash or a refusal leaves the previous file intact.
4. An install reads back what it wrote and decodes it; a mismatch is rolled back to the
   backed-up bytes rather than left as a corrupt save.
5. A restore reports exactly which files were restored, which failed and which were never
   touched, and it is idempotent - re-running after the cause is fixed finishes the job.

**Files created after the backup are never deleted.** A file absent from the record is reported
as an *extra* and left alone: its provenance is unknown, GD may have written it, and it may be
the only surviving copy of something. Identify it before relaunching the game.

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
import ctypes
import hashlib
import importlib.util
import json
import os
import shutil
import sys
import uuid
from pathlib import Path
from typing import Iterable

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

# The process image a save-writing session runs as. Compared case-insensitively.
GAME_IMAGES = ("GeometryDash.exe",)


class SandboxError(RuntimeError):
    """A refused or failed operation, stated in terms of the files involved."""


class RestoreError(SandboxError):
    """A restore that could not complete, carrying exactly which file is which."""

    def __init__(self, restored: list[str], failed: list[tuple[str, str]],
                 untouched: list[str]) -> None:
        self.restored = list(restored)
        self.failed = list(failed)
        self.untouched = list(untouched)
        parts = []
        if self.failed:
            parts.append("failed: " + "; ".join(f"{name} ({why})" for name, why in self.failed))
        parts.append("restored: " + (", ".join(self.restored) or "none"))
        parts.append("untouched: " + (", ".join(self.untouched) or "none"))
        super().__init__("restore did not complete - " + "; ".join(parts))


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def digest_bytes(payload: bytes) -> str:
    return hashlib.sha256(payload).hexdigest()


# --- process guard -------------------------------------------------------------------------


def running_image_names() -> list[str]:
    """Every running process image name, via the Windows toolhelp snapshot.

    Returns `[]` off Windows, where this tool has nothing to guard. Enumeration failure is not
    silently treated as "not running": an unreadable process list cannot prove the game is
    closed, so it refuses.
    """
    if sys.platform != "win32":
        return []

    class PROCESSENTRY32W(ctypes.Structure):
        _fields_ = [
            ("dwSize", ctypes.c_uint32),
            ("cntUsage", ctypes.c_uint32),
            ("th32ProcessID", ctypes.c_uint32),
            ("th32DefaultHeapID", ctypes.c_void_p),
            ("th32ModuleID", ctypes.c_uint32),
            ("cntThreads", ctypes.c_uint32),
            ("th32ParentProcessID", ctypes.c_uint32),
            ("pcPriClassBase", ctypes.c_long),
            ("dwFlags", ctypes.c_uint32),
            ("szExeFile", ctypes.c_wchar * 260),
        ]

    kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel32.CreateToolhelp32Snapshot.restype = ctypes.c_void_p
    kernel32.CreateToolhelp32Snapshot.argtypes = [ctypes.c_uint32, ctypes.c_uint32]
    kernel32.Process32FirstW.argtypes = [ctypes.c_void_p, ctypes.POINTER(PROCESSENTRY32W)]
    kernel32.Process32NextW.argtypes = [ctypes.c_void_p, ctypes.POINTER(PROCESSENTRY32W)]
    kernel32.CloseHandle.argtypes = [ctypes.c_void_p]

    invalid = ctypes.c_void_p(-1).value
    snapshot = kernel32.CreateToolhelp32Snapshot(0x00000002, 0)  # TH32CS_SNAPPROCESS
    if snapshot is None or snapshot == invalid:
        raise SandboxError(
            "could not read the process list, so it cannot be shown that Geometry Dash is "
            "closed; refusing to touch a save"
        )
    names: list[str] = []
    try:
        entry = PROCESSENTRY32W()
        entry.dwSize = ctypes.sizeof(PROCESSENTRY32W)
        more = kernel32.Process32FirstW(snapshot, ctypes.byref(entry))
        while more:
            names.append(entry.szExeFile)
            more = kernel32.Process32NextW(snapshot, ctypes.byref(entry))
    finally:
        kernel32.CloseHandle(snapshot)
    return names


def game_processes(images: Iterable[str] | None = None) -> list[str]:
    """The watched game images that are running, sorted. `images` is an injection point."""
    names = running_image_names() if images is None else list(images)
    watched = {image.lower() for image in GAME_IMAGES}
    return sorted({name for name in names if name.lower() in watched})


def assert_game_not_running(images: Iterable[str] | None = None) -> None:
    hits = game_processes(images)
    if hits:
        raise SandboxError(
            f"Geometry Dash is running ({', '.join(hits)}); close it before backing up, "
            "installing or restoring a save"
        )


# --- atomic file operations ----------------------------------------------------------------


def _temporary_beside(target: Path) -> Path:
    return target.with_name(f"{target.name}.tmp-{os.getpid()}-{uuid.uuid4().hex[:8]}")


def _write_atomic(target: Path, payload: bytes) -> None:
    """Write beside the target, flush, fsync, then rename over it."""
    temporary = _temporary_beside(target)
    try:
        with open(temporary, "wb") as handle:
            handle.write(payload)
            handle.flush()
            os.fsync(handle.fileno())
        os.replace(temporary, target)
    finally:
        if temporary.exists():
            temporary.unlink()


def _copy_atomic(source: Path, target: Path) -> None:
    temporary = _temporary_beside(target)
    try:
        shutil.copy2(source, temporary)
        os.replace(temporary, target)
    finally:
        if temporary.exists():
            temporary.unlink()


# --- records -------------------------------------------------------------------------------


def snapshot(save_dir: Path) -> dict[str, dict[str, object]]:
    files: dict[str, dict[str, object]] = {}
    for name in SAVE_FILES:
        path = save_dir / name
        if path.is_file():
            files[name] = {"sha256": digest(path), "size": path.stat().st_size}
    return files


def read_record(backup_dir: Path) -> dict[str, dict[str, object]]:
    record = backup_dir / RECORD
    if not record.is_file():
        raise SandboxError(f"no {RECORD} in {backup_dir}: run --backup before touching a save")
    try:
        payload = json.loads(record.read_text(encoding="utf-8"))
    except (OSError, ValueError) as error:
        raise SandboxError(f"{record} is not readable JSON: {error}") from error
    files = payload.get("files") if isinstance(payload, dict) else None
    if not isinstance(files, dict) or not files:
        raise SandboxError(f"{record} has no 'files' mapping")
    for name, facts in files.items():
        if (not isinstance(facts, dict) or not isinstance(facts.get("sha256"), str)
                or not isinstance(facts.get("size"), int)):
            raise SandboxError(f"{record}: the entry for {name} is malformed")
    return files


def verify_backup(backup_dir: Path) -> dict[str, dict[str, object]]:
    """The backup's own copies must still match their recorded digests."""
    files = read_record(backup_dir)
    for name, facts in files.items():
        path = backup_dir / name
        if not path.is_file():
            raise SandboxError(f"backup copy of {name} is missing from {backup_dir}")
        if digest(path) != facts["sha256"]:
            raise SandboxError(f"backup copy of {name} has been altered since it was taken")
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


# --- operations ----------------------------------------------------------------------------


def backup(save_dir: Path, backup_dir: Path,
           images: Iterable[str] | None = None) -> dict[str, dict[str, object]]:
    assert_game_not_running(images)
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
    for name, facts in files.items():
        _copy_atomic(save_dir / name, backup_dir / name)
        if digest(backup_dir / name) != facts["sha256"]:
            raise SandboxError(
                f"the backup copy of {name} does not match the file it was taken from"
            )
    # The record is written last: a failed copy leaves a directory with no record, which every
    # other operation refuses, rather than a record that vouches for bytes it does not have.
    (backup_dir / RECORD).write_text(json.dumps({"files": files}, indent=2), encoding="utf-8")
    return files


def install_xml(xml: bytes, save_dir: Path, backup_dir: Path,
                images: Iterable[str] | None = None) -> Path:
    """Install `xml` as the levels save, against a verified backup. Atomic and read back."""
    assert_game_not_running(images)
    if not xml.startswith(b"<?xml"):
        raise SandboxError("the fixture is not the plist XML form")
    payload = gd_save.encode(xml)
    target = save_dir / "CCLocalLevels.dat"
    recorded = verify_backup(backup_dir)
    # Re-installing the same fixture is a no-op, so a retried install after a failure or an
    # interrupted session does not need a fresh backup.
    if target.is_file() and target.read_bytes() == payload:
        return target
    verify_live_matches_backup(save_dir, backup_dir)
    _write_atomic(target, payload)

    written = target.read_bytes()
    if written != payload:
        how = _roll_back(target, backup_dir, recorded)
        raise SandboxError(
            f"{target} did not verify after writing (wrote {digest_bytes(payload)}, read back "
            f"{digest_bytes(written)}){how}"
        )
    try:
        decoded = gd_save.decode(written)
    except gd_save.SaveFormatError as error:
        how = _roll_back(target, backup_dir, recorded)
        raise SandboxError(
            f"{target} does not decode after writing ({error}){how}"
        ) from error
    if decoded != xml:
        how = _roll_back(target, backup_dir, recorded)
        raise SandboxError(
            f"{target} decoded to different bytes than the fixture{how}"
        )
    return target


def _roll_back(target: Path, backup_dir: Path,
               recorded: dict[str, dict[str, object]]) -> str:
    """Put the backed-up bytes back after a failed install, if the backup has them."""
    if target.name not in recorded:
        return "; there was no backed-up copy of that file to restore"
    _copy_atomic(backup_dir / target.name, target)
    return "; the backed-up file was restored"


def install(save_dir: Path, backup_dir: Path, fixture: Path,
            images: Iterable[str] | None = None) -> Path:
    if not fixture.is_file():
        raise SandboxError(f"{fixture} is not a file")
    return install_xml(fixture.read_bytes(), save_dir, backup_dir, images)


def restore(save_dir: Path, backup_dir: Path,
            images: Iterable[str] | None = None) -> list[str]:
    assert_game_not_running(images)
    files = verify_backup(backup_dir)
    restored: list[str] = []
    failed: list[tuple[str, str]] = []
    untouched: list[str] = []
    for name, facts in files.items():
        try:
            _copy_atomic(backup_dir / name, save_dir / name)
        except OSError as error:
            failed.append((name, str(error)))
            untouched.append(name)
            continue
        if digest(save_dir / name) != facts["sha256"]:
            failed.append((name, "restored bytes do not match the recorded digest"))
            continue
        restored.append(name)
    if failed:
        raise RestoreError(restored, failed, untouched)
    return restored


def status(save_dir: Path, backup_dir: Path) -> dict[str, object]:
    """Per-file state against the record, plus files that appeared after the backup.

    `missing` and `differs` are separate answers: a save that vanished is not a save that
    changed, and the recovery differs.
    """
    recorded = read_record(backup_dir)
    current = snapshot(save_dir)
    states: dict[str, str] = {}
    for name, facts in recorded.items():
        if name not in current:
            states[name] = "missing"
        elif current[name]["sha256"] == facts["sha256"]:
            states[name] = "same"
        else:
            states[name] = "differs"
    extras = sorted(name for name in current if name not in recorded)
    return {"files": states, "extras": extras}


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
            report = status(args.save_dir, args.backup_dir)
            for name, state in report["files"].items():
                print(f"  {name}: {state}")
            _print_extras(report["extras"])
        else:
            if game_processes():
                print("warning: Geometry Dash is running; the save may be mid-write")
            report = status(args.save_dir, args.backup_dir)
            for name, state in report["files"].items():
                print(f"{name}: {state}")
            _print_extras(report["extras"])
        return 0
    except RestoreError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2
    except (OSError, SandboxError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2


def _print_extras(extras: list[str]) -> None:
    for name in extras:
        print(f"extra: {name} (appeared after the backup; left in place - identify it before "
              "relaunching the game)")


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
