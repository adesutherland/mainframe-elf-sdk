#!/usr/bin/env python3
"""Check the linked high image's initial C save area against its actual code."""

import argparse
import struct
from pathlib import Path


def check(path):
    data = path.read_bytes()
    if data[:6] != b"\x7fELF\x02\x02" or len(data) < 64:
        raise ValueError("expected ELF64 big-endian image")
    shoff = struct.unpack_from(">Q", data, 40)[0]
    entsize, count, shstr = struct.unpack_from(">HHH", data, 58)
    if entsize != 64 or count == 0 or shoff + count * 64 > len(data):
        raise ValueError("invalid ELF section table")
    sections = [struct.unpack_from(">IIQQQQIIQQ", data, shoff + i * 64)
                for i in range(count)]
    if shstr >= count:
        raise ValueError("invalid section names")
    names_sec = sections[shstr]
    names = data[names_sec[4]:names_sec[4] + names_sec[5]]

    def name(offset, pool):
        if offset >= len(pool):
            raise ValueError("invalid ELF string offset")
        return pool[offset:pool.index(0, offset)].decode("ascii")

    by_name = {name(section[0], names): section for section in sections}
    text = by_name[".text"]
    symbols = by_name[".symtab"]
    strings = sections[symbols[6]]
    pool = data[strings[4]:strings[4] + strings[5]]
    values = {}
    if symbols[9] != 24 or symbols[4] + symbols[5] > len(data):
        raise ValueError("invalid ELF symbol table")
    for offset in range(symbols[4], symbols[4] + symbols[5], 24):
        key, _, _, _, address, _ = struct.unpack_from(">IBBHQQ", data, offset)
        values[name(key, pool)] = address
    required = ("lab_high_entry", "lab_tso_start", "lab_high_stack_start",
                "lab_high_stack_end")
    if any(key not in values for key in required):
        raise ValueError("missing high-entry or stack symbol")
    entry, start, low, sp = (values[key] for key in required)
    image_end = max(s[3] + s[5] for s in sections if s[2] & 2)
    if entry != 0 or not 0 < start < 256 or not low < sp or sp & 15:
        raise ValueError("invalid entry or stack layout")
    # The retained startups save a varying first nonvolatile register through
    # R15 with STMG Rx,R15,D(R15). Find the actual prologue rather than
    # assuming RXAS, RXVM and RXC use identical register sets.
    code = data[text[4]:text[4] + text[5]]
    saves = []
    for i in range(start, min(start + 128, len(code) - 5)):
        op = code[i:i + 6]
        if op[0] != 0xeb or op[1] & 15 != 15 or op[2] >> 4 != 15 \
                or op[5] != 0x24:
            continue
        first = op[1] >> 4
        disp = ((op[2] & 15) << 8) | op[3] | (op[4] << 12)
        if first <= 15 and disp + (16 - first) * 8 <= 160:
            saves.append((i, first, disp))
    if not saves:
        raise ValueError("retained C startup save prologue changed")
    if sp + 160 > image_end or image_end - low != 0x100000:
        raise ValueError("initial C save area crosses the high image")
    print(f"HIGH ENTRY CHECK entry={entry:#x} c_start={start:#x} "
          f"stack=[{low:#x},{image_end:#x}) sp={sp:#x} "
          f"caller_reserved={image_end - sp} "
          f"prologue=STMG R{saves[0][1]}-R15+{saves[0][2]}")


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("elf", type=Path)
    args = p.parse_args()
    check(args.elf)


if __name__ == "__main__":
    main()
