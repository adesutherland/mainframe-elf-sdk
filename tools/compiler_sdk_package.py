#!/usr/bin/env python3
"""Install a profile-separated local SDK from fresh tools and checked runtimes.

Runtime archives and native object decks are explicit, pinned bootstrap inputs.
Their use is recorded in the package manifest, never called a source rebuild.
"""

from __future__ import annotations

import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tarfile

from compiler_sdk import ACTIVE, ROOT, sha
from host_module import PATCHES as PDLD_PATCHES

RUNTIME_ARCHIVES = {
    "cms24": ("70c1ba809475b2c25cb8962fece0068bcd5f08a20a443fff6211b2698826b072", "cms24", "vm370-4381-v1"),
    "cms31": ("23fe90a16eac4d4d3256f65fd3ae9b2506edce5e420b43e75c0570d4a5d78447", "cms31", "cms20-esa31-v1"),
    "mvs": ("444ed72e1dee293f08ee9b837ee352b9526eab8d51d016d829b4cc7e752ec37e", "mvs-newlib-2026-09-24-v1", None),
}
NATIVE_SHA = {
    "TENTRY.obj": "9d1b7813847ad9f27b67f8401dac5f3972d14216c8794460f6a76d6c10d25270",
    "PDPSUP-tso.obj": "79c232b808201729d54d62f5ba1017dee5fc68decb6ae4790141cf819509f182",
}
ANY_NATIVE_SHA = {
    "PDPL34.obj": "2e513bb7535c1a7fceecd5954790845cd3d4affb521a68644fcf773c0e6c8e53",
    "E64.obj": "00088a98ea3392363464422b0bbc7bf054f5e92304fbdfa48fc6fc3a7b6a0df0",
}
HIGH_LAUNCHER_SHA = {
    "LAU65O.obj": "f0edb7af6caf446259c5f5de303c0ac570c63a173d58e161c5b526bd5e53bcdb",
    "LAVM65O.obj": "66ecc7b09dc775ff89b68a66fd2dbafb886e6d1860821467b2618d0a8e9fa7fc",
    "LAC65O.obj": "1a5bb86fe381b0ea1de813bc179fc9dcd3d6f38996b25a62daff8855cc9b85fe",
}


def unpack_checked(archive: Path, expected: str, out: Path) -> None:
    if sha(archive) != expected:
        raise ValueError(f"archive SHA-256 mismatch: {archive}")
    with tarfile.open(archive) as stream:
        stream.extractall(out, filter="data")


def verify_sums(root: Path) -> None:
    entries = (root / "SHA256SUMS").read_text().splitlines()
    for entry in entries:
        digest, name = entry.split("  ", 1)
        item = root / name
        if not item.is_file() or sha(item) != digest:
            raise ValueError(f"runtime inventory mismatch: {item}")


def copy_files(source: Path, dest: Path, names: tuple[str, ...]) -> None:
    dest.mkdir(parents=True)
    for name in names:
        item = source / name
        if not item.exists():
            raise ValueError(f"missing installed tool: {item}")
        if item.is_dir():
            shutil.copytree(item, dest / name, symlinks=True,
                            ignore=shutil.ignore_patterns("._*", ".DS_Store"))
        else:
            shutil.copy2(item, dest / name)


def build_host_helpers(out: Path, cc: str) -> dict:
    names = {"s370_check": "s370_check.c", "tso64_check": "tso64_check.c",
             "elf_to_cms_text": "elf_to_cms.c", "elf_to_cms_module": "elf_to_cms.c",
             "elf_to_cms_module31": "elf_to_cms.c", "elf_to_mvs": "elf_to_mvs.c",
             "elf_to_mvs24": "elf_to_mvs24.c", "elf_to_mvs64": "elf_to_mvs64.c",
             "elf_to_vmce_asm": "elf_to_vmce_asm.c"}
    result = {}
    for name, source in names.items():
        target = out / "host/bin" / name
        command = [cc, "-std=c11", "-D_POSIX_C_SOURCE=200809L", "-O2", "-Wall", "-Wextra", "-Werror",
                   str(ROOT / "tools" / source), "-o", str(target)]
        if name.startswith("elf_to_cms_module"):
            command.insert(-2, "-DMODULE_OUTPUT")
        if name == "elf_to_cms_module31":
            command.insert(-2, "-DRELOCATABLE_MODULE31")
        subprocess.run(command, check=True)
        result[name] = sha(target)
    return result


def tar_package(directory: Path, archive: Path) -> None:
    with archive.open("wb") as destination, gzip.GzipFile(fileobj=destination, mode="wb", filename="", mtime=0) as zipped:
        with tarfile.open(fileobj=zipped, mode="w", format=tarfile.PAX_FORMAT) as stream:
            for path in sorted(directory.rglob("*")):
                if path.name.startswith("._") or path.name == ".DS_Store":
                    continue
                info = stream.gettarinfo(path, arcname=str(Path(directory.name) / path.relative_to(directory)))
                info.uid = info.gid = info.mtime = 0
                info.uname = info.gname = ""
                info.mode = 0o755 if path.is_dir() or os.access(path, os.X_OK) else 0o644
                if path.is_file() and not path.is_symlink():
                    with path.open("rb") as body:
                        stream.addfile(info, body)
                else:
                    stream.addfile(info)


def main() -> None:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--work", type=Path, required=True)
    p.add_argument("--cms24", type=Path, required=True)
    p.add_argument("--cms31", type=Path, required=True)
    p.add_argument("--mvs", type=Path, required=True)
    p.add_argument("--native-decks", type=Path, required=True)
    p.add_argument("--native-service-any", type=Path, required=True)
    p.add_argument("--native-entry64-any", type=Path, required=True)
    p.add_argument("--native-high-launchers", type=Path, required=True)
    p.add_argument("--out", type=Path, required=True)
    p.add_argument("--cc", default="cc")
    a = p.parse_args()
    out = a.out.resolve()
    if out.exists() or Path(str(out) + ".tar.gz").exists():
        p.error("refusing existing SDK destination")
    work = a.work.resolve(strict=True)
    tool_receipt = json.loads((work / "tools.json").read_text())
    if tool_receipt["format"] != "mainframe-compiler-sdk-tools-v1":
        p.error("unrecognized tool producer receipt")
    out.mkdir(parents=True)
    stage = out / "input-stage"
    stage.mkdir()
    archives = {key: getattr(a, key).resolve(strict=True) for key in RUNTIME_ARCHIVES}
    for key, archive in archives.items():
        unpack_checked(archive, RUNTIME_ARCHIVES[key][0], stage / key)
    roots = {key: stage / key / RUNTIME_ARCHIVES[key][1] for key in RUNTIME_ARCHIVES}
    for root in roots.values():
        verify_sums(root)
    sysroots = out / "sysroots"
    sysroots.mkdir()
    for key, profile in (("cms24", "vm370-4381-v1"), ("cms31", "cms20-esa31-v1")):
        package = roots[key]
        if json.loads((package / "profile.json").read_text())["name"] != profile:
            raise ValueError(f"wrong {key} profile")
        target = sysroots / profile
        target.mkdir()
        for name in ("include", "lib", "startup.o", "profile.json", "notices", "SOURCE-PINS.txt"):
            source = package / name
            if source.is_dir():
                shutil.copytree(source, target / name,
                                ignore=shutil.ignore_patterns("._*", ".DS_Store"))
            else:
                shutil.copy2(source, target / name)
    for bits in (24, 31, 64):
        profile = f"tso-zos{bits}-v1"
        source = roots["mvs"] / f"tso{bits}"
        if json.loads((source / "profile.json").read_text())["name"] != profile:
            raise ValueError(f"wrong {profile} runtime package")
        shutil.copytree(source, sysroots / profile,
                        ignore=shutil.ignore_patterns("._*", ".DS_Store"))
        shutil.copy2(ROOT / "toolchain/profiles" / f"{profile}.json",
                     sysroots / profile / "profile.json")
    kernel = sysroots / "vmkernel"
    kernel.mkdir()
    shutil.copy2(ROOT / "toolchain/profiles/vmkernel.json", kernel / "profile.json")
    (kernel / "include/cms").mkdir(parents=True)
    shutil.copy2(ROOT / "systems/vmce/kernel-c/cms_storage.h", kernel / "include/cms/cms_storage.h")
    (kernel / "components/cms").mkdir(parents=True)
    shutil.copy2(ROOT / "systems/vmce/kernel-c/storage.c", kernel / "components/cms/storage.c")
    for name in ("FSTPROD.ASSEMBLE", "FSTGLUE.ASSEMBLE"):
        shutil.copy2(ROOT / "systems/vmce/kernel-c" / name, kernel / "components/cms" / name)
    # The retired profile is retained as source evidence, not an enabled sysroot.
    shutil.copytree(ROOT / "toolchain/profiles", out / "profiles",
                    ignore=shutil.ignore_patterns("._*", ".DS_Store"))
    copy_files(work / "install/gcc", out / "host/gcc", ("bin", "libexec", "lib", "share"))
    copy_files(work / "install/binutils", out / "host/binutils", ("bin", "lib", "share", "s390-linux-gnu"))
    (out / "host/bin").mkdir()
    helpers = build_host_helpers(out, a.cc)
    # Packaging/consumer tools and maintained source adapters are portable SDK inputs.
    tools = out / "tools"
    tools.mkdir()
    for name in ("compiler_sdk_consume.py", "host_module.py", "tso64_deck_check.py",
                 "tso31_deck_check.py", "pdos64_xmit_unload.py"):
        shutil.copy2(ROOT / "tools" / name, tools / name)
    (out / "patches/pdos390").mkdir(parents=True)
    for patch in PDLD_PATCHES:
        shutil.copy2(ROOT / patch, out / patch)
    shutil.copytree(ROOT / "runtime/tso64", out / "adapters/tso64",
                    ignore=shutil.ignore_patterns("._*", ".DS_Store"))
    shutil.copytree(ROOT / "runtime/cms", out / "adapters/cms",
                    ignore=shutil.ignore_patterns("._*", ".DS_Store"))
    copy_files(ROOT / "tests/tso", out / "contracts/tso",
               ("image.ld", "image64.ld", "image64-high.ld", "check_high_entry.py"))
    shutil.copytree(ROOT / "tests/profiles", out / "contracts/profiles",
                    ignore=shutil.ignore_patterns("._*", ".DS_Store"))
    shutil.copy2(ROOT / "sdk/README.md", out / "README.md")
    native_archive = a.native_decks.resolve(strict=True)
    unpack_checked(native_archive, "14599336f90b528439f5c06e8f4e173991d91fde2bb66d137b53f57af2b0848f", stage / "native")
    native = stage / "native/tso-native-decks-v1"
    for name, digest in NATIVE_SHA.items():
        if sha(native / name) != digest:
            raise ValueError(f"native deck hash mismatch: {name}")
    shutil.copytree(native, out / "native/tso31-bootstrap",
                    ignore=shutil.ignore_patterns("._*", ".DS_Store"))
    any_inputs = {"PDPL34.obj": a.native_service_any.resolve(strict=True),
                  "E64.obj": a.native_entry64_any.resolve(strict=True)}
    (out / "native/accepted-any").mkdir()
    for name, path in any_inputs.items():
        if sha(path) != ANY_NATIVE_SHA[name]:
            raise ValueError(f"accepted native object hash mismatch: {name}")
        shutil.copy2(path, out / "native/accepted-any" / name)
    high_launchers = a.native_high_launchers.resolve(strict=True)
    (out / "native/high-launchers").mkdir()
    for name, digest in HIGH_LAUNCHER_SHA.items():
        path = high_launchers / name
        if sha(path) != digest:
            raise ValueError(f"accepted high launcher object hash mismatch: {name}")
        shutil.copy2(path, out / "native/high-launchers" / name)
    copy_files(ROOT / "runtime/tso", out / "adapters/tso",
               ("entry24.asm", "entry31-any.asm", "entry31.asm", "io.c",
                "native-args.h", "native-path.c", "pdpclib-source.sha256",
                "pdptop.mac", "sdk-path.h", "services.h", "signals.c"))
    shutil.copy2(ROOT / "runtime/tso/sdk-application.c", out / "adapters/tso/application.c")
    shutil.copytree(ROOT / "patches/pdpclib", out / "adapters/pdpclib-patches",
                    ignore=shutil.ignore_patterns("._*", ".DS_Store"))
    upstream_pdp = roots["mvs"] / "source/vendor/pdos/pdpclib/mvssupa.asm"
    if sha(upstream_pdp) != "27758af986baae46fb726fe9bb35ed993898b98d547e315587328940c2f41e31":
        raise ValueError("PDPCLIB source differs")
    shutil.copy2(upstream_pdp, out / "adapters/tso/mvssupa-upstream.asm")
    shutil.rmtree(stage)
    profiles = {}
    for name in ACTIVE:
        profile_path = out / "profiles" / f"{name}.json"
        data = json.loads(profile_path.read_text())
        widths = {"vm370-4381-v1": 24, "cms20-esa31-v1": 31, "tso-zos24-v1": 24,
                  "tso-zos31-v1": 31, "tso-zos64-v1": 64, "vmkernel": 24}
        profiles[name] = {"profile_sha256": sha(profile_path), "object_identity": data["object_identity"],
                          "sysroot": f"sysroots/{name}", "address_bits": widths[name],
                          "actions": (["compile", "component"] if name == "vmkernel" else
                                      ["compile", "link", "module"] if name.startswith(("vm370", "cms20")) else
                                      ["compile", "link", "native-deck"] if name == "tso-zos24-v1" else
                                      ["compile", "link", "native-deck", "xmit-with-external-pdld"]),
                          "native_execution": "retained bounded evidence only; new host package not yet guest executed"}
    for name in ("tso-zos31-v1", "tso-zos64-v1"):
        profiles[name]["xmit_prerequisite"] = (
            "separately built, checked PDLD; this SDK includes shared source/patches and "
            "unload helper but no installed PDLD executable")
    profiles["tso-zos64-v1"]["residence_routes"] = {
        "default": "RMODE ANY, image below 2 GiB; accepted z/OS 1.5 route",
        "high": "low launcher plus RMODE64 high code; accepted PDOS problem-state proof, modern z/OS unverified; native adapter regeneration remains open"}
    input_hashes = {key: sha(path) for key, path in archives.items()}
    input_hashes["native_decks"] = sha(native_archive)
    files = {str(path.relative_to(out)): sha(path) for path in sorted(out.rglob("*"))
             if path.is_file() and not path.is_symlink() and not path.name.startswith("._")}
    receipt = {"format": "mainframe-compiler-sdk-local-v1", "version": "0.1.0-local", "host": tool_receipt["host"],
               "tool_producer": tool_receipt, "source_manifest_sha256": sha(ROOT / "SOURCE-MANIFEST.json"),
               "runtime_bootstrap_archives_sha256": input_hashes,
               "native_bootstrap_objects_sha256": {**NATIVE_SHA, **ANY_NATIVE_SHA, **HIGH_LAUNCHER_SHA},
               "host_helpers_sha256": helpers, "profiles": profiles, "files_sha256": files,
               "source_rebuild_gap": "runtime archives reused as checked bootstrap inputs; native TSO adapters assembled with ASMA90; no complete open-source regeneration demonstrated",
               "pdos_application_target": "excluded"}
    (out / "SDK-MANIFEST.json").write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n")
    archive = Path(str(out) + ".tar.gz")
    tar_package(out, archive)
    print(f"SDK {archive} SHA-256 {sha(archive)}")


if __name__ == "__main__":
    main()
