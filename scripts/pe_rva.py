"""Resolve PE RVAs/VAs to file offsets without silently lying about zero-fill.

Why this exists
---------------
Twice in this project's history a byte was read from the wrong place because an
RVA past a section's ``SizeOfRawData`` was converted to a file offset by linear
subtraction. Both times the result was plausible-looking data from a later
section: once unwind information from ``.pdata``, once a string address that
turned out to be a runtime-populated pointer slot.

``pefile.get_offset_from_rva`` and hand-rolled arithmetic both do this. The
Windows loader maps a section's *whole* page-rounded virtual extent, and zeroes
the tail past ``SizeOfRawData``. An RVA in that tail is a valid mapped address
with no corresponding byte in the file, so "the file offset" of it is not a
number - it is an error.

This module answers the question properly and distinguishes four cases:

    backed        the RVA has real bytes; a file offset is returned
    zero-fill     the RVA is mapped and reads as 0, but has no file bytes
    gap           the RVA is past a section's VirtualSize, before the next
                  section, and is inside no section at all
    unmapped      the RVA is outside the image

No third-party dependencies, so it works on a fresh clone with no sample set
present.

Usage
-----
    python pe_rva.py <image.dll> <rva-or-va> [<rva-or-va> ...]

    python pe_rva.py samples/absolllute.hackmega/absolllute.hackmega_original.dll \\
        0x800840 0x57AEFC 0x81BFC0

As a library:

    from pe_rva import Image
    img = Image.from_path("mod.dll")
    print(img.resolve(0x180286371))
"""

from __future__ import annotations

import argparse
import dataclasses
import pathlib
import struct
import sys

IMAGE_BASE_DEFAULT = 0
DOS_MAGIC = 0x5A4D
PE_SIGNATURE = b"PE\0\0"
COFF_HEADER_SIZE = 20
SECTION_HEADER_SIZE = 40
OPTIONAL_HEADER_MIN_SIZE = 28

SCN_CNT_CODE = 0x00000020
SCN_CNT_INITIALIZED_DATA = 0x00000040
SCN_CNT_UNINITIALIZED_DATA = 0x00000080
SCN_MEM_DISCARDABLE = 0x02000000
SCN_MEM_NOT_CACHED = 0x04000000
SCN_MEM_NOT_PAGED = 0x08000000
SCN_MEM_SHARED = 0x10000000
SCN_MEM_EXECUTE = 0x20000000
SCN_MEM_READ = 0x40000000
SCN_MEM_WRITE = 0x80000000

CHARACTERISTIC_NAMES = (
    (SCN_CNT_CODE, "CODE"),
    (SCN_CNT_INITIALIZED_DATA, "INITIALIZED_DATA"),
    (SCN_CNT_UNINITIALIZED_DATA, "UNINITIALIZED_DATA"),
    (SCN_MEM_DISCARDABLE, "DISCARDABLE"),
    (SCN_MEM_NOT_CACHED, "NOT_CACHED"),
    (SCN_MEM_NOT_PAGED, "NOT_PAGED"),
    (SCN_MEM_SHARED, "SHARED"),
    (SCN_MEM_EXECUTE, "EXECUTE"),
    (SCN_MEM_READ, "READ"),
    (SCN_MEM_WRITE, "WRITE"),
)


class PeFormatError(Exception):
    """The file is not a PE image this module can read."""


def decode_characteristics(value: int) -> str:
    names = [name for mask, name in CHARACTERISTIC_NAMES if value & mask]
    return "|".join(names) if names else f"0x{value:08x}"


@dataclasses.dataclass(frozen=True)
class Section:
    name: str
    virtual_address: int
    virtual_size: int
    size_of_raw_data: int
    pointer_to_raw_data: int
    characteristics: int

    @property
    def virtual_end(self) -> int:
        """Exclusive end of the section's declared virtual extent."""
        return self.virtual_address + self.virtual_size

    @property
    def raw_end(self) -> int:
        """Exclusive end of the section's raw-backed virtual extent."""
        return self.virtual_address + self.size_of_raw_data

    def describe(self) -> str:
        return (
            f"{self.name:<8} RVA {self.virtual_address:#010x}..{self.virtual_end:#010x} "
            f"raw-backed {self.virtual_address:#010x}..{self.raw_end:#010x} "
            f"file {self.pointer_to_raw_data:#x}.."
            f"{self.pointer_to_raw_data + self.size_of_raw_data:#x} "
            f"[{decode_characteristics(self.characteristics)}]"
        )


@dataclasses.dataclass(frozen=True)
class Resolution:
    rva: int
    kind: str
    section: Section | None
    file_offset: int | None
    note: str

    @property
    def ok(self) -> bool:
        return self.kind == "backed"


class Image:
    """Minimal read-only PE image: headers and section table only."""

    def __init__(self, data: bytes, path: pathlib.Path | None = None) -> None:
        self.data = data
        self.path = path
        self._parse_headers()

    @classmethod
    def from_path(cls, path: str | pathlib.Path) -> "Image":
        p = pathlib.Path(path)
        return cls(p.read_bytes(), p)

    def _parse_headers(self) -> None:
        data = self.data
        if len(data) < 0x40 or data[:2] != b"MZ":
            raise PeFormatError("missing MZ signature; not a PE image")
        (lfanew,) = struct.unpack_from("<I", data, 0x3C)
        if lfanew + 4 > len(data) or data[lfanew : lfanew + 4] != PE_SIGNATURE:
            raise PeFormatError(f"missing PE signature at e_lfanew={lfanew:#x}")

        coff = lfanew + 4
        (self.machine, self.number_of_sections, _, _, _, self.size_of_optional_header, _) = (
            struct.unpack_from("<HHIIIHH", data, coff)
        )

        optional = coff + COFF_HEADER_SIZE
        if self.size_of_optional_header < OPTIONAL_HEADER_MIN_SIZE:
            raise PeFormatError("optional header too small")
        magic, = struct.unpack_from("<H", data, optional)
        if magic == 0x20B:
            self.pe32_plus = True
        elif magic == 0x10B:
            self.pe32_plus = False
        else:
            raise PeFormatError(f"unknown optional header magic {magic:#x}")

        image_base_off = optional + (24 if self.pe32_plus else 28)
        (self.image_base,) = struct.unpack_from("<Q" if self.pe32_plus else "<I", data, image_base_off)
        (self.section_alignment,) = struct.unpack_from("<I", data, optional + 32)
        (self.size_of_image,) = struct.unpack_from("<I", data, optional + 56)

        table = optional + self.size_of_optional_header
        self.sections: list[Section] = []
        for index in range(self.number_of_sections):
            base = table + index * SECTION_HEADER_SIZE
            (raw_name, vsize, vaddr, raw_size, raw_ptr) = struct.unpack_from(
                "<8sIIII", data, base
            )
            (characteristics,) = struct.unpack_from("<I", data, base + 36)
            self.sections.append(
                Section(
                    name=raw_name.rstrip(b"\0").decode(errors="replace"),
                    virtual_size=vsize,
                    virtual_address=vaddr,
                    size_of_raw_data=raw_size,
                    pointer_to_raw_data=raw_ptr,
                    characteristics=characteristics,
                )
            )
        if not self.sections:
            raise PeFormatError("no sections")

    # -- address handling ------------------------------------------------

    def to_rva(self, address: int) -> int:
        """Accept either a preferred-base VA or a bare RVA."""
        if self.image_base and address >= self.image_base:
            return address - self.image_base
        return address

    def to_va(self, rva: int) -> int:
        return self.image_base + rva

    def round_up(self, value: int, boundary: int) -> int:
        return (value + boundary - 1) // boundary * boundary

    def section_for(self, rva: int) -> Section | None:
        for section in self.sections:
            if section.virtual_address <= rva < section.virtual_end:
                return section
        return None

    def mapped_extent(self, section: Section) -> int:
        """Page-rounded virtual end, which is what the loader actually commits."""
        alignment = self.section_alignment or 0x1000
        return self.round_up(section.virtual_size, alignment)

    def resolve(self, address: int) -> Resolution:
        """Classify an address and return a file offset only when one exists."""
        rva = self.to_rva(address)
        if rva >= self.size_of_image:
            return Resolution(rva, "unmapped", None, None, f"past SizeOfImage {self.size_of_image:#x}")

        section = self.section_for(rva)
        if section is None:
            owner = self._nearest_section(rva)
            if owner is None:
                return Resolution(rva, "unmapped", None, None, "no section covers this RVA")
            return Resolution(
                rva,
                "gap",
                None,
                None,
                f"past {owner.name} VirtualSize {owner.virtual_size:#x}; "
                f"before {owner.name} ends at {owner.virtual_end:#x}",
            )

        delta = rva - section.virtual_address
        if delta >= section.size_of_raw_data:
            return Resolution(
                rva,
                "zero-fill",
                section,
                None,
                f"{delta - section.size_of_raw_data:#x} past {section.name} "
                f"SizeOfRawData {section.size_of_raw_data:#x}; mapped by page rounding to "
                f"{section.virtual_address + self.mapped_extent(section):#x}, reads as 0",
            )

        return Resolution(
            rva,
            "backed",
            section,
            section.pointer_to_raw_data + delta,
            f"{delta:#x} into {section.name}",
        )

    def read(self, address: int, size: int) -> bytes:
        """Read bytes, refusing any address with no file backing."""
        resolution = self.resolve(address)
        if not resolution.ok:
            raise PeFormatError(
                f"cannot read {size} byte(s) at RVA {resolution.rva:#x}: {resolution.kind} "
                f"({resolution.note})"
            )
        start = resolution.file_offset
        end = start + size
        if end > len(self.data):
            raise PeFormatError(
                f"read at RVA {resolution.rva:#x} runs past end of file "
                f"({start:#x}+{size:#x} > {len(self.data):#x})"
            )
        return self.data[start:end]

    def _nearest_section(self, rva: int) -> Section | None:
        before = [s for s in self.sections if s.virtual_end <= rva]
        return max(before, key=lambda s: s.virtual_end, default=None)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Resolve PE RVAs/VAs to file offsets, distinguishing zero-fill from real data.",
    )
    parser.add_argument("image", help="path to a PE image")
    parser.add_argument("addresses", nargs="+", help="RVAs or preferred-base VAs")
    parser.add_argument(
        "--sections", action="store_true", help="also list the section table"
    )
    args = parser.parse_args(argv)

    try:
        image = Image.from_path(args.image)
    except (OSError, PeFormatError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2

    print(f"{args.image}")
    print(f"  ImageBase {image.image_base:#x}  SectionAlignment {image.section_alignment:#x}  "
          f"SizeOfImage {image.size_of_image:#x}  sections {image.number_of_sections}")
    if args.sections:
        print()
        for section in image.sections:
            print(f"  {section.describe()}")
    print()

    failures = 0
    for raw in args.addresses:
        try:
            value = int(raw, 0)
        except ValueError:
            print(f"  {raw:<18} not a number")
            failures += 1
            continue
        r = image.resolve(value)
        where = f"{r.rva:#x} (VA {image.to_va(r.rva):#x})"
        if r.kind == "backed":
            assert r.section is not None and r.file_offset is not None
            print(f"  {where:<34} backed   file {r.file_offset:#x}  {r.note}")
        else:
            failures += 1
            print(f"  {where:<34} {r.kind:<8} no file offset  {r.note}")

    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())