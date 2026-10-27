#!/usr/bin/env python3
"""Tests for harness/tools/preflight.py.

Run: python harness/tools/test_preflight.py
"""

from __future__ import annotations

import hashlib
import io
import contextlib
import json
import struct
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import preflight


def make_pe(path: Path, image_base: int = 0x180000000) -> None:
    """A PE32+ image with headers only: enough for the header reader, nothing else."""
    data = bytearray(0x200)
    data[0:2] = b"MZ"
    struct.pack_into("<I", data, 0x3C, 0x40)
    data[0x40:0x44] = b"PE\0\0"

    coff = 0x44
    # machine, sections, timestamp, symbol table, symbols, optional size, characteristics
    struct.pack_into("<HHIIIHH", data, coff, 0x8664, 1, 0, 0, 0, 0x70, 0x0022)

    optional = coff + 20
    struct.pack_into("<H", data, optional, 0x20B)  # PE32+
    struct.pack_into("<Q", data, optional + 24, image_base)
    struct.pack_into("<I", data, optional + 32, 0x1000)  # section alignment
    struct.pack_into("<I", data, optional + 56, 0x1000)  # size of image

    # pe_rva refuses an image with no sections, so give it one header-only .text.
    table = optional + 0x70
    struct.pack_into("<8sIIII", data, table, b".text", 0x10, 0x1000, 0, 0)
    struct.pack_into("<I", data, table + 36, 0x60000020)
    path.write_bytes(bytes(data))


def test_sha256_matches_hashlib() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        blob = Path(tmp) / "blob.bin"
        payload = bytes(range(256)) * 8192  # spans more than one read chunk
        blob.write_bytes(payload)
        assert preflight.sha256_file(blob) == hashlib.sha256(payload).hexdigest()


def test_image_base_reads_the_header() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        pe = Path(tmp) / "image.dll"
        make_pe(pe, image_base=0x180000000)
        assert preflight.image_base_hex(pe) == "0x180000000"


def test_sidecar_is_sha256sum_form() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        trace = Path(tmp) / "run.jsonl"
        trace.write_bytes(b"{}\n")  # bytes: text mode would rewrite LF as CRLF
        sidecar = preflight.write_sidecar(trace)
        assert sidecar.name == "run.jsonl.sha256"
        digest, name = sidecar.read_text(encoding="utf-8").rstrip("\n").split("  ")
        assert name == "run.jsonl"
        assert digest == hashlib.sha256(b"{}\n").hexdigest()


def test_profile_refusals() -> None:
    for profile, gd in [
        ("a", "C:/Users/x/GD/GeometryDash.exe"),
        ("runtime511", "C:/Program Files (x86)/Steam/steamapps/common/Geometry Dash/GeometryDash.exe"),
        ("", "C:/Users/x/GD/GeometryDash.exe"),
    ]:
        try:
            preflight.check_profile(profile, gd)
        except preflight.PreflightError:
            continue
        raise AssertionError(f"profile {profile!r} at {gd!r} should be refused")
    preflight.check_profile("runtime511", "C:/Users/x/GD-Runtime-511/GeometryDash.exe")


def test_one_target_rule() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        one = Path(tmp) / "one.dll"
        one.write_bytes(b"MZ")
        assert preflight.check_targets([one]) == one
        for targets in ([], [one, one]):
            try:
                preflight.check_targets(targets)
            except preflight.PreflightError:
                continue
            raise AssertionError(f"{len(targets)} targets should be refused")
        try:
            preflight.check_targets([Path(tmp) / "missing.dll"])
        except preflight.PreflightError:
            pass
        else:
            raise AssertionError("a missing artifact should be refused")


def test_fragment_uses_the_contract_field_names() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        target = Path(tmp) / "absolllute.hackmega_original.dll"
        gd = Path(tmp) / "GeometryDash.exe"
        geode = Path(tmp) / "Geode.dll"
        harness = Path(tmp) / "megahack.trace.geode"
        for path in (target, gd, geode, harness):
            make_pe(path)
        fragment = preflight.build_fragment(
            target=target, gd=gd, geode=geode, harness=harness,
            gd_version="2.2081", geode_version="5.11.2",
        )
        expected = {path.name: hashlib.sha256(path.read_bytes()).hexdigest()
                    for path in (gd, geode, harness)}
        unknown = preflight.build_fragment(
            target=target, gd=gd, geode=geode, harness=harness,
            gd_version=None, geode_version=None,
        )

    assert set(fragment) == {"target", "environment"}
    assert set(fragment["target"]) == {"role", "artifact", "sha256", "size", "image_base"}
    assert fragment["target"]["role"] == "release"
    assert fragment["target"]["image_base"] == "0x180000000"
    assert fragment["target"]["size"] == 0x200

    environment = fragment["environment"]
    assert set(environment) == {"os", "arch", "gd", "geode", "harness"}
    for key in ("gd", "geode"):
        assert set(environment[key]) == {"version", "sha256"}
        assert environment[key]["sha256"] == expected[
            "GeometryDash.exe" if key == "gd" else "Geode.dll"
        ]
    assert set(environment["harness"]) == {"id", "version", "sha256"}
    assert environment["harness"]["id"] == "megahack.trace"
    assert environment["geode"]["version"] == "5.11.2"

    # Unknown versions stay null rather than being guessed.
    assert unknown["environment"]["gd"]["version"] is None
    assert unknown["environment"]["geode"]["version"] is None


def test_context_carries_the_manifest_and_repetition() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        target = Path(tmp) / "target.dll"
        gd = Path(tmp) / "GeometryDash.exe"
        geode = Path(tmp) / "Geode.dll"
        harness = Path(tmp) / "megahack.trace.geode"
        for path in (target, gd, geode, harness):
            make_pe(path)
        fragment = preflight.build_fragment(
            target=target, gd=gd, geode=geode, harness=harness,
            gd_version="2.2081", geode_version="5.11.2",
        )
    manifest = json.dumps({
        "schema": "mh-scenario/1",
        "id": "safe-mode-smoke",
        "settings": {"reset": "defaults", "tags": []},
        "level": {"fixture": "f.gmd", "sha256": "5" * 64, "source": "generated"},
    }).encode("utf-8")

    context = preflight.build_context(fragment=fragment, scenario_bytes=manifest, repetition=1)
    assert context["scenario"]["id"] == "safe-mode-smoke"
    assert context["scenario"]["sha256"] == hashlib.sha256(manifest).hexdigest()
    assert context["repetition_index"] == 1
    assert context["settings"]["tags"] == []
    assert context["settings"]["snapshot_sha256"] is None
    assert context["level"] == {"fixture": "f.gmd", "sha256": "5" * 64, "source": "generated"}
    assert context["target"] == fragment["target"]
    assert context["environment"] == fragment["environment"]


def test_context_refuses_declared_tags_without_a_settings_file() -> None:
    """Declared tags must be observed, never copied from the manifest into the record."""
    fragment = {"target": {"role": "release"}, "environment": {}}
    manifest = json.dumps({
        "id": "safe-mode-smoke",
        "settings": {"reset": "defaults", "tags": [{"tag": "0x1E2", "label": "HACK_SAFE_MODE", "value": True}]},
        "level": {"fixture": "f.gmd", "sha256": "5" * 64, "source": "generated"},
    }).encode("utf-8")
    try:
        preflight.build_context(fragment=fragment, scenario_bytes=manifest, repetition=0)
    except preflight.PreflightError as error:
        assert "no --settings file" in str(error), error
    else:
        raise AssertionError("declared tags without a settings file should be refused")


def test_context_reads_tag_values_from_the_settings_file() -> None:
    fragment = {"target": {"role": "release"}, "environment": {}}
    manifest = json.dumps({
        "id": "safe-mode-smoke",
        "settings": {"reset": "defaults", "tags": [{"tag": "0x1E2", "label": "HACK_SAFE_MODE", "value": True}]},
        "level": {"fixture": "f.gmd", "sha256": "5" * 64, "source": "generated"},
    }).encode("utf-8")
    settings_bytes = json.dumps({
        "PROFILE": "DEFAULT",
        "PROFILES": {"DEFAULT": []},
        "V_BOOL": {"SAFETY/HACK_SAFE_MODE": True},
    }).encode("utf-8")
    settings = json.loads(settings_bytes)

    context = preflight.build_context(
        fragment=fragment, scenario_bytes=manifest, repetition=0,
        settings=settings, settings_bytes=settings_bytes,
        tag_keys={"0X1E2": "SAFETY/HACK_SAFE_MODE"},
    )
    assert context["settings"]["tags"] == [{
        "tag": "0X1E2", "label": "HACK_SAFE_MODE", "value": True, "source": "settings",
    }]
    assert context["settings"]["snapshot_sha256"] == hashlib.sha256(settings_bytes).hexdigest()


def test_context_refuses_a_tag_that_disagrees_with_the_file() -> None:
    fragment = {"target": {}, "environment": {}}
    manifest = json.dumps({
        "id": "s",
        "settings": {"tags": [{"tag": "0x1E2", "label": "HACK_SAFE_MODE", "value": True}]},
        "level": {"sha256": "5" * 64},
    }).encode("utf-8")
    settings = {"V_BOOL": {"SAFETY/HACK_SAFE_MODE": False}}

    for tag_keys, message in [
        ({"0X1E2": "SAFETY/HACK_SAFE_MODE"}, "declares"),
        ({"0X1E2": "SAFETY/MISSING"}, "not present"),
        ({}, "no stored key"),
    ]:
        try:
            preflight.build_context(
                fragment=fragment, scenario_bytes=manifest, repetition=0,
                settings=settings, settings_bytes=b"{}", tag_keys=tag_keys,
            )
        except preflight.PreflightError as error:
            assert message in str(error), (message, error)
        else:
            raise AssertionError(f"{message!r} case should be refused")


def test_settings_key_in_two_groups_is_refused() -> None:
    settings = {"V_BOOL": {"K": True}, "V_INT": {"K": 1}}
    try:
        preflight.settings_value(settings, "K")
    except preflight.PreflightError as error:
        assert "more than one value group" in str(error)
    else:
        raise AssertionError("a key in two groups should be refused")


def test_cli_refuses_a_live_profile_and_writes_a_fragment() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        target = Path(tmp) / "target.dll"
        gd = Path(tmp) / "GeometryDash.exe"
        geode = Path(tmp) / "Geode.dll"
        harness = Path(tmp) / "megahack.trace.geode"
        for path in (target, gd, geode, harness):
            make_pe(path)

        with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
            refused = preflight.main([
                "--target", str(target), "--gd", str(gd), "--geode", str(geode),
                "--harness", str(harness), "--profile", "a",
            ])
        assert refused == 1, refused

        out = Path(tmp) / "fragment.json"
        with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
            ok = preflight.main([
                "--target", str(target), "--gd", str(gd), "--geode", str(geode),
                "--harness", str(harness), "--profile", "runtime511",
                "--gd-version", "2.2081", "--geode-version", "5.11.2", "--out", str(out),
            ])
        assert ok == 0, ok
        assert json.loads(out.read_text(encoding="utf-8"))["target"]["artifact"] == "target.dll"


if __name__ == "__main__":
    test_sha256_matches_hashlib()
    test_image_base_reads_the_header()
    test_sidecar_is_sha256sum_form()
    test_profile_refusals()
    test_one_target_rule()
    test_fragment_uses_the_contract_field_names()
    test_context_carries_the_manifest_and_repetition()
    test_context_refuses_declared_tags_without_a_settings_file()
    test_context_reads_tag_values_from_the_settings_file()
    test_context_refuses_a_tag_that_disagrees_with_the_file()
    test_settings_key_in_two_groups_is_refused()
    test_cli_refuses_a_live_profile_and_writes_a_fragment()
    print("all preflight tests passed")

