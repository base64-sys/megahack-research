"""Decompile the whole Mega Hack image into a readable pseudocode tree.

Produces one .c file per function plus an index and a symbol table, so the
mod can be read and navigated locally.

    python scripts/decompile_all.py
    python scripts/decompile_all.py --image samples/.../absolllute.hackmega_original.dll
    python scripts/decompile_all.py --functions 0x180072B60 0x180169EB0

IMPORTANT. The output is decompiler pseudocode for Absolllute's proprietary
binary. It is not compilable, it is not their source, and it must never be
committed. The output directory is gitignored and so is this script's default
target. See AGENTS.md.

The *script* is tracked and reproducible: rerun it and you get the same tree
for the same image and the same Ghidra version. Ghidra's own output is not
guaranteed identical across versions, so the Ghidra version is recorded in
_manifest.txt and a mismatch is reported.
"""

from __future__ import annotations

import argparse
import json
import os
import pathlib
import shutil
import subprocess
import sys
import time

REPO = pathlib.Path(__file__).resolve().parent.parent
DEFAULT_IMAGE = REPO / "samples" / "absolllute.hackmega" / "absolllute.hackmega_original.dll"
DEFAULT_OUT = REPO / "recovered" / "functions"
DEFAULT_PROJECT = REPO / "recovered" / "ghidra-project"
SCRIPT_DIR = REPO / "scripts" / "ghidra_scripts"

GHIDRA_CANDIDATES = [
    pathlib.Path(r"C:\Users\Robert_\Downloads\ghidra_12.1.2_PUBLIC_20260605"),
    pathlib.Path(os.environ.get("GHIDRA_INSTALL_DIR", "")),
]

HEADLESS_RELATIVE = ("support", "analyzeHeadless.bat")


def looks_like_ghidra(root: pathlib.Path) -> bool:
    return root.joinpath(*HEADLESS_RELATIVE).exists()


def find_ghidra() -> pathlib.Path:
    """Locate a Ghidra install root, descending one level for nested archives.

    Official Windows downloads unpack to <zipname>/ghidra_<version>_<build>/,
    so the directory the user points at is often one level above the real root.
    """
    override = os.environ.get("GHIDRA_INSTALL_DIR")
    candidates = [pathlib.Path(override)] if override else []
    candidates += [c for c in GHIDRA_CANDIDATES if c]
    for candidate in candidates:
        if not candidate or not candidate.is_dir():
            continue
        if looks_like_ghidra(candidate):
            return candidate
        for child in sorted(candidate.iterdir()):
            if child.is_dir() and looks_like_ghidra(child):
                return child
    raise SystemExit(
        "Could not find Ghidra. Set GHIDRA_INSTALL_DIR to the directory that\n"
        "contains support/analyzeHeadless.bat (or its parent)."
    )


def ghidra_version(root: pathlib.Path) -> str:
    version_file = root / "Ghidra" / "application.properties"
    if version_file.exists():
        for line in version_file.read_text(encoding="utf-8", errors="replace").splitlines():
            if line.startswith("application.version="):
                return line.split("=", 1)[1].strip()
    return "unknown"


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--image", type=pathlib.Path, default=DEFAULT_IMAGE)
    parser.add_argument("--out", type=pathlib.Path, default=DEFAULT_OUT)
    parser.add_argument("--project", type=pathlib.Path, default=DEFAULT_PROJECT)
    parser.add_argument(
        "--project-name", default="megahack",
        help="Ghidra project name; change it to keep a second image alongside",
    )
    parser.add_argument("--functions", nargs="+", type=lambda v: int(v, 0), default=None)
    parser.add_argument("--min-bytes", type=lambda v: int(v, 0), default=0)
    parser.add_argument("--timeout-min", type=int, default=180)
    parser.add_argument("--keep-project", action="store_true")
    parser.add_argument("--reuse-project", action="store_true",
                        help="skip import if the project already exists")
    args = parser.parse_args(argv)

    if not args.image.exists():
        raise SystemExit(f"image not found: {args.image}")

    ghidra = find_ghidra()
    version = ghidra_version(ghidra)
    print(f"ghidra     {ghidra}")
    print(f"version    {version}")
    print(f"image      {args.image}  ({args.image.stat().st_size} bytes)")

    args.out.mkdir(parents=True, exist_ok=True)
    args.project.mkdir(parents=True, exist_ok=True)

    headless = ghidra / "support" / "analyzeHeadless.bat"
    command = [str(headless), str(args.project), args.project_name]

    # Ghidra 12 refuses to load a script by absolute path unless its directory
    # is on the script path.
    command += ["-scriptPath", str(SCRIPT_DIR)]

    program_name = args.image.name
    project_marker = args.project / f"{args.project_name}.gpr"
    if args.reuse_project and project_marker.exists():
        # Analysis is the expensive step (~5 min on this image), so reuse it.
        print(f"reusing existing project {project_marker}")
        command += ["-process", program_name, "-noanalysis"]
    else:
        command += ["-import", str(args.image)]

    if args.functions:
        script = SCRIPT_DIR / "DumpDecompileFile.java"
        report = args.out.parent / "selected_functions.md"
        command += [
            "-postScript", str(script), str(report),
            *[f"{a:#x}" for a in args.functions],
        ]
        print(f"decompiling {len(args.functions)} selected functions -> {report}")
    else:
        script = SCRIPT_DIR / "ExportAllFunctions.java"
        command += [
            "-postScript", str(script), str(args.out),
            str(args.min_bytes),
        ]
        print(f"decompiling every function -> {args.out}")

    if not args.keep_project:
        # -import already runs analysis; this only controls whether the
        # Ghidra project survives afterwards.
        command += ["-deleteProject"]

    print()
    print(" ".join(command))
    print()
    started = time.time()
    completed = subprocess.run(command, capture_output=True, text=True, errors="replace")
    elapsed = time.time() - started

    log = args.out.parent / "decompile.log"
    log.write_text(
        f"command: {' '.join(command)}\n"
        f"ghidra: {version}\n"
        f"elapsed: {elapsed:.1f}s\nexit: {completed.returncode}\n\n"
        f"--- stdout ---\n{completed.stdout}\n\n--- stderr ---\n{completed.stderr}\n",
        encoding="utf-8",
    )

    print(f"exit {completed.returncode} in {elapsed:.0f}s; log at {log}")
    for line in completed.stdout.splitlines():
        if line.startswith("DONE") or "decompiled" in line:
            print("  " + line)

    files = sorted(args.out.glob("*.c")) if args.out.exists() else []
    manifest = {
        "image": str(args.image),
        "image_size": args.image.stat().st_size,
        "ghidra_version": version,
        "elapsed_seconds": round(elapsed, 1),
        "exit_code": completed.returncode,
        "function_files": len(files),
        "total_bytes": sum(f.stat().st_size for f in files),
    }
    (args.out / "_manifest.txt").write_text(
        "\n".join(f"{k}: {v}" for k, v in manifest.items()) + "\n", encoding="utf-8"
    )
    (args.out / "_manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")

    print()
    print(f"function files: {len(files)}")
    if files:
        print(f"total size    : {sum(f.stat().st_size for f in files) / 1e6:.1f} MB")
        print(f"output dir    : {args.out}")
    return completed.returncode


if __name__ == "__main__":
    raise SystemExit(main())