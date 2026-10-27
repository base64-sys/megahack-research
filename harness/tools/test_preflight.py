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
import zipfile
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


def test_harness_version_comes_from_the_packaged_archive() -> None:
    """A meta record names the version inside the `.geode`, not a hand-kept constant.

    That constant drifted once: the 0.2.0 observer was described as 0.1.1 in a run context
    because nobody remembered to bump it with the package.
    """
    with tempfile.TemporaryDirectory() as tmp:
        target = Path(tmp) / "target.dll"
        gd = Path(tmp) / "GeometryDash.exe"
        geode = Path(tmp) / "Geode.dll"
        harness = Path(tmp) / "megahack.trace.geode"
        for path in (target, gd, geode):
            make_pe(path)
        with zipfile.ZipFile(harness, "w") as archive:
            archive.writestr("megahack.trace.dll", b"MZ")
            archive.writestr("mod.json", json.dumps({"version": "v0.9.9"}))
        fragment = preflight.build_fragment(
            target=target, gd=gd, geode=geode, harness=harness,
            gd_version="2.2081", geode_version="5.11.2",
        )

    assert fragment["environment"]["harness"]["version"] == "0.9.9"


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


def test_context_refuses_the_unimplemented_harness_input_driver() -> None:
    """`harness` claims injected input through the game, and nothing injects input."""
    fragment = {"target": {"role": "release"}, "environment": {}}
    manifest = json.dumps({
        "id": "safe-mode-smoke",
        "settings": {"reset": "defaults", "tags": []},
        "level": {"fixture": "f.gmd", "sha256": "5" * 64, "source": "generated"},
        "input": {"driver": "harness", "actions": []},
    }).encode("utf-8")
    try:
        preflight.build_context(fragment=fragment, scenario_bytes=manifest, repetition=0)
    except preflight.PreflightError as error:
        assert "no input injection is implemented" in str(error), error
    else:
        raise AssertionError("a harness-driven scenario should be refused")


def test_context_accepts_the_operator_input_driver() -> None:
    fragment = {"target": {"role": "release"}, "environment": {}}
    manifest = json.dumps({
        "id": "safe-mode-smoke",
        "settings": {"reset": "defaults", "tags": []},
        "level": {"fixture": "f.gmd", "sha256": "5" * 64, "source": "generated"},
        "input": {"driver": "operator", "actions": []},
    }).encode("utf-8")
    context = preflight.build_context(fragment=fragment, scenario_bytes=manifest, repetition=0)
    assert context["scenario"]["id"] == "safe-mode-smoke"


def test_context_warns_about_the_os_input_driver() -> None:
    fragment = {"target": {"role": "release"}, "environment": {}}
    manifest = json.dumps({
        "id": "safe-mode-smoke",
        "settings": {"reset": "defaults", "tags": []},
        "level": {"fixture": "f.gmd", "sha256": "5" * 64, "source": "generated"},
        "input": {"driver": "os", "actions": []},
    }).encode("utf-8")
    captured = io.StringIO()
    with contextlib.redirect_stderr(captured):
        preflight.build_context(fragment=fragment, scenario_bytes=manifest, repetition=0)
    assert "external tool" in captured.getvalue()


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
        tag_keys={"0X1E2": ("value", "SAFETY/HACK_SAFE_MODE")},
    )
    assert context["settings"]["tags"] == [{
        "tag": "0X1E2", "label": "HACK_SAFE_MODE", "value": True, "source": "settings",
    }]
    assert context["settings"]["snapshot_sha256"] == hashlib.sha256(settings_bytes).hexdigest()


def test_context_reads_a_map_b_flag_from_the_profile_list() -> None:
    """Map B: value is membership in PROFILES[PROFILE], not a V_* entry."""
    fragment = {"target": {}, "environment": {}}
    manifest = json.dumps({
        "id": "safe-mode-smoke",
        "settings": {"tags": [{"tag": "0x1E2", "label": "HACK_SAFE_MODE", "value": True}]},
        "level": {"sha256": "5" * 64},
    }).encode("utf-8")
    settings = {
        "PROFILE": "DEFAULT",
        "PROFILES": {"DEFAULT": ["CHEAT_SAFETY/HACK_SAFE_MODE", "BYPASS/HACK_ANTI_KICK"]},
    }
    keys = preflight.parse_tag_keys(["0x1E2=flag:CHEAT_SAFETY/HACK_SAFE_MODE"])
    context = preflight.build_context(
        fragment=fragment, scenario_bytes=manifest, repetition=0,
        settings=settings, settings_bytes=b"{}", tag_keys=keys,
    )
    assert context["settings"]["tags"][0]["value"] is True

    # Absent from the enabled list means off, and a scenario that declares true is refused.
    settings["PROFILES"]["DEFAULT"] = ["BYPASS/HACK_ANTI_KICK"]
    try:
        preflight.build_context(
            fragment=fragment, scenario_bytes=manifest, repetition=0,
            settings=settings, settings_bytes=b"{}", tag_keys=keys,
        )
    except preflight.PreflightError as error:
        assert "declares" in str(error), error
    else:
        raise AssertionError("an absent flag should be refused when the scenario wants it on")

    # A file without the profile cannot answer at all.
    try:
        preflight.settings_flag({"PROFILE": "MISSING", "PROFILES": {}}, "X")
    except preflight.PreflightError as error:
        assert "no enabled list" in str(error)
    else:
        raise AssertionError("an unknown profile should be refused")


def test_context_carries_a_declared_capture_window() -> None:
    """A window is the only form of `timing` the observer can enforce, so it is the only one sent."""
    fragment = {"target": {"role": "release"}, "environment": {}}
    manifest = json.dumps({
        "id": "no-input-smoke",
        "settings": {"reset": "defaults", "tags": []},
        "level": {"fixture": "f.gmd", "sha256": "5" * 64, "source": "generated"},
        "input": {"driver": "operator", "actions": []},
        "timing": {"window": {"anchor": "level_started", "max_frames": 600}, "stop_on": []},
    }).encode("utf-8")
    context = preflight.build_context(fragment=fragment, scenario_bytes=manifest, repetition=0)
    assert context["timing"] == {"window": {"anchor": "level_started", "max_frames": 600}}


def test_a_frame_count_without_an_anchor_is_not_carried() -> None:
    """`timing.max_frames` alone has no frame to count from: it is dropped, with a warning."""
    fragment = {"target": {"role": "release"}, "environment": {}}
    manifest = json.dumps({
        "id": "safe-mode-smoke",
        "settings": {"reset": "defaults", "tags": []},
        "level": {"fixture": "f.gmd", "sha256": "5" * 64, "source": "generated"},
        "timing": {"max_frames": 240, "stop_on": ["player_died"]},
    }).encode("utf-8")
    captured = io.StringIO()
    with contextlib.redirect_stderr(captured):
        context = preflight.build_context(fragment=fragment, scenario_bytes=manifest, repetition=0)
    assert "timing" not in context
    assert "nothing enforces it" in captured.getvalue()


def test_a_malformed_window_is_refused() -> None:
    fragment = {"target": {"role": "release"}, "environment": {}}
    cases = [
        ({"window": "level_started"}, "must be an object"),
        ({"window": {"max_frames": 600}}, "anchor must be 'level_started'"),
        ({"window": {"anchor": "attempt_started", "max_frames": 600}}, "anchor must be"),
        ({"window": {"anchor": "level_started"}}, "positive integer"),
        ({"window": {"anchor": "level_started", "max_frames": 0}}, "positive integer"),
        ({"window": {"anchor": "level_started", "max_frames": "600"}}, "positive integer"),
        ({"window": {"anchor": "level_started", "max_frames": True}}, "positive integer"),
        ({"window": {"anchor": "level_started", "max_frames": 600}, "stop_on": ["player_died"]},
         "stop_on is not supported"),
    ]
    for timing, message in cases:
        manifest = json.dumps({
            "id": "no-input-smoke",
            "settings": {"reset": "defaults", "tags": []},
            "level": {"fixture": "f.gmd", "sha256": "5" * 64, "source": "generated"},
            "timing": timing,
        }).encode("utf-8")
        try:
            with contextlib.redirect_stderr(io.StringIO()):
                preflight.build_context(fragment=fragment, scenario_bytes=manifest, repetition=0)
        except preflight.PreflightError as error:
            assert message in str(error), (message, error)
        else:
            raise AssertionError(f"{timing!r} should be refused")


def test_context_refuses_a_tag_that_disagrees_with_the_file() -> None:
    fragment = {"target": {}, "environment": {}}
    manifest = json.dumps({
        "id": "s",
        "settings": {"tags": [{"tag": "0x1E2", "label": "HACK_SAFE_MODE", "value": True}]},
        "level": {"sha256": "5" * 64},
    }).encode("utf-8")
    settings = {"V_BOOL": {"SAFETY/HACK_SAFE_MODE": False}}

    for tag_keys, message in [
        ({"0X1E2": ("value", "SAFETY/HACK_SAFE_MODE")}, "declares"),
        ({"0X1E2": ("value", "SAFETY/MISSING")}, "not present"),
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


def test_target_role_and_digest_pin() -> None:
    with tempfile.TemporaryDirectory() as tmp:
        target = Path(tmp) / "target.dll"
        make_pe(target)
        digest = hashlib.sha256(target.read_bytes()).hexdigest()

        fragment = preflight.build_fragment(
            target=target, gd=target, geode=target, harness=target,
            gd_version=None, geode_version=None, role="reconstruction",
            expected_sha256=digest.upper(),
        )
        assert fragment["target"]["role"] == "reconstruction"

        try:
            preflight.build_fragment(
                target=target, gd=target, geode=target, harness=target,
                gd_version=None, geode_version=None, expected_sha256="0" * 64,
            )
        except preflight.PreflightError as error:
            assert "pins" in str(error), error
        else:
            raise AssertionError("a digest that disagrees with the pin should be refused")

        try:
            preflight.build_fragment(
                target=target, gd=target, geode=target, harness=target,
                gd_version=None, geode_version=None, role="user-level",
            )
        except preflight.PreflightError as error:
            assert "role" in str(error)
        else:
            raise AssertionError("an unknown role should be refused")


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
    test_harness_version_comes_from_the_packaged_archive()
    test_context_carries_the_manifest_and_repetition()
    test_context_refuses_declared_tags_without_a_settings_file()
    test_context_refuses_the_unimplemented_harness_input_driver()
    test_context_accepts_the_operator_input_driver()
    test_context_warns_about_the_os_input_driver()
    test_context_reads_tag_values_from_the_settings_file()
    test_context_reads_a_map_b_flag_from_the_profile_list()
    test_context_carries_a_declared_capture_window()
    test_a_frame_count_without_an_anchor_is_not_carried()
    test_a_malformed_window_is_refused()
    test_context_refuses_a_tag_that_disagrees_with_the_file()
    test_settings_key_in_two_groups_is_refused()
    test_target_role_and_digest_pin()
    test_cli_refuses_a_live_profile_and_writes_a_fragment()
    print("all preflight tests passed")



