"""Regression tests for scripts/pe_rva.py.

These encode the two wrong reads this project actually performed, so neither can
be silently reintroduced. The test skips itself when the sample set is absent,
since a fresh clone has no binaries.

    python scripts/test_pe_rva.py
"""

from __future__ import annotations

import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from pe_rva import Image, PeFormatError  # noqa: E402

REPO = pathlib.Path(__file__).resolve().parent.parent
BASELINE = REPO / "samples" / "absolllute.hackmega" / "absolllute.hackmega_original.dll"

# (rva, expected kind, expected file offset or None, what it proves)
CASES = [
    # A real string literal in .rdata.
    (0x57AEFC, "backed", 0x57A2FC, "GJGameLevel::savePercentage is raw .rdata"),
    # Real code.
    (0x72B60, "backed", 0x71F60, "geodeImplicitEntry is raw .text"),
    # BUG 1: RVA past .data SizeOfRawData. Linear subtraction returned file
    # 0x81B1C0, which is .pdata unwind info, not the requested address.
    (0x800840, "zero-fill", None, "runtime pointer slot has no file bytes"),
    (0x800848, "zero-fill", None, "second runtime pointer slot has no file bytes"),
    # The mutable state block. Past VirtualSize, so no section claims it, yet
    # the loader still maps and commits its page writable.
    (0x81BFC0, "gap", None, "state block is past VirtualSize, not file-backed"),
    (0x81BD90, "gap", None, "exact VirtualSize end is a gap"),
    # The page that actually carries the state block.
    (0x81B000, "zero-fill", None, "state block's page is zero-fill but mapped"),
    (0xFFFFFFFF, "unmapped", None, "past SizeOfImage"),
]


def check_backed_agrees_with_linear_arithmetic(image: Image) -> None:
    """For genuinely backed RVAs the two methods must agree.

    This is what makes the refusal meaningful: pe_rva.py is not simply
    disagreeing with everyone, it agrees on real data and refuses only where a
    file offset does not exist.
    """
    for section in image.sections:
        probe = section.virtual_address + min(section.size_of_raw_data, 0x100) - 1
        resolution = image.resolve(probe)
        if resolution.kind != "backed" or resolution.section is None:
            continue
        naive = section.pointer_to_raw_data + (probe - section.virtual_address)
        assert resolution.file_offset == naive, (
            f"disagreement on backed RVA {probe:#x}: "
            f"{resolution.file_offset:#x} != {naive:#x}"
        )


def check_read_refuses_unbacked(image: Image) -> None:
    for address in (0x800840, 0x81BFC0):
        try:
            image.read(address, 8)
        except PeFormatError:
            continue
        raise AssertionError(f"read() should have refused {address:#x}")


def check_bad_images_rejected(tmp: pathlib.Path) -> None:
    bad = tmp / "not-a-pe.bin"
    bad.write_bytes(b"nope" * 64)
    for payload, label in (
        (b"MZ" + b"\x00" * 62, "MZ with no PE signature"),
        (b"ZZ" + b"\x00" * 62, "no MZ signature"),
    ):
        bad.write_bytes(payload)
        try:
            Image.from_path(bad)
        except PeFormatError:
            continue
        raise AssertionError(f"should have rejected {label}")


def main() -> int:
    if not BASELINE.exists():
        print(f"skip: no sample set at {BASELINE}")
        return 0

    image = Image.from_path(BASELINE)
    assert image.image_base == 0x180000000, f"ImageBase {image.image_base:#x}"
    assert image.section_alignment == 0x1000
    assert len(image.sections) == 7, f"{len(image.sections)} sections"
    assert image.sections[0].name == ".text"
    assert image.sections[2].name == ".data"

    # .data must be readable/writable, not read-only. A stale-loop-variable bug
    # once reported .reloc's characteristics under a .data label.
    data_section = image.sections[2]
    assert data_section.characteristics & 0x80000000, ".data must be writable"
    assert data_section.characteristics & 0x40000000, ".data must be readable"
    assert not (image.sections[6].characteristics & 0x80000000), ".reloc is not writable"

    failures = 0
    for rva, kind, offset, why in CASES:
        resolution = image.resolve(rva)
        if resolution.kind != kind or resolution.file_offset != offset:
            failures += 1
            print(
                f"FAIL RVA {rva:#x}: expected {kind}/{offset}, "
                f"got {resolution.kind}/{resolution.file_offset}"
            )
            print(f"     {why}")
        else:
            print(f"ok   {rva:#x} {kind:<9} {why}")

    check_backed_agrees_with_linear_arithmetic(image)
    print("ok   backed RVAs agree with linear arithmetic")

    # The inverse mapping must round-trip for every backed address.
    # Six sections have SizeOfRawData > VirtualSize: the trailing bytes are
    # file alignment padding with no virtual mapping, so clamp the high probe.
    mismatches = 0
    for section in image.sections:
        span = min(section.size_of_raw_data, section.virtual_size)
        if span <= 0:
            continue
        for offset in (
            section.pointer_to_raw_data,
            section.pointer_to_raw_data + span - 1,
        ):
            va = image.va_of_file_offset(offset)
            back = image.resolve(va) if va is not None else None
            if back is None or not back.ok or back.file_offset != offset:
                mismatches += 1
    assert mismatches == 0, f"{mismatches} offset/VA round-trips failed"
    print("ok   file offset -> VA round-trips at both section ends")

    # Offsets outside every section have no VA and must be refused, not guessed.
    assert image.va_of_file_offset(0) is None, "headers should not map to a VA"
    assert image.va_of_file_offset(len(image.data) - 1) is None, "EOF should not map"

    check_read_refuses_unbacked(image)
    print("ok   read() refuses unbacked addresses")

    import tempfile

    with tempfile.TemporaryDirectory() as tmp:
        check_bad_images_rejected(pathlib.Path(tmp))
    print("ok   malformed images rejected")

    print()
    if failures:
        print(f"{failures} case(s) failed")
        return 1
    print(f"all {len(CASES)} address cases plus 3 invariants passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())