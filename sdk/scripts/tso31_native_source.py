#!/usr/bin/env python3
"""Prepare the pinned, lean TSO31 native service source without editing PDPCLIB.

ASMA90 and the IBM macros remain native, private build prerequisites. The
output is a checked source delta, not an assembled object or qualified module.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess


ROOT = Path(__file__).resolve().parents[2]
PATCHES = (
    ROOT / "sdk/archive/pdpclib/0001-pdse-no-lstar-check.patch",
    ROOT / "sdk/archive/pdpclib/0002-tso31-omit-prefix-parser.patch",
    ROOT / "sdk/archive/pdpclib/0003-tso31-dynal-low-request-pointers.patch",
)
REQUIRED = ("@@AOPEN", "@@AREAD", "@@AWRITE", "@@ACLOSE", "@@DYNAL")


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def expected_upstream_hash() -> str:
    manifest = (ROOT / "libc/src/adapters/tso/pdpclib-source.sha256").read_text().strip()
    expected, name = manifest.split()
    if name != "vendor/pdos/pdpclib/mvssupa.asm" or not re.fullmatch(
        r"[0-9a-f]{64}", expected
    ):
        raise ValueError("invalid pinned PDPCLIB source identity")
    return expected


def prepare(upstream: Path, out: Path) -> None:
    upstream = upstream.resolve(strict=True)
    upstream_hash = sha(upstream)
    if upstream_hash != expected_upstream_hash():
        raise ValueError(f"PDPCLIB source SHA-256 mismatch: {upstream_hash}")
    out = out.resolve()
    if out.exists():
        raise ValueError(f"output already exists: {out}")
    out.mkdir(parents=True)
    source = out / "pdpclib/mvssupa.asm"
    source.parent.mkdir()
    shutil.copyfile(upstream, source)
    for number, patch in enumerate(PATCHES, 1):
        with (out / f"patch-{number}.log").open("wb") as log:
            subprocess.run(
                ["patch", "--batch", "--forward", "--fuzz=0", "-p1",
                 "-i", str(patch)],
                cwd=out, stdout=log, stderr=subprocess.STDOUT, check=True,
            )
    text = source.read_text()
    for name in REQUIRED:
        if not re.search(r"^" + re.escape(name) + r"\s+FUNHEAD\b",
                         text, re.MULTILINE):
            raise ValueError(f"retained native service is missing: {name}")
    if (re.search(r"^\s*(?:ENTRY\s+)?@@(?:GETEPF|PCLST)\s", text,
                  re.MULTILINE) or
            re.search(r"^\s*(?:CALLTSSR|IKJPARM|IKJPOSIT|IKJENDP)\b",
                      text, re.MULTILINE) or "DSNCBOA" in text):
        raise ValueError("TSO prefix parser remains in the lean source")
    for target in ("ALLARBP", "ALLURBP"):
        if not re.search(r"^\s*ST\s+R0," + target + r"\b", text,
                         re.MULTILINE) or re.search(
                             r"^\s*STCM\s+R0,7," + target + r"\+1\b",
                             text, re.MULTILINE):
            raise ValueError(f"dynamic request pointer remains partial: {target}")
    for name in ("entry31.asm", "pdptop.mac"):
        shutil.copyfile(ROOT / "libc/src/adapters/tso" / name, out / name)
    manifest = {
        "format": "mainframe-lab-tso31-lean-native-source-v1",
        "upstream_sha256": upstream_hash,
        "patches_sha256": {patch.name: sha(patch) for patch in PATCHES},
        "source_sha256": sha(source),
        "entry_sha256": sha(out / "entry31.asm"),
        "pdptop_sha256": sha(out / "pdptop.mac"),
        "retained_services": list(REQUIRED),
    }
    (out / "source-manifest.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n"
    )
    print(f"TSO31 lean native source: {source} sha256 {manifest['source_sha256']}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--upstream", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    args = parser.parse_args()
    prepare(args.upstream, args.out)


if __name__ == "__main__":
    main()
