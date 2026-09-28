#!/usr/bin/env python3
"""Extract one native IEBCOPY unload from a fixed-card TSO XMIT container.

Only XMIT transport framing is removed. Every IEBCOPY logical payload is
copied unchanged and prefixed with its original logical length as an RDW.
The complete input and directory bytes are retained and hashed in the receipt.
"""

import argparse
import hashlib
import json
from pathlib import Path


def sha(data):
    return hashlib.sha256(data).hexdigest()


def decode_xmit(data):
    if len(data) % 80 or len(data) < 80:
        raise ValueError("XMIT is not complete FB80 transport cards")
    offset = 0
    controls = []
    payloads = []
    segments = 0
    parts = None
    control = False
    while offset < len(data):
        if offset + 2 > len(data):
            raise ValueError("truncated XMIT segment")
        length, flag = data[offset:offset + 2]
        if length < 2 or offset + length > len(data):
            raise ValueError(f"invalid XMIT segment length at {offset}")
        if flag & ~0xE0:
            raise ValueError(f"unsupported XMIT segment flag at {offset}")
        if flag & 0x80:
            if parts is not None:
                raise ValueError("nested XMIT record")
            parts = []
            control = bool(flag & 0x20)
        elif parts is None or control != bool(flag & 0x20):
            raise ValueError("orphan or mixed-type XMIT segment")
        parts.append(data[offset + 2:offset + length])
        segments += 1
        offset += length
        if flag & 0x40:
            record = b"".join(parts)
            parts = None
            if control:
                label = record[:6].decode("cp037")
                controls.append(label)
                if label == "INMR06":
                    if len(data[offset:]) >= 80 or any(b != 0x40 for b in data[offset:]):
                        raise ValueError("invalid XMIT trailer padding")
                    break
            else:
                payloads.append(record)
    if parts is not None or controls != ["INMR01", "INMR02", "INMR02",
                                      "INMR03", "INMR06"]:
        raise ValueError(f"unsupported XMIT control sequence {controls}")
    if len(payloads) < 4 or payloads[0][:4] != bytes.fromhex("00ca6d0f"):
        raise ValueError("missing IEBCOPY unload header")
    return payloads, controls, segments


def extract(data, expected_member, expected_amode, expected_rmode="ANY"):
    payloads, controls, segments = decode_xmit(data)
    directory = payloads[2]
    if len(directory) != 288 or int.from_bytes(directory[20:22], "big") != 50:
        raise ValueError("expected one native load-member directory")
    member = directory[22:30].decode("cp037").rstrip()
    if member != expected_member:
        raise ValueError(f"expected member {expected_member}, got {member}")
    flags = directory[53]
    amode_code = flags & 0x03
    amode = {0: 24, 1: 64, 2: 31}.get(amode_code)
    if flags & 0x20:
        if not flags & 0x10:
            raise ValueError(f"unsupported native RMODE flag combination {flags:02x}")
        rmode = "64"
    else:
        rmode = "ANY" if flags & 0x10 else "24"
    if amode is None or amode != expected_amode or rmode != expected_rmode:
        raise ValueError(f"unsupported or unexpected native AMODE/RMODE byte {flags:02x}")
    output = bytearray()
    for number, payload in enumerate(payloads, 1):
        length = len(payload) + 4
        if length > 65535:
            raise ValueError(f"IEBCOPY record {number} exceeds RDW limit")
        output += length.to_bytes(2, "big") + b"\0\0" + payload
    return bytes(output), {
        "member": member, "native_directory_amode": amode,
        "native_directory_rmode": rmode, "directory_flags_hex": f"{flags:02x}",
        "directory_sha256": sha(directory), "directory_hex": directory.hex(),
        "iecopy_payload_count": len(payloads), "xmit_segments": segments,
        "xmit_controls": controls,
        "payload_hashes_sha256": [sha(payload) for payload in payloads],
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--member", required=True)
    parser.add_argument("--amode", type=int, choices=(31, 64), required=True)
    parser.add_argument("--rmode", choices=("24", "ANY", "64"), default="ANY")
    args = parser.parse_args()
    if args.output.exists():
        parser.error(f"output already exists: {args.output}")
    data = args.input.read_bytes()
    output, details = extract(data, args.member, args.amode, args.rmode)
    args.output.write_bytes(output)
    receipt = {"input": str(args.input.resolve()), "input_sha256": sha(data),
               "input_bytes": len(data), "output": str(args.output.resolve()),
               "output_sha256": sha(output), "output_bytes": len(output),
               **details}
    args.output.with_suffix(args.output.suffix + ".json").write_text(
        json.dumps(receipt, indent=2) + "\n")
    print(json.dumps({key: value for key, value in receipt.items()
                      if key not in ("directory_hex", "payload_hashes_sha256")}))


if __name__ == "__main__":
    main()
