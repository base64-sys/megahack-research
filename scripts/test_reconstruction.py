#!/usr/bin/env python3
"""Run the C++ core reconstruction test suite.

Compiles and executes reconstruction/tests/test_reconstruction.cpp against
reconstruction/src/ without leaving any compiled binary artifacts on disk.
"""

from __future__ import annotations

import pathlib
import shutil
import subprocess
import sys
import tempfile

REPO_ROOT = pathlib.Path(__file__).resolve().parent.parent
RECONSTRUCTION_DIR = REPO_ROOT / "reconstruction"


def find_compiler() -> str | None:
    compiler = shutil.which("g++")
    if compiler:
        return compiler
    winget_gcc = pathlib.Path(
        r"C:\Users\Robert_\AppData\Local\Microsoft\WinGet\Packages"
        r"\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin\g++.exe"
    )
    if winget_gcc.is_file():
        return str(winget_gcc)
    return None


def run_tests() -> int:
    compiler = find_compiler()
    if not compiler:
        print("SKIP: C++ compiler (g++) not found on system PATH.")
        return 0

    include_dir = RECONSTRUCTION_DIR / "include"
    src_files = [
        str(RECONSTRUCTION_DIR / "src" / "config.cpp"),
        str(RECONSTRUCTION_DIR / "src" / "extensions.cpp"),
        str(RECONSTRUCTION_DIR / "src" / "entry.cpp"),
        str(RECONSTRUCTION_DIR / "tests" / "test_reconstruction.cpp"),
    ]

    with tempfile.TemporaryDirectory() as tmp_dir:
        exe_path = pathlib.Path(tmp_dir) / "test_reconstruction.exe"
        cmd = [
            compiler,
            "-std=c++20",
            f"-I{include_dir}",
            *src_files,
            "-o",
            str(exe_path),
        ]

        print(f"Compiling reconstruction test suite with {compiler}...")
        res = subprocess.run(cmd, capture_output=True, text=True)
        if res.returncode != 0:
            print("Compilation failed:")
            print(res.stderr)
            return res.returncode

        print("Running reconstruction test suite...")
        res = subprocess.run([str(exe_path)], capture_output=True, text=True)
        print(res.stdout, end="")
        if res.returncode != 0:
            print(res.stderr, file=sys.stderr)
            return res.returncode

    return 0


if __name__ == "__main__":
    raise SystemExit(run_tests())
