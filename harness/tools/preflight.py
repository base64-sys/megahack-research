#!/usr/bin/env python3
"""Preflight: collect the facts a trace meta needs before the game starts.

The observer cannot know these at load time (see `harness/OBSERVER-PROGRESS.md`, O2/O4): the
identity of the target artifact, the hashes of the runtime images, and which profile is
allowed to be touched. The runner owns them.

The output is not a new format. It is the exact `target` and `environment` objects of the
meta record in `notes/runtime-validation.md` (section "meta record"), so the observer's meta
and the runner's preflight share one spelling and there is no second dialect to keep in step.

Launching the game is deliberately absent: that is a user action, and the loop that wrote
this must never do it on its own.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path
from typing import Sequence

_SCRIPTS = Path(__file__).resolve().parents[2] / "scripts"
if str(_SCRIPTS) not in sys.path:
    sys.path.insert(0, str(_SCRIPTS))

import pe_rva  # noqa: E402

HARNESS_ID = "megahack.trace"
HARNESS_VERSION = "0.1.0"

# The default profile is the live Steam install. A run against it can write a player's save,
# so it is refused by name and, independently, by the path it points at.
LIVE_PROFILE = "a"
LIVE_PATH_MARKER = "steamapps"

CHUNK = 1 << 20


class PreflightError(Exception):
    """A run that must not start."""


def sha256_file(path: str | Path) -> str:
    digest = hashlib.sha256()
    with Path(path).open("rb") as handle:
        for chunk in iter(lambda: handle.read(CHUNK), b""):
            digest.update(chunk)
    return digest.hexdigest()


def image_base_hex(path: str | Path) -> str:
    """The image base from the PE header. Never derived by arithmetic from an RVA."""
    return f"0x{pe_rva.Image.from_path(path).image_base:X}"


def check_profile(profile: str, gd_path: str | Path) -> None:
    if not profile:
        raise PreflightError("a profile name is required; the runner never guesses one")
    if profile == LIVE_PROFILE:
        raise PreflightError(
            f"profile {profile!r} is the live Steam install; use a disposable profile"
        )
    if LIVE_PATH_MARKER in str(gd_path).lower():
        raise PreflightError(f"{gd_path} looks like a live Steam install")


def check_targets(targets: Sequence[str | Path]) -> Path:
    """The comparability gate compares one target. Zero or two is a configuration error."""
    if len(targets) != 1:
        raise PreflightError(f"exactly one target artifact is required, got {len(targets)}")
    target = Path(targets[0])
    if not target.is_file():
        raise PreflightError(f"target artifact not found: {target}")
    return target


def build_fragment(
    *,
    target: str | Path,
    gd: str | Path,
    geode: str | Path,
    harness: str | Path,
    gd_version: str | None,
    geode_version: str | None,
    os_name: str = "Windows",
    arch: str = "x86_64",
) -> dict:
    """The `target` and `environment` objects of a meta record, as the contract spells them."""
    target_path = Path(target)
    return {
        "target": {
            "role": "release",
            "artifact": target_path.name,
            "sha256": sha256_file(target_path),
            "size": target_path.stat().st_size,
            "image_base": image_base_hex(target_path),
        },
        "environment": {
            "os": os_name,
            "arch": arch,
            "gd": {"version": gd_version, "sha256": sha256_file(gd)},
            "geode": {"version": geode_version, "sha256": sha256_file(geode)},
            "harness": {
                "id": HARNESS_ID,
                "version": HARNESS_VERSION,
                "sha256": sha256_file(harness),
            },
        },
    }


def build_context(*, fragment: dict, scenario_bytes: bytes, repetition: int) -> dict:
    """The run context the observer splices into meta (harness/include/mh/context.hpp).

    Not a new schema: the meta body's own members plus repetition_index, which is the sixth
    fingerprint input and is never part of the meta record.
    """
    manifest = json.loads(scenario_bytes.decode("utf-8"))
    settings = manifest.get("settings") or {}
    level = manifest.get("level") or {}
    return {
        "target": fragment["target"],
        "scenario": {
            "id": manifest.get("id"),
            "sha256": hashlib.sha256(scenario_bytes).hexdigest(),
        },
        "environment": fragment["environment"],
        "settings": {
            "reset": settings.get("reset"),
            "tags": settings.get("tags") or [],
            "snapshot_sha256": None,
        },
        "level": {
            "fixture": level.get("fixture"),
            "sha256": level.get("sha256"),
            "source": level.get("source"),
        },
        "repetition_index": repetition,
    }


def write_sidecar(trace: str | Path) -> Path:
    """`<trace>.sha256` in sha256sum form, so the trace can be checked without this tool."""
    trace_path = Path(trace)
    digest = sha256_file(trace_path)
    sidecar = trace_path.with_name(trace_path.name + ".sha256")
    sidecar.write_text(f"{digest}  {trace_path.name}\n", encoding="utf-8")
    return sidecar


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--target", action="append", default=[],
                        help="target artifact; repeat to prove the one-target rule refuses it")
    parser.add_argument("--gd", help="GeometryDash.exe of the disposable profile")
    parser.add_argument("--geode", help="Geode.dll loaded by that profile")
    parser.add_argument("--harness", help="the observer module (megahack.trace.geode or .dll)")
    parser.add_argument("--gd-version", default=None, help="e.g. 2.2081; omitted means unknown")
    parser.add_argument("--geode-version", default=None, help="e.g. 5.11.2")
    parser.add_argument("--profile", default="", help="profile name; 'a' is refused")
    parser.add_argument("--out", help="write the fragment here instead of stdout")
    parser.add_argument("--scenario", help="scenario manifest; with --context, supplies its fields")
    parser.add_argument("--repetition", type=int, default=0, help="repetition_index for the context")
    parser.add_argument("--context", help="write the run context for this run here")
    parser.add_argument("--sidecar", help="write <trace>.sha256 for this trace file")
    args = parser.parse_args(argv)

    try:
        if args.sidecar:
            sidecar = write_sidecar(args.sidecar)
            print(f"sidecar: {sidecar}")
        if not (args.target or args.gd or args.geode or args.harness):
            return 0 if args.sidecar else 2

        if not args.gd or not args.geode or not args.harness:
            raise PreflightError("--gd, --geode and --harness are all required")
        check_profile(args.profile, args.gd)
        target = check_targets(args.target)
        fragment = build_fragment(
            target=target,
            gd=args.gd,
            geode=args.geode,
            harness=args.harness,
            gd_version=args.gd_version,
            geode_version=args.geode_version,
        )
        if args.context:
            if not args.scenario:
                raise PreflightError("--context needs --scenario")
            scenario_path = Path(args.scenario)
            if not scenario_path.is_file():
                raise PreflightError(f"scenario manifest not found: {scenario_path}")
            context = build_context(
                fragment=fragment,
                scenario_bytes=scenario_path.read_bytes(),
                repetition=args.repetition,
            )
            Path(args.context).write_text(
                json.dumps(context, indent=2, sort_keys=True) + "\n", encoding="utf-8"
            )
            print(f"context: {args.context}")
    except PreflightError as error:
        print(f"preflight refused: {error}", file=sys.stderr)
        return 1

    text = json.dumps(fragment, indent=2, sort_keys=True) + "\n"
    if args.out:
        Path(args.out).write_text(text, encoding="utf-8")
        print(f"fragment: {args.out}")
    else:
        sys.stdout.write(text)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
