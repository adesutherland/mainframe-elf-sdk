#!/usr/bin/env python3
"""Preflight the retained AMODE64/RMODE ANY entry and below-bar C image.

This checks the actual ASMA90/ELF exporter cards before the Linux writer
packages them. Guest execution remains the authority for service behavior.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path


FILE_SERVICES = {"@@AOPEN", "@@AREAD", "@@AWRITE", "@@ACLOSE", "@@DYNAL"}
RLD_FLAGS = {0x0C, 0x0D, 0x1C, 0x1D}


def inspect(path: Path, attribute: int, section_name: str | None) -> dict:
    data = path.read_bytes()
    if not data or len(data) % 80:
        raise ValueError(f"incomplete FB80 object deck: {path}")
    sections: list[dict] = []
    exports: set[str] = set()
    externals: set[str] = set()
    relocations = 0
    kinds: list[str] = []
    for offset in range(0, len(data), 80):
        card = data[offset:offset + 80]
        if card[0] != 2:
            raise ValueError(f"invalid object marker: {path}:{offset}")
        kind = card[1:4].decode("cp037")
        kinds.append(kind)
        if kind not in {"ESD", "TXT", "RLD", "END"}:
            raise ValueError(f"unsupported card {kind}: {path}:{offset}")
        if kind not in {"ESD", "RLD"}:
            continue
        size = int.from_bytes(card[10:12], "big")
        if size > 56:
            raise ValueError(f"oversized object-card payload: {path}:{offset}")
        payload = card[16:16 + size]
        if kind == "ESD":
            if size % 16:
                raise ValueError(f"partial ESD: {path}:{offset}")
            for start in range(0, size, 16):
                item = payload[start:start + 16]
                name = item[:8].decode("cp037").rstrip()
                item_type = item[8]
                if item_type in (0, 4):
                    if item[12] != attribute:
                        raise ValueError(f"{path}: {name} mode {item[12]:#04x}, "
                                         f"expected {attribute:#04x}")
                    sections.append({"name": name, "attribute": f"{item[12]:02x}",
                                     "size": int.from_bytes(item[13:16], "big")})
                elif item_type == 1:
                    exports.add(name)
                elif item_type == 2:
                    externals.add(name)
                else:
                    raise ValueError(f"unsupported ESD type {item_type:#04x}: {path}")
        else:
            index = 0
            include_ids = True
            while index < size:
                width = 8 if include_ids else 4
                if index + width > size:
                    raise ValueError(f"partial RLD: {path}:{offset}")
                flag = payload[index + (4 if include_ids else 0)]
                if flag not in RLD_FLAGS:
                    raise ValueError(f"unsupported RLD flag {flag:#04x}: {path}")
                relocations += 1
                index += width
                include_ids = not bool(flag & 1)
    if kinds[0] != "ESD" or kinds[-1] != "END" or len(sections) != 1:
        raise ValueError(f"expected one complete CSECT: {path}")
    if section_name is not None and sections[0]["name"] != section_name:
        raise ValueError(f"unexpected CSECT {sections[0]['name']!r}: {path}")
    if not 0 < sections[0]["size"] < 16 * 1024 * 1024:
        raise ValueError(f"invalid CSECT size: {path}")
    return {"sha256": hashlib.sha256(data).hexdigest(), "cards": len(kinds),
            "sections": sections, "exports": sorted(exports),
            "externals": sorted(externals), "relocations": relocations}


def check_triple(entry: Path, image: Path, service: Path) -> dict:
    e = inspect(entry, 0x14, "LABTS64")
    c = inspect(image, 0x06, "ELFPOC")
    s = inspect(service, 0x06, "")
    if set(e["externals"]) != FILE_SERVICES | {"ELFPOC"}:
        raise ValueError("AMODE64 entry service closure changed")
    if c["externals"]:
        raise ValueError("exported C image has unresolved native references")
    if not FILE_SERVICES <= set(s["exports"]) or s["externals"]:
        raise ValueError("native file-service closure changed")
    if not e["relocations"] or not c["relocations"]:
        raise ValueError("entry or C image lost relocation records")
    return {"format": "mainframe-lab-tso64-native-decks-v1", "entry": e,
            "image": c, "service": s,
            "required_file_services": sorted(FILE_SERVICES)}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--entry", type=Path, required=True)
    parser.add_argument("--image", type=Path, required=True)
    parser.add_argument("--service", type=Path, required=True)
    parser.add_argument("--out", type=Path)
    args = parser.parse_args()
    result = check_triple(args.entry.resolve(strict=True),
                          args.image.resolve(strict=True),
                          args.service.resolve(strict=True))
    if args.out:
        args.out.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n")
    print("TSO64 native decks AMODE64/RMODE ANY and file-service closure PASS")


if __name__ == "__main__":
    main()
