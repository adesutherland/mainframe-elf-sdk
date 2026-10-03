#!/usr/bin/env python3
"""Check source-built classic TSO entry objects before native linking.

This checks transport, section mode, external closure and independently
expected instruction octets. Guest service behavior remains a separate gate.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path

from tso31_deck_check import EXPECTED_SERVICES, check_deck


def check_source_entry(path: Path, bits: int) -> dict:
    if bits not in (24, 31):
        raise ValueError(f"unsupported entry width: {bits}")
    name = "LABTS24" if bits == 24 else "LABTSO"
    attribute = 0x01 if bits == 24 else 0x06
    info = check_deck(path, expected_attribute=attribute)
    if len(info["sections"]) != 1 or info["sections"][0]["name"] != name:
        raise ValueError(f"expected one {name} section")
    if set(info["externals"]) != EXPECTED_SERVICES | {"ELFPOC"}:
        raise ValueError("source entry file-call closure changed")
    size = info["sections"][0]["size"]
    image, present = bytearray(size), bytearray(size)
    data = path.read_bytes()
    for start in range(0, len(data), 80):
        card = data[start:start + 80]
        if card[1:4].decode("cp037") != "TXT":
            continue
        offset = int.from_bytes(card[5:8], "big")
        length = int.from_bytes(card[10:12], "big")
        if not 0 < length <= 56 or offset + length > size:
            raise ValueError("invalid source entry text extent")
        if any(present[offset:offset + length]):
            raise ValueError("overlapping source entry text")
        image[offset:offset + length] = card[16:16 + length]
        present[offset:offset + length] = b"\x01" * length
    # EPSW comes from the z/Architecture PoP. Selected SVC numbers and
    # conditional storage options come from the public register interfaces
    # recorded in z-pdos/tso31-bridge/doc/architecture/SERVICES.md.
    patterns = {"SVC 120": (bytes.fromhex("0a78"), 6),
                "SVC 93": (bytes.fromhex("0a5d"), 2),
                "SVC 99": (bytes.fromhex("0a63"), 1),
                "below-line GETMAIN": (bytes.fromhex("1b1141f000100a78"),
                                       3 if bits == 24 else 1)}
    if bits == 31:
        patterns["31-bit GETMAIN"] = (bytes.fromhex("1b1141f000300a78"), 2)
        patterns["EPSW R2,R3"] = (bytes.fromhex("b98d0023"), 2)
    for label, (pattern, expected) in patterns.items():
        actual = sum(image[i:i + len(pattern)] == pattern and
                     all(present[i:i + len(pattern)])
                     for i in range(size - len(pattern) + 1))
        if actual != expected:
            raise ValueError(f"{label}: expected {expected}, found {actual}")
    return {"format": "mainframe-elf-sdk-tso-entry-source-v1", "profile_bits": bits,
            "entry": info, "independent_service_patterns": sorted(patterns)}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bits", type=int, choices=(24, 31), required=True)
    parser.add_argument("--entry", type=Path, required=True)
    parser.add_argument("--out", type=Path)
    args = parser.parse_args()
    result = check_source_entry(args.entry.resolve(strict=True), args.bits)
    if args.out:
        args.out.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n")
    print(f"TSO{args.bits} source entry mode, closure and service instructions PASS")


if __name__ == "__main__":
    main()
