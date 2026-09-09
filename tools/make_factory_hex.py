#!/usr/bin/env python3
"""Build and validate the TECKIO factory Intel HEX image."""

from __future__ import annotations

import argparse
import struct
import zlib
from pathlib import Path

BOOT_START = 0x0000
BOOT_END = 0x2000
APP_START = 0x2000
METADATA_START = 0x7FC0
FLASH_END = 0x8000
CONFIG_START = 0x300000
CONFIG_END = 0x30000E
METADATA_MAGIC = b"TKAPP01\xA5"
METADATA_COMMIT = 0x51AA3CC3


class HexError(ValueError):
    pass


def parse_hex(path: Path) -> dict[int, int]:
    memory: dict[int, int] = {}
    address_base = 0
    eof_seen = False

    for line_number, raw in enumerate(path.read_text(encoding="ascii").splitlines(), 1):
        line = raw.strip()
        if not line:
            continue
        if eof_seen:
            raise HexError(f"{path}:{line_number}: data after EOF")
        if not line.startswith(":") or len(line) < 11 or len(line) % 2 == 0:
            raise HexError(f"{path}:{line_number}: invalid Intel HEX record")
        try:
            record = bytes.fromhex(line[1:])
        except ValueError as exc:
            raise HexError(f"{path}:{line_number}: invalid hexadecimal data") from exc
        if len(record) != record[0] + 5 or sum(record) & 0xFF:
            raise HexError(f"{path}:{line_number}: invalid length or checksum")

        count = record[0]
        offset = (record[1] << 8) | record[2]
        record_type = record[3]
        data = record[4 : 4 + count]

        if record_type == 0x00:
            for index, value in enumerate(data):
                address = address_base + offset + index
                previous = memory.setdefault(address, value)
                if previous != value:
                    raise HexError(f"{path}:{line_number}: conflicting byte at 0x{address:06X}")
        elif record_type == 0x01:
            if count:
                raise HexError(f"{path}:{line_number}: invalid EOF")
            eof_seen = True
        elif record_type == 0x02:
            if count != 2:
                raise HexError(f"{path}:{line_number}: invalid segment address")
            address_base = int.from_bytes(data, "big") << 4
        elif record_type == 0x04:
            if count != 2:
                raise HexError(f"{path}:{line_number}: invalid linear address")
            address_base = int.from_bytes(data, "big") << 16
        elif record_type not in (0x03, 0x05):
            raise HexError(f"{path}:{line_number}: unsupported record type 0x{record_type:02X}")

    if not eof_seen:
        raise HexError(f"{path}: missing EOF record")
    return memory


def make_record(address: int, record_type: int, data: bytes = b"") -> str:
    payload = bytes((len(data), (address >> 8) & 0xFF, address & 0xFF, record_type)) + data
    checksum = (-sum(payload)) & 0xFF
    return ":" + (payload + bytes((checksum,))).hex().upper()


def write_hex(path: Path, memory: dict[int, int]) -> None:
    lines: list[str] = []
    addresses = sorted(memory)
    index = 0
    current_upper: int | None = None

    while index < len(addresses):
        start = addresses[index]
        upper = start >> 16
        if upper != current_upper:
            lines.append(make_record(0, 0x04, upper.to_bytes(2, "big")))
            current_upper = upper

        data = bytearray((memory[start],))
        index += 1
        while index < len(addresses) and len(data) < 16:
            address = addresses[index]
            if address != start + len(data) or address >> 16 != upper:
                break
            data.append(memory[address])
            index += 1
        lines.append(make_record(start & 0xFFFF, 0x00, bytes(data)))

    lines.append(make_record(0, 0x01))
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines) + "\n", encoding="ascii")


def application_payload(memory: dict[int, int]) -> tuple[int, int, int, int]:
    addresses = sorted(memory)
    if not addresses or APP_START not in memory:
        raise HexError("application does not contain its relocated reset vector at 0x2000")
    if any(address < APP_START or address >= METADATA_START for address in addresses):
        raise HexError("application contains data outside 0x2000-0x7FBF")

    crc = 0
    previous: int | None = None
    segment = bytearray()
    for address in addresses:
        if previous is not None and address != previous + 1:
            crc = zlib.crc32(segment, crc)
            segment.clear()
        segment.append(memory[address])
        previous = address
    if segment:
        crc = zlib.crc32(segment, crc)

    return APP_START, addresses[-1] + 1, len(addresses), crc & 0xFFFFFFFF


def metadata_row(app: dict[int, int]) -> bytes:
    start, end, byte_count, crc = application_payload(app)
    row = bytearray(b"\xFF" * 64)
    row[:8] = METADATA_MAGIC
    row[8:12] = struct.pack("<I", start)
    row[12:16] = struct.pack("<I", end)
    row[16:20] = struct.pack("<I", byte_count)
    row[20:24] = struct.pack("<I", crc)
    row[24:28] = struct.pack("<I", (~crc) & 0xFFFFFFFF)
    row[28:32] = struct.pack("<I", METADATA_COMMIT)
    row[32:36] = struct.pack("<I", (~METADATA_COMMIT) & 0xFFFFFFFF)
    return bytes(row)


def build_factory(bootloader: dict[int, int], application: dict[int, int]) -> dict[int, int]:
    for address in bootloader:
        in_loader = BOOT_START <= address < BOOT_END
        in_config = CONFIG_START <= address < CONFIG_END
        if not (in_loader or in_config):
            raise HexError(f"bootloader byte outside protected/config area: 0x{address:06X}")
    application_payload(application)

    factory = dict(bootloader)
    for address, value in application.items():
        if address in factory:
            raise HexError(f"bootloader/application overlap at 0x{address:06X}")
        factory[address] = value
    for offset, value in enumerate(metadata_row(application)):
        factory[METADATA_START + offset] = value
    return factory


def validate_factory(factory: dict[int, int], application: dict[int, int]) -> None:
    expected_metadata = metadata_row(application)
    actual_metadata = bytes(factory.get(METADATA_START + i, -1) for i in range(64))
    if actual_metadata != expected_metadata:
        raise HexError("factory metadata validation failed")
    if not any(BOOT_START <= address < BOOT_END for address in factory):
        raise HexError("factory image has no bootloader")
    if not any(CONFIG_START <= address < CONFIG_END for address in factory):
        raise HexError("factory image has no configuration bytes")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--bootloader", type=Path, required=True)
    parser.add_argument("--application", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    bootloader = parse_hex(args.bootloader)
    application = parse_hex(args.application)
    factory = build_factory(bootloader, application)
    write_hex(args.output, factory)
    validate_factory(parse_hex(args.output), application)

    start, end, count, crc = application_payload(application)
    print(
        f"Factory HEX OK: app=0x{start:04X}-0x{end - 1:04X}, "
        f"bytes={count}, crc32=0x{crc:08X}"
    )


if __name__ == "__main__":
    main()
