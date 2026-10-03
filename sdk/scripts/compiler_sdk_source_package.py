#!/usr/bin/env python3
"""Package source-built SDK sysroots, tools and native bridge objects.

This is a local candidate producer. Its inputs must come from the checked
source recipes; guest and second-host qualification are separate gates.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import shutil

from compiler_sdk import ACTIVE, ROOT, sha, verify_source
from compiler_sdk_package import build_host_helpers, copy_files, tar_package


SYSROOTS = {
    "cms24": "vm370-4381-v1",
    "cms31": "cms20-esa31-v1",
    "tso24": "tso-zos24-v1",
    "tso31": "tso-zos31-v1",
    "tso64": "tso-zos64-v1",
}
ENTRY_OBJECTS = ("tso24-entry.obj", "tso31-any-entry.obj",
                 "tso64-low-entry.obj", "tso64-any-entry.obj")
ZPDOS_COMPONENTS = ("assembler", "linker", "pdpclib")


def source_directory(path: Path, expected: str) -> Path:
    source = path.resolve(strict=True)
    if json.loads((source / "profile.json").read_text())["name"] != expected:
        raise ValueError(f"wrong source-built sysroot: {source}")
    for name in ("include", "lib", "notices"):
        if not (source / name).is_dir():
            raise ValueError(f"incomplete sysroot: {source / name}")
    if expected.startswith(("vm370", "cms20")) and not (source / "startup.o").is_file():
        raise ValueError(f"missing CMS startup: {source}")
    return source


def copy_native(native: Path, out: Path) -> dict[str, str]:
    source = native.resolve(strict=True)
    target = out / "native/source"
    target.mkdir(parents=True)
    inputs = {"mvssupa.obj": source / "mvssupa.obj"}
    inputs.update({name: source / "entries" / name for name in ENTRY_OBJECTS})
    for name, path in inputs.items():
        if not path.is_file():
            raise ValueError(f"missing source-built native object: {path}")
        shutil.copy2(path, target / name)
    for name in ("mvssupa-source.sha256", "tso31-native-check.json",
                 "tso64-native-check.json"):
        shutil.copy2(source / name, target / name)
    hashes = {name: sha(path) for name, path in inputs.items()}
    for bits, entry in ((31, "tso31-any-entry.obj"),
                        (64, "tso64-any-entry.obj")):
        check = json.loads((source / f"tso{bits}-native-check.json").read_text())
        if (check["entry"]["sha256"] != hashes[entry] or
                check["service"]["sha256"] != hashes["mvssupa.obj"]):
            raise ValueError(f"stale TSO{bits} native deck check")
    return hashes


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--work", type=Path, required=True)
    for key in SYSROOTS:
        parser.add_argument(f"--{key}", type=Path, required=True)
    parser.add_argument("--native", type=Path, required=True)
    parser.add_argument("--classic-as", type=Path, required=True)
    parser.add_argument("--classic-ld", type=Path, required=True)
    parser.add_argument("--zpdos-root", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--cc", default="cc")
    args = parser.parse_args()
    verify_source()
    out = args.out.resolve()
    if out.exists() or Path(str(out) + ".tar.gz").exists():
        parser.error("refusing existing SDK destination")
    work = args.work.resolve(strict=True)
    tool_receipt = json.loads((work / "tools.json").read_text())
    if tool_receipt["format"] != "mainframe-compiler-sdk-tools-v1":
        parser.error("unrecognized tool producer receipt")
    sources = {key: source_directory(getattr(args, key), name)
               for key, name in SYSROOTS.items()}
    classic_tools = {"mf-classic-as": args.classic_as.resolve(strict=True),
                     "mf-classic-ld": args.classic_ld.resolve(strict=True)}
    for path in classic_tools.values():
        if not path.is_file():
            raise ValueError(f"missing Classic tool: {path}")
    zpdos = args.zpdos_root.resolve(strict=True)
    for component in ZPDOS_COMPONENTS:
        for name in ("LICENSE", "UPSTREAM.md"):
            if not (zpdos / component / name).is_file():
                raise ValueError(f"missing z/PDOS notice: {component}/{name}")
    out.mkdir(parents=True)
    sysroots = out / "sysroots"
    sysroots.mkdir()
    for key, name in SYSROOTS.items():
        shutil.copytree(sources[key], sysroots / name,
                        ignore=shutil.ignore_patterns("._*", ".DS_Store"))
    shutil.copytree(ROOT / "sdk/src/profiles", out / "profiles",
                    ignore=shutil.ignore_patterns("._*", ".DS_Store"))
    kernel = sysroots / "vmkernel"
    kernel.mkdir()
    shutil.copy2(ROOT / "sdk/src/profiles/vmkernel.json", kernel / "profile.json")
    (kernel / "include/cms").mkdir(parents=True)
    shutil.copy2(ROOT / "sdk/src/vmkernel/cms_storage.h", kernel / "include/cms/cms_storage.h")
    (kernel / "components/cms").mkdir(parents=True)
    for name in ("storage.c", "FSTPROD.ASSEMBLE", "FSTGLUE.ASSEMBLE"):
        shutil.copy2(ROOT / "sdk/src/vmkernel" / name, kernel / "components/cms" / name)
    copy_files(work / "install/gcc", out / "host/gcc", ("bin", "libexec", "lib", "share"))
    copy_files(work / "install/binutils", out / "host/binutils",
               ("bin", "lib", "share", "s390-linux-gnu"))
    (out / "host/bin").mkdir()
    helpers = build_host_helpers(out, args.cc)
    (out / "host/classic/bin").mkdir(parents=True)
    for name, path in classic_tools.items():
        shutil.copy2(path, out / "host/classic/bin" / name)
    native_hashes = copy_native(args.native, out)
    (out / "tools").mkdir()
    for name in ("compiler_sdk_consume.py", "tso64_deck_check.py",
                 "tso31_deck_check.py", "pdos64_xmit_unload.py"):
        shutil.copy2(ROOT / "sdk/scripts" / name, out / "tools" / name)
    shutil.copytree(ROOT / "libc/src/adapters/tso64", out / "adapters/tso64",
                    ignore=shutil.ignore_patterns("._*", ".DS_Store"))
    shutil.copytree(ROOT / "libc/src/adapters/cms", out / "adapters/cms",
                    ignore=shutil.ignore_patterns("._*", ".DS_Store"))
    (out / "adapters/tso").mkdir()
    for name in ("entry24.asm", "entry31-any.asm", "entry31.asm", "io.c",
                 "native-args.h", "native-path.c", "sdk-path.h", "services.h",
                 "signals.c"):
        shutil.copy2(ROOT / "libc/src/adapters/tso" / name, out / "adapters/tso" / name)
    shutil.copy2(ROOT / "libc/src/adapters/tso/sdk-application.c",
                 out / "adapters/tso/application.c")
    shutil.copy2(ROOT / "libc/src/adapters/mainframe_text.h",
                 out / "adapters/mainframe_text.h")
    (out / "contracts/tso").mkdir(parents=True)
    shutil.copy2(ROOT / "tests/tso/sdk-member-smoke.rexx",
                 out / "contracts/tso/sdk-member-smoke.rexx")
    shutil.copy2(ROOT / "tests/tso/sdk-file-smoke.c",
                 out / "contracts/tso/sdk-file-smoke.c")
    shutil.copy2(ROOT / "tests/tso/sdk-file-setup.rexx",
                 out / "contracts/tso/sdk-file-setup.rexx")
    for source, installed in (("tso-image.ld", "image.ld"),
                              ("tso64-image.ld", "image64.ld"),
                              ("tso64-high-image.ld", "image64-high.ld")):
        shutil.copy2(ROOT / "sdk/src/layouts" / source, out / "contracts/tso" / installed)
    shutil.copy2(ROOT / "sdk/scripts/check_high_entry.py",
                 out / "contracts/tso/check_high_entry.py")
    shutil.copytree(ROOT / "tests/profiles", out / "contracts/profiles",
                    ignore=shutil.ignore_patterns("._*", ".DS_Store"))
    shutil.copy2(ROOT / "sdk/src/layouts/cms-image.ld",
                 out / "contracts/profiles/image.ld")
    shutil.copy2(ROOT / "sdk/doc/INSTALLED-README.md", out / "README.md")
    notices = out / "notices"
    notices.mkdir()
    for name in ("LICENSE", "LICENSING.md"):
        shutil.copy2(ROOT / name, notices / name)
    for component in ZPDOS_COMPONENTS:
        target = notices / f"z-pdos-{component}"
        target.mkdir()
        for name in ("LICENSE", "UPSTREAM.md"):
            shutil.copy2(zpdos / component / name, target / name)
    profiles = {}
    widths = {"vm370-4381-v1": 24, "cms20-esa31-v1": 31,
              "tso-zos24-v1": 24, "tso-zos31-v1": 31,
              "tso-zos64-v1": 64, "vmkernel": 24}
    for name in ACTIVE:
        profile_path = out / "profiles" / f"{name}.json"
        data = json.loads(profile_path.read_text())
        profiles[name] = {"profile_sha256": sha(profile_path),
                          "object_identity": data["object_identity"],
                          "sysroot": f"sysroots/{name}",
                          "address_bits": widths[name],
                          "actions": (["compile", "component"] if name == "vmkernel" else
                                      ["compile", "link", "module"] if name.startswith(("vm370", "cms20")) else
                                      ["compile", "link", "native-deck", "xmit"]),
                          "native_execution": "host build only; guest qualification pending"}
    profiles["tso-zos64-v1"]["residence_routes"] = {
        "default": "AMODE64/RMODE ANY entry and RMODE31 image below 2 GiB",
        "high": "application low launcher remains in Mainframe Lab; excluded from SDK 0.1.0"}
    files = {str(path.relative_to(out)): sha(path) for path in sorted(out.rglob("*"))
             if path.is_file() and not path.is_symlink() and not path.name.startswith("._")}
    receipt = {"format": "mainframe-compiler-sdk-local-v1",
               "version": "0.1.0-candidate", "host": tool_receipt["host"],
               "source_only_build_inputs": True,
               "tool_producer": tool_receipt,
               "source_manifest_sha256": sha(ROOT / "SOURCE-MANIFEST.json"),
               "source_sysroots_sha256": {
                   key: {str(path.relative_to(source)): sha(path)
                         for path in sorted(source.rglob("*")) if path.is_file()}
                   for key, source in sources.items()},
               "classic_tools_sha256": {name: sha(path) for name, path in classic_tools.items()},
               "native_source_objects_sha256": native_hashes,
               "host_helpers_sha256": helpers,
               "profiles": profiles, "files_sha256": files,
               "qualification": "source-built host candidate; guest and other-host results are recorded separately",
               "pdos_application_target": "excluded"}
    (out / "SDK-MANIFEST.json").write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n")
    archive = Path(str(out) + ".tar.gz")
    tar_package(out, archive)
    print(f"Source-built SDK candidate {archive} SHA-256 {sha(archive)}")


if __name__ == "__main__":
    main()
