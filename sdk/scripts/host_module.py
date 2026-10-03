#!/usr/bin/env python3
"""Build the pinned Linux PDLD writer and package classic MVS modules.

The source archive and native object decks are private build inputs. This tool
never edits them. XMIT is a fixed 80-byte-card transport for TSO RECEIVE;
MVS is PDLD's raw classic unload stream for the separately tested PDOS path.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shutil
import subprocess
import tarfile
import tempfile


SCRIPT = Path(__file__).resolve()
ROOT = SCRIPT.parents[2] if SCRIPT.parent.name == "scripts" and SCRIPT.parent.parent.name == "sdk" else SCRIPT.parents[1]
PATCHES = (
    "sdk/archive/pdld/0001-pdld-only-multi-csect.patch",
    "sdk/archive/pdld/0003-xmit-card-images.patch",
    "sdk/archive/pdld/0004-xmit-source-date-epoch.patch",
    "sdk/archive/pdld/0005-section-relative-rld.patch",
    "sdk/archive/pdld/0006-zero-extended-csect.patch",
)
CARDS = {"ESD", "TXT", "RLD", "END"}


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def put_json(path: Path, value: dict) -> None:
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n")


def run(argv: list[str], log: Path, *, cwd: Path | None = None,
        env: dict[str, str] | None = None, stdin: Path | None = None) -> None:
    with log.open("wb") as output:
        if stdin is None:
            result = subprocess.run(argv, cwd=cwd, env=env,
                                    stdout=output, stderr=subprocess.STDOUT)
        else:
            with stdin.open("rb") as source:
                result = subprocess.run(argv, cwd=cwd, env=env, stdin=source,
                                        stdout=output, stderr=subprocess.STDOUT)
    if result.returncode:
        raise RuntimeError(f"{argv[0]} failed ({result.returncode}); see {log}")


def fresh(path: Path) -> Path:
    path = path.resolve()
    if path.exists():
        raise ValueError(f"output already exists: {path}")
    path.mkdir(parents=True)
    return path


def build(args: argparse.Namespace) -> None:
    if platform.system() != "Linux":
        raise ValueError("the qualified writer build requires Linux")
    archive = args.archive.resolve(strict=True)
    actual = digest(archive)
    if actual != args.archive_sha256.lower():
        raise ValueError(f"source archive SHA-256 mismatch: {actual}")
    out = fresh(args.out)
    source = out / "source"
    source.mkdir()
    with tarfile.open(archive, "r:gz") as tar:
        for item in tar:
            if not item.name.startswith("pdld/"):
                continue
            parts = Path(item.name).parts
            if item.name.startswith("/") or ".." in parts or item.issym() or item.islnk():
                raise ValueError(f"unsafe source archive member: {item.name}")
            if item.isdir():
                continue
            if not item.isfile():
                raise ValueError(f"unsupported archive member: {item.name}")
            target = source.joinpath(*parts)
            target.parent.mkdir(parents=True, exist_ok=True)
            with tar.extractfile(item) as body, target.open("wb") as output:
                shutil.copyfileobj(body, output)
    for index, name in enumerate(PATCHES, 1):
        run(["patch", "-p1", "--batch", "--forward"], out / f"patch-{index}.log",
            cwd=source, stdin=ROOT / name)
    folder = source / "pdld/src"
    sources = sorted(folder.rglob("*.c"))
    if len(sources) != 39:
        raise ValueError(f"expected 39 PDLD C files, got {len(sources)}")
    compiler_name = shutil.which(args.cc)
    if compiler_name is None:
        raise ValueError(f"C compiler not found: {args.cc}")
    compiler = Path(compiler_name).resolve(strict=True)
    writer = out / "pdld-host"
    run([str(compiler), "-O2", "-std=c99", "-iquote", str(folder),
         "-iquote", str(folder / "bytearray"),
         "-iquote", str(folder / "hashtab"),
         "-iquote", str(folder / "ftebc"),
         "-o", str(writer), *(str(path) for path in sources)], out / "build.log")
    manifest = {
        "source_revision": args.source_revision,
        "source_archive_sha256": actual,
        "patch_sha256": {name: digest(ROOT / name) for name in PATCHES},
        "host": platform.platform(),
        "compiler": str(compiler),
        "compiler_sha256": digest(compiler),
        "compiler_version": subprocess.check_output([str(compiler), "--version"], text=True).splitlines()[0],
        "writer_sha256": digest(writer),
        "sources": len(sources),
    }
    put_json(out / "writer-manifest.json", manifest)
    print(f"Linux PDLD writer: {writer} sha256 {manifest['writer_sha256']}")


def check_input(path: Path) -> dict:
    data = path.read_bytes()
    if len(data) >= 80 and len(data) % 80 == 0 and data[0] == 2:
        types = []
        for offset in range(0, len(data), 80):
            card = data[offset:offset + 80]
            if card[0] != 2:
                raise ValueError(f"invalid card marker at {path}:{offset}")
            kind = card[1:4].decode("cp037")
            if kind not in CARDS:
                raise ValueError(f"invalid object card {kind!r} at {path}:{offset}")
            types.append(kind)
        if types[0] != "ESD" or types[-1] != "END":
            raise ValueError(f"incomplete ESD/END deck: {path}")
        return {"kind": "object-cards", "records": len(types)}
    if data[:4] == b"\x7fELF":
        if data[4] != 1 or data[5] != 2 or int.from_bytes(data[18:20], "big") != 22:
            raise ValueError(f"expected 32-bit big-endian S/390 ELF: {path}")
        return {"kind": "s390-elf", "class": data[4]}
    raise ValueError(f"unsupported object input: {path}")


def package(args: argparse.Namespace) -> None:
    writer = args.writer.resolve(strict=True)
    inputs = [path.resolve(strict=True) for path in args.inputs]
    descriptions = [check_input(path) for path in inputs]
    if args.format == "xmit" and args.amode == 31 and args.rmode == 31 and \
            args.entry == "LABTSO":
        # A classic member has one residence attribute. Refuse an override
        # while a native CSECT or relocated field still requires low storage.
        from tso31_deck_check import check_deck, check_pair
        if len(inputs) != 3 or any(item["kind"] != "object-cards"
                                    for item in descriptions):
            raise ValueError("high TSO31 requires entry, C and service decks")
        check_pair(inputs[0], inputs[2])
        check_deck(inputs[1])
    if args.format == "xmit" and args.amode == 64:
        if args.rmode != 31 or args.entry != "LABTS64" or len(inputs) != 3 or \
                any(item["kind"] != "object-cards" for item in descriptions):
            raise ValueError("TSO64 requires the checked entry, C and service decks")
        from tso64_deck_check import check_triple
        check_triple(*inputs)
    if not re.fullmatch(r"[A-Z][A-Z0-9@$#]{0,7}", args.member):
        raise ValueError("member must be an uppercase MVS name of at most eight characters")
    if not re.fullmatch(r"[A-Za-z_@$#][A-Za-z0-9_@$#]*", args.entry):
        raise ValueError("invalid entry symbol")
    if args.format == "xmit" and args.epoch is None:
        raise ValueError("XMIT requires an explicit UTC SOURCE_DATE_EPOCH")
    out = fresh(args.out)
    filename = args.member + (".XMI" if args.format == "xmit" else ".EXE")
    output = out / filename
    env = os.environ.copy()
    if args.epoch is not None:
        env["SOURCE_DATE_EPOCH"] = str(args.epoch)
    run([str(writer), "--oformat", args.format,
         "--amode", str(args.amode), "--rmode", str(args.rmode),
         "-e", args.entry, "-Map", filename + ".map",
         "-o", filename, *(str(path) for path in inputs)],
        out / "pdld.log", cwd=out, env=env)
    if "cannot find entry symbol" in (out / "pdld.log").read_text(errors="replace"):
        raise ValueError(f"entry symbol {args.entry} was not found; see {out / 'pdld.log'}")
    data = output.read_bytes()
    if args.format == "xmit":
        # The writer's final variable-length INMR06 record may begin in the
        # penultimate FB80 card. Only EBCDIC blanks may follow its marker.
        trailer = "INMR06".encode("cp037")
        trailer_at = data.rfind(trailer, max(0, len(data) - 160))
        if (len(data) % 80 or data[2:8] != "INMR01".encode("cp037") or
                trailer_at < 0 or any(byte != 0x40 for byte in
                                      data[trailer_at + len(trailer):])):
            raise ValueError("XMIT lacks complete FB80 card framing")
    result = {
        "format": args.format,
        "member": args.member,
        "entry": args.entry,
        "amode": args.amode,
        "rmode": args.rmode,
        "source_date_epoch": args.epoch,
        "writer": str(writer),
        "writer_sha256": digest(writer),
        "inputs": [{"path": str(path), "sha256": digest(path), **description}
                   for path, description in zip(inputs, descriptions)],
        "output_sha256": digest(output),
        "output_bytes": len(data),
        "map_sha256": digest(out / (filename + ".map")),
    }
    put_json(out / "package-manifest.json", result)
    print(f"{args.format} {args.member}: {output} sha256 {result['output_sha256']}")


def verify(args: argparse.Namespace) -> None:
    package_dir = args.package.resolve(strict=True)
    data = json.loads((package_dir / "package-manifest.json").read_text())
    writer = Path(data["writer"])
    if digest(writer) != data["writer_sha256"]:
        raise ValueError("writer SHA-256 changed")
    paths = [Path(item["path"]) for item in data["inputs"]]
    for path, item in zip(paths, data["inputs"]):
        if digest(path) != item["sha256"] or check_input(path)["kind"] != item["kind"]:
            raise ValueError(f"input changed: {path}")
    filename = data["member"] + (".XMI" if data["format"] == "xmit" else ".EXE")
    if digest(package_dir / filename) != data["output_sha256"]:
        raise ValueError("packaged output SHA-256 changed")
    with tempfile.TemporaryDirectory(prefix="host-module-verify-") as temporary:
        replay = argparse.Namespace(writer=writer, inputs=paths, out=Path(temporary) / "package",
                                    member=data["member"], entry=data["entry"],
                                    format=data["format"], amode=data["amode"],
                                    rmode=data["rmode"], epoch=data["source_date_epoch"])
        package(replay)
        if digest(replay.out / filename) != data["output_sha256"]:
            raise ValueError("fresh consumer produced different bytes")
    print(f"Fresh consumer PASS: {data['member']} {data['output_sha256']}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    b = commands.add_parser("build")
    b.add_argument("--archive", type=Path, required=True)
    b.add_argument("--archive-sha256", required=True)
    b.add_argument("--source-revision", required=True)
    b.add_argument("--cc", default="cc")
    b.add_argument("--out", type=Path, required=True)
    p = commands.add_parser("package")
    p.add_argument("--writer", type=Path, required=True)
    p.add_argument("--format", choices=("xmit", "mvs"), required=True)
    p.add_argument("--amode", type=int, choices=(24, 31, 64), required=True)
    p.add_argument("--rmode", type=int, choices=(24, 31), required=True)
    p.add_argument("--entry", required=True)
    p.add_argument("--member", required=True)
    p.add_argument("--epoch", type=int)
    p.add_argument("--out", type=Path, required=True)
    p.add_argument("inputs", nargs="+", type=Path)
    v = commands.add_parser("verify")
    v.add_argument("--package", type=Path, required=True)
    args = parser.parse_args()
    {"build": build, "package": package, "verify": verify}[args.command](args)


if __name__ == "__main__":
    main()
