#!/usr/bin/env python3
"""Check native object residence and relocations before a TSO31 high package.

This reads ASMA90 80-byte EBCDIC object cards. It is a conservative preflight;
the guest must still prove actual load placement and service behavior.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path


EXPECTED_SERVICES = {"@@AOPEN", "@@AREAD", "@@AWRITE", "@@ACLOSE", "@@DYNAL"}
FORBIDDEN = {"@@GETEPF", "@@PCLST"}


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def check_deck(path: Path, *, expected_attribute: int = 0x06) -> dict:
    data = path.read_bytes()
    if not data or len(data) % 80:
        raise ValueError(f"not a complete FB80 object deck: {path}")
    sections: list[dict] = []
    exports: set[str] = set()
    externals: set[str] = set()
    relocations = 0
    first = last = ""
    for offset in range(0, len(data), 80):
        card = data[offset:offset + 80]
        if card[0] != 2:
            raise ValueError(f"invalid object marker at {path}:{offset}")
        kind = card[1:4].decode("cp037")
        if offset == 0:
            first = kind
        last = kind
        if kind not in {"ESD", "TXT", "RLD", "END"}:
            raise ValueError(f"unsupported object card {kind} at {path}:{offset}")
        if kind not in {"ESD", "RLD"}:
            continue
        size = int.from_bytes(card[10:12], "big")
        if size > 56:
            raise ValueError(f"object card payload exceeds 56 bytes: {path}:{offset}")
        payload = card[16:16 + size]
        if kind == "ESD":
            if size % 16:
                raise ValueError(f"partial ESD item: {path}:{offset}")
            for i in range(0, size, 16):
                item = payload[i:i + 16]
                name = item[:8].decode("cp037").rstrip()
                item_type = item[8]
                if item_type in (0, 4):  # SD or private CSECT
                    attribute = item[12]
                    if attribute != expected_attribute:
                        raise ValueError(
                            f"{path}: CSECT {name or '<private>'} has "
                            f"mode attribute {attribute:#04x}, expected {expected_attribute:#04x}"
                        )
                    sections.append({"name": name or "<private>",
                                     "attribute": f"{attribute:02x}",
                                     "size": int.from_bytes(item[13:16], "big")})
                elif item_type == 1:  # LD, including service entry names
                    exports.add(name)
                elif item_type == 2:  # ER
                    externals.add(name)
                else:
                    raise ValueError(
                        f"unsupported high-residence ESD type {item_type:#04x} "
                        f"at {path}:{offset}"
                    )
        else:
            i = 0
            include_ids = True
            while i < size:
                width = 8 if include_ids else 4
                if i + width > size:
                    raise ValueError(f"partial RLD item: {path}:{offset}")
                flag = payload[i + (4 if include_ids else 0)]
                # The assembled entry/service decks use only fullword A/V
                # relocations, with bit 0 continuing compressed RLD IDs.
                # Reject every other flag bit rather than guessing its meaning.
                if flag not in (0x0C, 0x0D, 0x1C, 0x1D):
                    raise ValueError(
                        f"unsupported high-residence RLD flag {flag:#04x} "
                        f"at {path}:{offset}"
                    )
                relocations += 1
                i += width
                include_ids = not bool(flag & 1)
    if first != "ESD" or last != "END" or not sections:
        raise ValueError(f"incomplete native object deck: {path}")
    if FORBIDDEN & (exports | externals | {s["name"] for s in sections}):
        raise ValueError(f"TSO prefix parser symbol remains in {path}")
    return {"sha256": sha(path), "cards": len(data) // 80,
            "sections": sections, "exports": sorted(exports),
            "externals": sorted(externals), "relocations": relocations}


def check_pair(entry: Path, service: Path) -> dict:
    entry_info = check_deck(entry)
    service_info = check_deck(service)
    if not any(s["name"] == "LABTSO" for s in entry_info["sections"]):
        raise ValueError("native entry has no LABTSO CSECT")
    if set(entry_info["externals"]) != EXPECTED_SERVICES | {"ELFPOC"}:
        raise ValueError("native entry file-call closure changed")
    if not EXPECTED_SERVICES <= set(service_info["exports"]):
        raise ValueError("native service deck lacks a retained file entry")
    definitions = (set(entry_info["exports"]) |
                   set(service_info["exports"]) |
                   {s["name"] for s in entry_info["sections"] +
                    service_info["sections"]})
    unresolved = set(service_info["externals"]) - definitions
    if unresolved:
        raise ValueError(f"native service references are unresolved: {sorted(unresolved)}")
    return {"format": "mainframe-lab-tso31-high-native-decks-v1",
            "entry": entry_info, "service": service_info,
            "required_file_services": sorted(EXPECTED_SERVICES)}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--entry", type=Path, required=True)
    parser.add_argument("--service", type=Path, required=True)
    parser.add_argument("--out", type=Path)
    args = parser.parse_args()
    result = check_pair(args.entry.resolve(strict=True),
                        args.service.resolve(strict=True))
    if args.out:
        args.out.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n")
    print("TSO31 native deck AMODE31/RMODE ANY and file-service closure PASS")


if __name__ == "__main__":
    main()
