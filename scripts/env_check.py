#!/usr/bin/env python3
"""Report whether this machine can run the repository's analysis and build steps.

The repository deliberately has no install step: tracked tools are dependency-free,
the sample set is untracked, and the decompiled tree is regenerated on demand. That
makes "is this checkout ready?" a recurring question with a different answer on a
fresh clone, a research machine and a build-only machine.

This script answers it in one pass, without importing anything outside the standard
library and without requiring ``samples/`` to exist:

* interpreter version;
* optional analysis packages, and the vendored fallback under ``samples/``;
* the ground-truth DLL's size and SHA-256, checked against ``notes/provenance.md``;
* the generated indexes under ``recovered/functions/``;
* Ghidra and the compilers used by ``reconstruction/``.

A fresh clone is a supported state: a missing sample set or index tree is a warning,
not a failure. ``--strict`` turns warnings into a non-zero exit for a machine that is
supposed to be fully provisioned.

Usage (repository root)::

    python3 scripts/env_check.py
    python3 scripts/env_check.py --json
    python3 scripts/env_check.py --strict

Exit code 1 on a failed check, or on a warning under ``--strict``.
"""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import os
import re
import shutil
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SAMPLE_DIR = ROOT / "samples" / "absolllute.hackmega"
GROUND_TRUTH = SAMPLE_DIR / "absolllute.hackmega_original.dll"
VENDORED = SAMPLE_DIR / ".analysis_tools"
PROVENANCE = ROOT / "notes" / "provenance.md"
INDEX_DIR = ROOT / "recovered" / "functions"

INDEX_FILES = (
    "_index.tsv",
    "_xrefs.tsv",
    "_settings.tsv",
    "_strings.tsv",
    "_callers.tsv",
    "_calls.tsv",
    "_rtti.tsv",
    "_rtti_hierarchy.tsv",
    "_init_table.tsv",
    "_host_methods.tsv",
    "_hook_sites.tsv",
    "_wrappers.tsv",
)
PACKAGES = ("capstone", "pefile", "keystone")
BUILD_TOOLS = ("cmake", "ninja", "g++", "javac")

PASS = "pass"
WARN = "warn"
FAIL = "fail"


def record(checks: list[dict[str, str]], name: str, status: str, detail: str) -> None:
    checks.append({"name": name, "status": status, "detail": detail})


def digest(path: Path) -> str:
    value = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            value.update(block)
    return value.hexdigest()


def expected_digest() -> tuple[int, str] | None:
    """Read the baseline row from notes/provenance.md, if it is present."""
    if not PROVENANCE.is_file():
        return None
    text = PROVENANCE.read_text(encoding="utf-8", errors="replace")
    match = re.search(
        r"`absolllute\.hackmega_original\.dll`\s*\|\s*([\d,]+)\s*\|\s*`([0-9a-fA-F]{64})`",
        text,
    )
    if not match:
        return None
    return int(match.group(1).replace(",", "")), match.group(2).lower()


def check_python(checks: list[dict[str, str]]) -> None:
    version = ".".join(str(part) for part in sys.version_info[:3])
    if sys.version_info >= (3, 10):
        record(checks, "python", PASS, f"{version} ({sys.executable})")
    else:
        record(checks, "python", FAIL, f"{version} is older than 3.10 ({sys.executable})")


def check_packages(checks: list[dict[str, str]]) -> None:
    missing = [name for name in PACKAGES if importlib.util.find_spec(name) is None]
    if not missing:
        record(checks, "analysis packages", PASS, ", ".join(PACKAGES))
        return
    if VENDORED.is_dir():
        record(
            checks,
            "analysis packages",
            PASS,
            f"missing from this interpreter: {', '.join(missing)}; vendored copy at "
            f"{VENDORED.relative_to(ROOT)} covers sample tools run from the mod root "
            "(tracked tools need no packages)",
        )
    else:
        record(
            checks,
            "analysis packages",
            WARN,
            f"missing: {', '.join(missing)}; no vendored copy under samples/ "
            "(tracked tools do not need them)",
        )


def check_sample(checks: list[dict[str, str]]) -> None:
    if not GROUND_TRUTH.is_file():
        record(
            checks,
            "ground truth",
            WARN,
            "samples/ is absent; fresh-clone work is supported but binary evidence "
            "and the sample-only tools are unavailable",
        )
        return
    expected = expected_digest()
    size = GROUND_TRUTH.stat().st_size
    if expected is None:
        record(
            checks,
            "ground truth",
            WARN,
            f"{GROUND_TRUTH.name} is {size:,} bytes, but notes/provenance.md has no "
            "parseable baseline row",
        )
        return
    want_size, want_hash = expected
    got_hash = digest(GROUND_TRUTH)
    if size == want_size and got_hash == want_hash:
        record(checks, "ground truth", PASS, f"{GROUND_TRUTH.name} {size:,} bytes {got_hash[:16]}...")
    else:
        record(
            checks,
            "ground truth",
            FAIL,
            f"{GROUND_TRUTH.name} is {size:,} bytes / {got_hash}, expected "
            f"{want_size:,} / {want_hash} from notes/provenance.md",
        )


def check_indexes(checks: list[dict[str, str]]) -> None:
    present = []
    for name in INDEX_FILES:
        path = INDEX_DIR / name
        if not path.is_file():
            continue
        lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
        rows = sum(1 for line in lines if line.strip() and not line.startswith("#"))
        present.append((name, rows))
    if not present:
        record(
            checks,
            "recovered indexes",
            WARN,
            "absent; regenerate with <python> scripts/decompile_all.py "
            "--keep-project --reuse-project",
        )
        return
    detail = ", ".join(f"{name}={rows}" for name, rows in present)
    if len(present) < len(INDEX_FILES):
        record(checks, "recovered indexes", WARN, f"partial: {detail}")
    else:
        record(checks, "recovered indexes", PASS, detail)


def find_ghidra(explicit: str | None) -> Path | None:
    candidates: list[Path] = []
    if explicit:
        candidates.append(Path(explicit).expanduser())
    if os.environ.get("GHIDRA_HOME"):
        candidates.append(Path(os.environ["GHIDRA_HOME"]).expanduser())
    downloads = Path.home() / "Downloads"
    if downloads.is_dir():
        candidates.extend(sorted(downloads.glob("ghidra_*"), reverse=True))
    for candidate in candidates:
        # A Ghidra distribution is sometimes unpacked one level down, so accept both
        # <dir>/support/analyzeHeadless.bat and <dir>/<product>/support/analyzeHeadless.bat.
        for root in (candidate, *sorted(candidate.glob("ghidra_*"))):
            if (root / "support" / "analyzeHeadless.bat").is_file():
                return root
    return None


def check_ghidra(checks: list[dict[str, str]], explicit: str | None) -> None:
    found = find_ghidra(explicit)
    if found:
        record(checks, "ghidra", PASS, str(found))
    else:
        record(
            checks,
            "ghidra",
            WARN,
            "no analyzeHeadless.bat found; pass --ghidra or set GHIDRA_HOME "
            "(only needed to regenerate recovered/)",
        )


def check_build(checks: list[dict[str, str]]) -> None:
    missing = [name for name in BUILD_TOOLS if shutil.which(name) is None]
    if missing:
        record(checks, "build tools", WARN, f"missing from PATH: {', '.join(missing)}")
    else:
        record(checks, "build tools", PASS, ", ".join(BUILD_TOOLS))
    test_binaries = (
        ROOT / "reconstruction" / "tests" / "test_reconstruction.exe",
        ROOT / "reconstruction" / "build" / "tests" / "test_reconstruction.exe",
        ROOT / "reconstruction" / "build" / "test_reconstruction.exe",
    )
    test_binary = next((path for path in test_binaries if path.is_file()), None)
    if test_binary is not None:
        record(checks, "reconstruction test", PASS, str(test_binary.relative_to(ROOT)))
    else:
        record(
            checks,
            "reconstruction test",
            WARN,
            "not built; configure reconstruction/ with CMake and build the test target",
        )


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Report whether this checkout can run its analysis and build steps.")
    parser.add_argument("--json", action="store_true", help="emit machine-readable JSON")
    parser.add_argument("--strict", action="store_true", help="treat warnings as failures")
    parser.add_argument("--ghidra", metavar="DIR", help="Ghidra installation directory (default: GHIDRA_HOME or ~/Downloads/ghidra_*)")
    args = parser.parse_args(argv)

    checks: list[dict[str, str]] = []
    record(checks, "repository", PASS, str(ROOT))
    check_python(checks)
    check_packages(checks)
    check_sample(checks)
    check_indexes(checks)
    check_ghidra(checks, args.ghidra)
    check_build(checks)

    failed = sum(1 for check in checks if check["status"] == FAIL)
    warned = sum(1 for check in checks if check["status"] == WARN)
    passed = sum(1 for check in checks if check["status"] == PASS)
    summary = {"passed": passed, "warnings": warned, "failed": failed}

    if args.json:
        print(json.dumps({"checks": checks, "summary": summary}, indent=2))
    else:
        width = max(len(check["name"]) for check in checks)
        for check in checks:
            print(f"[{check['status']:<4}] {check['name']:<{width}}  {check['detail']}")
        print(f"\n{passed} passed, {warned} warning(s), {failed} failed")

    if failed or (args.strict and warned):
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
