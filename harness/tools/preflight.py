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
HARNESS_VERSION = "0.1.1"

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
    role: str = "release",
    expected_sha256: str | None = None,
) -> dict:
    """The `target` and `environment` objects of a meta record, as the contract spells them.

    `role` is the variant the caller says this is, and `expected_sha256` pins it: the contract
    allows only `release` and `reconstruction`, and a role claimed for the wrong bytes would be
    a false identity in an evidence record, so a mismatch refuses the run.
    """
    if role not in {"release", "reconstruction"}:
        raise PreflightError(f"target.role must be release or reconstruction, got {role!r}")
    target_path = Path(target)
    digest = sha256_file(target_path)
    if expected_sha256 is not None and digest != expected_sha256.lower():
        raise PreflightError(
            f"target sha256 is {digest} but the run pins {expected_sha256.lower()}"
        )
    return {
        "target": {
            "role": role,
            "artifact": target_path.name,
            "sha256": digest,
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


SETTINGS_VALUE_GROUPS = ("V_BOOL", "V_INT", "V_DECIMAL", "V_STRING", "V_BYTES")

_MISSING = object()


def read_settings(path: str | Path) -> dict:
    """`hackpro.json`: PROFILE, PROFILES, and one map per value type (notes/settings-contents.md)."""
    document = json.loads(Path(path).read_text(encoding="utf-8"))
    if not isinstance(document, dict):
        raise PreflightError(f"{path} is not a settings document")
    return document


def parse_tag_keys(pairs: list[str]) -> dict[str, tuple[str, str]]:
    """`<tag hex>=<key>` for a `V_*` value, `<tag hex>=flag:<profile name>` for the enabled list."""
    mapping: dict[str, tuple[str, str]] = {}
    for pair in pairs:
        tag, separator, key = pair.partition("=")
        if not separator or not tag.strip() or not key.strip():
            raise PreflightError(f"--tag-key expects <tag hex>=<stored key>, got {pair!r}")
        tag = tag.strip().upper()
        key = key.strip()
        if key.startswith("flag:"):
            mapping[tag] = ("flag", key[len("flag:"):])
        else:
            mapping[tag] = ("value", key)
    return mapping


def settings_value(settings: dict, key: str):
    """The observed value for one stored key, or _MISSING.

    The file groups values by type (`V_BOOL`, `V_INT`, ...), and the same key must not appear
    in two groups: a key with two values has no single interpretation.
    """
    found = _MISSING
    for group in SETTINGS_VALUE_GROUPS:
        values = settings.get(group)
        if not isinstance(values, dict) or key not in values:
            continue
        if found is not _MISSING:
            raise PreflightError(f"settings key {key!r} appears in more than one value group")
        found = values[key]
    return found


def settings_flag(settings: dict, name: str) -> bool:
    """Membership in the active profile's enabled list.

    Map B settings are not `V_*` values: `PROFILES[PROFILE]` is a list of enabled names in
    `SECTION/HACK_NAME` form (`CHEAT_SAFETY/HACK_SAFE_MODE_POPUP`, `BYPASS/HACK_ANTI_KICK`), and
    a name that is absent is off. `CHEAT_SAFETY` is a section in the release's own string table
    (`samples/.../absolllute.hackmega_original.dll` at file offset `0x533a01`).
    """
    profile = settings.get("PROFILE")
    profiles = settings.get("PROFILES")
    if not isinstance(profiles, dict) or profile not in profiles:
        raise PreflightError(f"the settings file has no enabled list for profile {profile!r}")
    return name in (profiles.get(profile) or [])


def resolve_tags(settings: dict, declared_tags: list[dict],
                 tag_keys: dict[str, tuple[str, str]]) -> list[dict]:
    """The semantic `settings.tags` list, with the value **observed** in the file.

    The manifest declares what the tag must be; the file says what it is. A mismatch refuses
    the run instead of writing the declared value into a record that claims to be an
    observation. `tag_keys` maps the tag hex to either a value key (stored in a `V_*` group) or
    `("flag", name)` for the profile's enabled list.
    """
    resolved = []
    for entry in declared_tags:
        tag = str(entry.get("tag", "")).upper()
        if not tag:
            raise PreflightError("a declared settings tag has no tag number")
        mapping = tag_keys.get(tag)
        if not mapping:
            raise PreflightError(
                f"tag {tag} has no stored key; pass --tag-key {tag}=<hackpro key> (or "
                f"{tag}=flag:<profile name>) so the value can be read instead of assumed"
            )
        kind, key = mapping
        if kind == "flag":
            value = settings_flag(settings, key)
        else:
            value = settings_value(settings, key)
            if value is _MISSING:
                raise PreflightError(f"tag {tag}: key {key!r} is not present in the settings file")
        if "value" in entry and entry["value"] != value:
            raise PreflightError(
                f"tag {tag}: the file has {value!r} but the scenario declares {entry['value']!r}"
            )
        resolved.append({
            "tag": tag,
            "label": entry.get("label"),
            "value": value,
            "source": "settings",
        })
    return resolved


def check_input_driver(manifest: dict) -> None:
    """Refuse a scenario that claims an input driver this project does not have.

    `operator` is the honest declaration for a human-driven run. `os` is allowed: an external
    tool can synthesise OS input, and the trace's input hooks are what prove it happened.
    `harness` is refused because nothing implements input injection through the game's input
    path - a manifest declaring it would record a claim nobody can keep.
    """
    driver = (manifest.get("input") or {}).get("driver")
    if driver == "harness":
        raise PreflightError(
            "input.driver is 'harness' but no input injection is implemented; declare "
            "'operator' for a human-driven run (or 'os' for an external tool)"
        )
    if driver == "os":
        print(
            "warning: input.driver is 'os'; the harness sends no input, so an external tool "
            "must, and only the pushButton/releaseButton records prove it did",
            file=sys.stderr,
        )


def build_context(*, fragment: dict, scenario_bytes: bytes, repetition: int,
                  settings: dict | None = None, tag_keys: dict[str, str] | None = None,
                  settings_bytes: bytes | None = None) -> dict:
    """The run context the observer splices into meta (harness/include/mh/context.hpp).

    Not a new schema: the meta body's own members plus repetition_index, which is the sixth
    fingerprint input and is never part of the meta record.
    """
    manifest = json.loads(scenario_bytes.decode("utf-8"))
    check_input_driver(manifest)
    settings_block = manifest.get("settings") or {}
    level = manifest.get("level") or {}
    declared_tags = settings_block.get("tags") or []

    tags: list[dict] = []
    snapshot = None
    if declared_tags:
        if settings is None:
            raise PreflightError(
                "the scenario declares settings tags but no --settings file was given; the "
                "tags would be a claim, not an observation"
            )
        tags = resolve_tags(settings, declared_tags, tag_keys or {})
        snapshot = hashlib.sha256(settings_bytes).hexdigest()

    return {
        "target": fragment["target"],
        "scenario": {
            "id": manifest.get("id"),
            "sha256": hashlib.sha256(scenario_bytes).hexdigest(),
        },
        "environment": fragment["environment"],
        "settings": {
            "reset": settings_block.get("reset"),
            "tags": tags,
            "snapshot_sha256": snapshot,
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
    parser.add_argument("--role", default="release", choices=("release", "reconstruction"),
                        help="which variant this artifact is")
    parser.add_argument("--expect-sha256", default=None,
                        help="pin the target's digest; a mismatch refuses the run")
    parser.add_argument("--out", help="write the fragment here instead of stdout")
    parser.add_argument("--scenario", help="scenario manifest; with --context, supplies its fields")
    parser.add_argument("--repetition", type=int, default=0, help="repetition_index for the context")
    parser.add_argument("--settings", help="hackpro.json to read the declared tags from")
    parser.add_argument("--tag-key", action="append", default=[],
                        help="<tag hex>=<stored key>, e.g. 0x1E2=SAFETY/HACK_SAFE_MODE")
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
            role=args.role,
            expected_sha256=args.expect_sha256,
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
                settings=read_settings(args.settings) if args.settings else None,
                settings_bytes=Path(args.settings).read_bytes() if args.settings else None,
                tag_keys=parse_tag_keys(args.tag_key),
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
