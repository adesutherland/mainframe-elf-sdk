#!/usr/bin/env python3
"""Extract and reconstruct the pinned modern compiler SDK source.

This file is copied into the extraction. Every command after ``extract`` runs
from that extraction, with no reference to the Mainframe Lab checkout.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import platform
import shutil
import subprocess
import sys
import tarfile
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LOCK = {
    "gcc": {"file": "gcc-16.2.0.tar.xz", "sha256": "e6738e29597f733270731aa90600f37ffdc045079dfc27ec7e8192cc81085c3e", "url": "https://ftp.gnu.org/gnu/gcc/gcc-16.2.0/gcc-16.2.0.tar.xz"},
    "binutils": {"file": "binutils-2.47.tar.xz", "sha256": "154ab23b60070e8f27013c22977f1129425d67d1e8acd6e13010e617811e4cff", "url": "https://ftp.gnu.org/gnu/binutils/binutils-2.47.tar.xz"},
    "newlib": {"file": "newlib-4.6.0.20260123.tar.gz", "sha256": "6ff27e3bf022666f43f7802255be680eeff722ac181b1725d21e2e8318604ee3", "url": "https://sourceware.org/pub/newlib/newlib-4.6.0.20260123.tar.gz"},
}
ACTIVE = ("vm370-4381-v1", "cms20-esa31-v1", "tso-zos24-v1", "tso-zos31-v1", "tso-zos64-v1", "vmkernel")
ENTRY_POINTS = (("sdk/PROJECT-README.md", "README.md"),
                ("sdk/LICENSING.md", "LICENSING.md"),
                ("sdk/.gitignore", ".gitignore"),
                ("sdk/LICENSE-MIT", "LICENSE"))


def sha(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def run(command: list[str], *, cwd: Path, log: Path | None = None) -> None:
    print("+", " ".join(command), flush=True)
    if log:
        log.parent.mkdir(parents=True, exist_ok=True)
        with log.open("w") as output:
            result = subprocess.run(command, cwd=cwd, stdout=output, stderr=subprocess.STDOUT)
        if result.returncode:
            print(log.read_text(errors="replace")[-8000:], file=sys.stderr)
    else:
        result = subprocess.run(command, cwd=cwd)
    if result.returncode:
        raise RuntimeError(f"command failed ({result.returncode}): {command[0]}")


def files() -> list[str]:
    return [line for line in (ROOT / "sdk/source-files.txt").read_text().splitlines()
            if line and not line.startswith("#")]


def extract(out: Path) -> None:
    if out.exists():
        raise ValueError(f"refusing existing extraction: {out}")
    names = files()
    if len(names) != len(set(names)):
        raise ValueError("duplicate source inventory entry")
    out.mkdir(parents=True)
    hashes = {}
    for name in names:
        path = Path(name)
        if path.is_absolute() or ".." in path.parts:
            raise ValueError(f"unsafe source name: {name}")
        source = ROOT / path
        if not source.is_file() or source.is_symlink():
            raise ValueError(f"missing or symlinked source: {name}")
        target = out / path
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source, target)
        hashes[name] = sha(target)
    # Public repository entry points are authored in sdk/ so extraction never
    # has to copy this laboratory's root README, licence policy or ignore list.
    for source_name, target_name in ENTRY_POINTS:
        if source_name not in hashes:
            raise ValueError(f"source inventory omits {source_name}")
        shutil.copyfile(out / source_name, out / target_name)
        hashes[target_name] = sha(out / target_name)
    if "tools/compiler_sdk.py" not in hashes:
        raise ValueError("source inventory omits the producer")
    manifest = {"format": "mainframe-compiler-sdk-source-v1", "active_profiles": ACTIVE,
                "retired_profile": "vmce-cms-kernel-v1", "files_sha256": hashes,
                "upstream": LOCK, "source_only": True}
    (out / "SOURCE-MANIFEST.json").write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n")
    print(f"Extracted {len(hashes)} source files: {out}")


def refresh_manifest() -> None:
    """Refresh the checked inventory in an extracted standalone source repo."""
    if not (ROOT / "SOURCE-MANIFEST.json").is_file():
        raise ValueError("refresh-manifest requires an extracted source root")
    names = files()
    if len(names) != len(set(names)):
        raise ValueError("duplicate source inventory entry")
    hashes = {}
    for name in names:
        path = Path(name)
        if path.is_absolute() or ".." in path.parts:
            raise ValueError(f"unsafe source name: {name}")
        source = ROOT / path
        if not source.is_file() or source.is_symlink():
            raise ValueError(f"missing or symlinked source: {name}")
        hashes[name] = sha(source)
    for source_name, target_name in ENTRY_POINTS:
        if source_name not in hashes:
            raise ValueError(f"source inventory omits {source_name}")
        hashes[target_name] = hashes[source_name]
    actual = {str(path.relative_to(ROOT)) for path in ROOT.rglob("*")
              if path.is_file() and not any(part in ("build", ".git", "__pycache__")
                                                for part in path.relative_to(ROOT).parts)}
    expected = set(hashes) | {"SOURCE-MANIFEST.json"}
    if actual != expected:
        raise ValueError(f"source tree differs: missing {sorted(expected - actual)}, "
                         f"extra {sorted(actual - expected)}")
    for source_name, target_name in ENTRY_POINTS:
        shutil.copyfile(ROOT / source_name, ROOT / target_name)
    manifest = {"format": "mainframe-compiler-sdk-source-v1", "active_profiles": ACTIVE,
                "retired_profile": "vmce-cms-kernel-v1", "files_sha256": hashes,
                "upstream": LOCK, "source_only": True}
    (ROOT / "SOURCE-MANIFEST.json").write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n")
    verify_source()
    print(f"Refreshed {len(hashes)} source files: {ROOT / 'SOURCE-MANIFEST.json'}")


def verify_source() -> dict:
    manifest = json.loads((ROOT / "SOURCE-MANIFEST.json").read_text())
    actual = {str(p.relative_to(ROOT)) for p in ROOT.rglob("*") if p.is_file() and
              not any(part in ("build", ".git", "__pycache__") for part in p.relative_to(ROOT).parts)}
    expected = set(manifest["files_sha256"]) | {"SOURCE-MANIFEST.json"}
    if actual != expected:
        raise ValueError(f"source tree differs: missing {sorted(expected - actual)}, extra {sorted(actual - expected)}")
    for name, digest in manifest["files_sha256"].items():
        if sha(ROOT / name) != digest:
            raise ValueError(f"changed source: {name}")
    return manifest


def acquire(cache: Path, name: str, *, download: bool) -> Path:
    pin = LOCK[name]
    path = cache / pin["file"]
    if not path.is_file():
        if not download:
            raise FileNotFoundError(f"missing pinned archive: {path}")
        cache.mkdir(parents=True, exist_ok=True)
        partial = path.with_suffix(path.suffix + ".part")
        with urllib.request.urlopen(pin["url"], timeout=180) as source, partial.open("wb") as target:
            shutil.copyfileobj(source, target)
        if sha(partial) != pin["sha256"]:
            partial.unlink()
            raise ValueError(f"download hash mismatch: {name}")
        partial.rename(path)
    if sha(path) != pin["sha256"]:
        raise ValueError(f"archive hash mismatch: {path}")
    return path


def untar(archive: Path, destination: Path) -> None:
    destination.mkdir(parents=True, exist_ok=True)
    with tarfile.open(archive) as stream:
        stream.extractall(destination, filter="data")


def check_sum_list(root: Path, list_file: Path, *, strip_prefix: str = "") -> None:
    for line in list_file.read_text().splitlines():
        digest, name = line.split(None, 1)
        name = name.strip()
        if strip_prefix:
            if not name.startswith(strip_prefix):
                raise ValueError(f"unexpected hash-list path: {name}")
            name = name[len(strip_prefix):]
        path = root / name
        if not path.is_file() or sha(path) != digest:
            raise ValueError(f"source hash mismatch: {path}")


def prepare(cache: Path, work: Path, *, download: bool) -> None:
    verify_source()
    if work.exists():
        raise ValueError(f"refusing existing work root: {work}")
    archives = {name: acquire(cache, name, download=download) for name in LOCK}
    work.mkdir(parents=True)
    for archive in archives.values():
        untar(archive, work / "source")
    gcc = work / "source/gcc-16.2.0"
    check_sum_list(gcc, ROOT / "toolchain/gcc16/inputs.sha256")
    patches = ROOT / "patches/gcc16/maintained"
    check_sum_list(patches, patches / "patches.sha256")
    for name in (patches / "series").read_text().splitlines():
        run(["patch", "--batch", "--forward", "-F0", "-p1", "-i", str(patches / name)], cwd=gcc,
            log=work / "logs" / f"gcc-{name}.log")
    check_sum_list(gcc, ROOT / "toolchain/gcc16/source.sha256")
    for name in (ROOT / "toolchain/gcc16/source.sha256").read_text().splitlines():
        path = name.split(None, 1)[1].strip()
        if (gcc / path).read_bytes() != (ROOT / "toolchain/gcc16/source" / path).read_bytes():
            raise ValueError(f"editable GCC source differs from recovery: {path}")
    newlib = work / "source/newlib-4.6.0.20260123"
    check_sum_list(newlib, ROOT / "patches/newlib/base.sha256",
                   strip_prefix="vendor/newlib-4.6.0.20260123/")
    run(["patch", "--batch", "--forward", "-p1", "-i", str(ROOT / "patches/newlib/0001-cms-elf-poc.patch")], cwd=newlib,
        log=work / "logs/newlib-patch.log")
    check_sum_list(newlib, ROOT / "patches/newlib/patched.sha256",
                   strip_prefix="build/compilers/libc-poc/newlib/source/")
    result = {"format": "mainframe-compiler-sdk-preparation-v1", "sources_sha256":
              {name: sha(path) for name, path in archives.items()},
              "gcc_recovery_series_sha256": sha(patches / "series"),
              "newlib_patch_sha256": sha(ROOT / "patches/newlib/0001-cms-elf-poc.patch")}
    (work / "preparation.json").write_text(json.dumps(result, indent=2, sort_keys=True) + "\n")
    print(f"Pinned source reconstructed: {work}")


def build_tools(work: Path, cc: str, cxx: str, jobs: int) -> None:
    if not (work / "preparation.json").is_file():
        raise ValueError("run prepare first")
    if (work / "install").exists():
        raise ValueError("refusing existing tool installation")
    host = f"{platform.system().lower()}-{platform.machine()}"
    env = os.environ.copy()
    env.update(CC=cc, CXX=cxx, CFLAGS="-O2", CXXFLAGS="-O2")
    bin_source = work / "source/binutils-2.47"
    bin_obj = work / "obj/binutils"
    bin_obj.mkdir(parents=True)
    bin_prefix = work / "install/binutils"
    gcc_source = work / "source/gcc-16.2.0"
    gcc_obj = work / "obj/gcc"
    gcc_obj.mkdir(parents=True)
    gcc_prefix = work / "install/gcc"
    def call(args: list[str], cwd: Path, log: str) -> None:
        with (work / "logs" / log).open("w") as output:
            print("+", " ".join(args), flush=True)
            result = subprocess.run(args, cwd=cwd, env=env, stdout=output, stderr=subprocess.STDOUT)
        if result.returncode:
            print((work / "logs" / log).read_text(errors="replace")[-8000:], file=sys.stderr)
            raise RuntimeError(f"{log} failed ({result.returncode})")
    call(["/bin/sh", str(bin_source / "configure"), "--target=s390-linux-gnu", "--enable-obsolete",
          "--disable-nls", "--disable-multilib", "--disable-gdb", "--disable-sim",
          f"--prefix={bin_prefix}"], bin_obj, "binutils-configure.log")
    call(["make", f"-j{jobs}", "all-binutils", "all-gas", "all-ld"], bin_obj, "binutils-build.log")
    call(["make", "install-binutils", "install-gas", "install-ld"], bin_obj, "binutils-install.log")
    configure = ["/bin/sh", str(gcc_source / "configure"), "--target=s390-linux-gnu",
                 "--enable-languages=c", "--enable-obsolete", "--disable-bootstrap", "--disable-nls",
                 "--disable-multilib", "--disable-shared", "--disable-threads", "--without-headers",
                 f"--prefix={gcc_prefix}"]
    if platform.system() == "Darwin":
        for name, prefix in (("gmp", "/opt/homebrew/opt/gmp"), ("mpfr", "/opt/homebrew/opt/mpfr"),
                             ("mpc", "/opt/homebrew/opt/libmpc"), ("isl", "/opt/homebrew/opt/isl")):
            if not Path(prefix).is_dir():
                raise ValueError(f"missing host prerequisite: {prefix}")
            configure.append(f"--with-{name}={prefix}")
    call(configure, gcc_obj, "gcc-configure.log")
    call(["make", f"-j{jobs}", "all-gcc"], gcc_obj, "gcc-build.log")
    call(["make", "install-gcc"], gcc_obj, "gcc-install.log")
    binaries = {}
    for name in ("s390-linux-gnu-as", "s390-linux-gnu-ld", "s390-linux-gnu-ar", "s390-linux-gnu-objdump", "s390-linux-gnu-readelf"):
        path = bin_prefix / "bin" / name
        binaries[name] = sha(path)
    binaries["s390-linux-gnu-gcc"] = sha(gcc_prefix / "bin/s390-linux-gnu-gcc")
    binaries["cc1"] = sha(next((gcc_prefix / "libexec/gcc/s390-linux-gnu").rglob("cc1")))
    receipt = {"format": "mainframe-compiler-sdk-tools-v1", "host": host, "compiler": cc,
               "cxx": cxx, "target": "s390-linux-gnu", "binaries_sha256": binaries,
               "preparation_sha256": sha(work / "preparation.json")}
    (work / "tools.json").write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n")
    print(f"Fresh tools built: {work / 'install'}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    e = commands.add_parser("extract")
    e.add_argument("--out", type=Path, required=True)
    commands.add_parser("refresh-manifest")
    p = commands.add_parser("prepare")
    p.add_argument("--cache", type=Path, required=True)
    p.add_argument("--work", type=Path, required=True)
    p.add_argument("--download", action="store_true")
    b = commands.add_parser("build-tools")
    b.add_argument("--work", type=Path, required=True)
    b.add_argument("--cc", default="gcc-16" if platform.system() == "Darwin" else "gcc")
    b.add_argument("--cxx", default="g++-16" if platform.system() == "Darwin" else "g++")
    b.add_argument("--jobs", type=int, default=min(6, os.cpu_count() or 1))
    args = parser.parse_args()
    if args.command == "extract":
        extract(args.out.resolve())
    elif args.command == "refresh-manifest":
        refresh_manifest()
    elif args.command == "prepare":
        prepare(args.cache.resolve(), args.work.resolve(), download=args.download)
    else:
        build_tools(args.work.resolve(), args.cc, args.cxx, args.jobs)


if __name__ == "__main__":
    main()
