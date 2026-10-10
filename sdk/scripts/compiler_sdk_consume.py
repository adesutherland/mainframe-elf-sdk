#!/usr/bin/env python3
"""Build small profile consumers from an installed SDK, outside its producer."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

from pdos64_xmit_unload import extract as check_xmit_member

PROFILES = ("vm370-4381-v1", "cms20-esa31-v1", "tso-zos24-v1",
            "tso-zos31-v1", "tso-zos64-v1", "vmkernel")


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(*args: str | Path, cwd: Path | None = None,
        env: dict[str, str] | None = None) -> None:
    command = [str(x) for x in args]
    print("+", " ".join(command), flush=True)
    subprocess.run(command, cwd=cwd, check=True,
                   env={**os.environ, **(env or {})})


def verify(sdk: Path) -> dict:
    receipt = json.loads((sdk / "SDK-MANIFEST.json").read_text())
    if receipt["format"] != "mainframe-compiler-sdk-local-v1":
        raise ValueError("unknown SDK format")
    for name, digest in receipt["files_sha256"].items():
        path = sdk / name
        if not path.is_file() or sha(path) != digest:
            raise ValueError(f"SDK file differs: {path}")
    return receipt


def producer_c_decks(work: Path, sysroots: dict[str, Path], out: Path) -> None:
    """Build the three checked TSO C decks before native service packaging."""
    from compiler_sdk import ROOT, verify_source
    from compiler_sdk_package import build_host_helpers

    verify_source()
    if out.exists():
        raise ValueError(f"refusing existing C deck output: {out}")
    out.mkdir(parents=True)
    stage = out / "stage"
    (stage / "host/bin").mkdir(parents=True)
    (stage / "host/gcc").symlink_to(work / "install/gcc", target_is_directory=True)
    (stage / "host/binutils").symlink_to(work / "install/binutils", target_is_directory=True)
    build_host_helpers(stage, "cc")
    (stage / "sysroots").mkdir()
    profiles = {"24": "tso-zos24-v1", "31": "tso-zos31-v1",
                "64": "tso-zos64-v1"}
    for bits, profile in profiles.items():
        source = sysroots[bits].resolve(strict=True)
        if json.loads((source / "profile.json").read_text())["name"] != profile:
            raise ValueError(f"wrong source-built sysroot: {source}")
        (stage / "sysroots" / profile).symlink_to(source, target_is_directory=True)
    (stage / "adapters").mkdir()
    for name in ("tso", "tso64", "cms"):
        source = ROOT / "libc/src/adapters" / name
        shutil.copytree(source, stage / "adapters" / name)
    shutil.copy2(ROOT / "libc/src/adapters/mainframe_text.h",
                 stage / "adapters/mainframe_text.h")
    for name in ("command.h", "command-internal.h", "command-text.c"):
        shutil.copy2(ROOT / "libc/src/adapters" / name, stage / "adapters" / name)
    shutil.copy2(stage / "adapters/tso/sdk-application.c",
                 stage / "adapters/tso/application.c")
    (stage / "contracts/tso").mkdir(parents=True)
    for source, installed in (("tso-image.ld", "image.ld"),
                              ("tso64-image.ld", "image64.ld")):
        shutil.copy2(ROOT / "sdk/src/layouts" / source,
                     stage / "contracts/tso" / installed)
    results = {}
    for profile in profiles.values():
        target = out / profile
        target.mkdir()
        source = target / "consumer.c"
        source.write_text("int main(void) { return 42; }\n")
        results[profile] = link_tso(stage, profile, source, target)
    (out / "producer-decks.json").write_text(
        json.dumps(results, indent=2, sort_keys=True) + "\n")
    print(f"Source-built TSO C decks PASS: {out}")


def compile_one(sdk: Path, profile: str, source: Path, output: Path,
                *, extra: tuple[str, ...] = ()) -> Path:
    cc = sdk / "host/gcc/bin/s390-linux-gnu-gcc"
    assembler = sdk / "host/binutils/bin/s390-linux-gnu-as"
    check = sdk / "host/bin/s370_check"
    include = sdk / "sysroots" / profile / "include"
    common = ["-S", "-O2", "-std=gnu99", "-fno-pic", "-fno-pie",
              "-fno-asynchronous-unwind-tables", "-fno-unwind-tables",
              "-fno-stack-protector", "-fno-merge-constants", "-ffreestanding",
              "-ffunction-sections", "-fdata-sections", "-funsigned-char"]
    if profile in ("vm370-4381-v1", "cms20-esa31-v1", "vmkernel"):
        machine = [f"-ms370-profile={profile}"]
    elif profile == "tso-zos64-v1":
        machine = ["-m64", "-mzarch", "-march=z900", "-msoft-float", "-mno-vx"]
    else:
        machine = ["-mexperimental-s370", "-m31", "-mesa", "-msoft-float", "-mno-mvcle"]
    output.parent.mkdir(parents=True, exist_ok=True)
    assembly = output.with_suffix(".s")
    stamped = output.with_suffix(".profile.s")
    run(cc, f"-B{sdk / 'host/binutils/bin'}/", *common, *machine,
        f"-I{include}", *extra, source, "-o", assembly)
    if profile == "tso-zos64-v1":
        check = sdk / "host/bin/tso64_check"
        run(check, "stamp", assembly, stamped)
        run(assembler, "-L", "-m64", "-march=z900", stamped, "-o", output)
        run(check, "input", output)
    else:
        run(check, "annotate", profile, assembly, stamped)
        run(assembler, "-L", "-m31", "-mesa", stamped, "-o", output)
        run(check, "input", profile, output)
    return output


def command_candidate(sdk: Path, profile: str, adapters: Path, out: Path) -> tuple[Path, dict]:
    """Stage reviewed adapter sources over a verified SDK without changing it.

    The cREXX qualification recipe owns this bounded workflow. Libraries,
    compiler, exporters and native file services remain verified base inputs.
    This is a local candidate, not a release package or guest result.
    """
    stage = out / "candidate-sdk"
    shutil.copytree(sdk, stage)
    sources = [adapters / name for name in
               ("command.h", "command-internal.h", "command-text.c",
                "cms/command.c", "cms/command-call.s", "cms/text-codec.c",
                "tso/command.c", "tso/entry31-any.asm", "tso/services.h",
                "tso/sdk-application.c")]
    identities = {}
    for path in sources:
        identities[str(path)] = sha(path)
        target = stage / "adapters" / path.relative_to(adapters)
        shutil.copyfile(path, target)
    shutil.copyfile(adapters / "command.h", stage / "sysroots" / profile /
                    "include/command.h")
    shutil.copyfile(adapters / "tso/sdk-application.c", stage / "adapters/tso/application.c")
    if profile == "cms20-esa31-v1":
        objects = []
        work = out / "command-runtime"
        extra = (f"-I{stage / 'adapters'}", f"-I{stage / 'adapters/cms'}")
        for name in ("cms/command.c", "command-text.c"):
            objects.append(compile_one(stage, profile, stage / "adapters" / name,
                           work / (Path(name).parent.name + "-" + Path(name).stem + ".o"),
                           extra=extra))
        stamped = work / "command-call.profile.s"
        object_file = work / "command-call.o"
        run(stage / "host/bin/s370_check", "annotate", profile,
            stage / "adapters/cms/command-call.s", stamped)
        run(stage / "host/binutils/bin/s390-linux-gnu-as", "-L", "-m31", "-mesa",
            stamped, "-o", object_file)
        run(stage / "host/bin/s370_check", "input", profile, object_file)
        objects.append(object_file)
        archive = stage / "sysroots" / profile / "lib/libcms.a"
        run(stage / "host/binutils/bin/s390-linux-gnu-ar", "rcsD", archive, *objects)
    else:
        entry = out / "command-entry.obj"
        run(stage / "host/classic/bin/mf-classic-as", "--profile", "z900",
            stage / "adapters/tso/entry31-any.asm", entry)
        shutil.copyfile(entry, stage / "native/source/tso31-any-entry.obj")
    return stage, {"source_files_sha256": identities,
                   "base_sdk_manifest_sha256": sha(sdk / "SDK-MANIFEST.json"),
                   "qualification": "local adapter overlay on verified SDK; no guest execution"}


def link_cms(sdk: Path, profile: str, object_file: Path, out: Path) -> dict:
    lib = sdk / "sysroots" / profile / "lib"
    start = sdk / "sysroots" / profile / "startup.o"
    libraries = [lib / name for name in ("libcms.a", "libnewlib.a", "libextra.a",
                "libm-extra.a", "libgcc-wide.a", "libgcc-soft.a",
                "libnewlib.a", "libcms.a", "libgcc-wide.a", "libgcc-soft.a")]
    ld = sdk / "host/binutils/bin/s390-linux-gnu-ld"
    check = sdk / "host/bin/s370_check"
    for path in [start, object_file, *libraries]:
        run(check, "input", profile, path)
    elf = out / "consumer.elf"
    run(ld, "-m", "elf_s390", "-static", "--gc-sections", "--emit-relocs",
        "--orphan-handling=error", "-T", sdk / "contracts/profiles/image.ld",
        f"-Map={out / 'consumer.map'}", "-o", elf, start, object_file, *libraries)
    run(check, "image", profile, elf)
    module = out / "consumer.module"
    if profile == "vm370-4381-v1":
        writer = sdk / "host/bin/elf_to_cms_module"
        origin = "020000"
    else:
        writer = sdk / "host/bin/elf_to_cms_module31"
        origin = "2200000"
    run(writer, elf, module, origin)
    run(writer, "--verify", elf, module, origin)
    return {"object_sha256": sha(object_file), "elf_sha256": sha(elf),
            "module_sha256": sha(module)}


def link_tso(sdk: Path, profile: str, source: Path, out: Path,
             member_prefix: str = "SDKTS") -> dict:
    bits = profile.removeprefix("tso-zos").removesuffix("-v1")
    include = sdk / "sysroots" / profile / "include"
    if bits == "64":
        profile_header = sdk / "adapters/tso64/profile.h"
        extra = (f"-I{sdk / 'adapters/tso64'}", f"-I{sdk / 'adapters/tso'}",
                 f"-I{sdk / 'adapters/cms'}",
                 f"-include{profile_header}", "-D__MAINFRAME_LAB_TSO64__=1",
                 "-DLAB_TSO_GENERIC_PROGRAM=1")
        checker = sdk / "host/bin/tso64_check"
        linker = "elf64_s390"
        layout = sdk / "contracts/tso/image64.ld"
        writer = sdk / "host/bin/elf_to_mvs64"
    else:
        extra = (f"-I{sdk / 'adapters'}", f"-I{sdk / 'adapters/tso'}", f"-I{sdk / 'adapters/cms'}",
                 "-DLAB_TSO_GENERIC_PROGRAM=1")
        if bits == "24":
            extra += ("-DLAB_TSO_HEAP_SIZE=4194304", "-DLAB_TSO24=1")
        checker = sdk / "host/bin/s370_check"
        linker = "elf_s390"
        layout = sdk / "contracts/tso/image.ld"
        writer = sdk / ("host/bin/elf_to_mvs24" if bits == "24" else "host/bin/elf_to_mvs")
    native_sources = [sdk / "adapters/tso/application.c", source,
                      sdk / "adapters/tso/io.c", sdk / "adapters/cms/text-codec.c",
                      sdk / "adapters/cms/arithmetic.c", sdk / "adapters/tso/signals.c",
                      sdk / "adapters/tso/native-path.c"]
    if bits == "64":
        native_sources.append(sdk / "adapters/tso64/filebridge.c")
    objects = [compile_one(sdk, profile, path, out / f"part{index}.o", extra=extra)
               for index, path in enumerate(native_sources)]
    if bits == "31" and (sdk / "adapters/tso/command.c").is_file():
        for name in ("tso/command.c", "command-text.c"):
            objects.append(compile_one(sdk, profile, sdk / "adapters" / name,
                           out / (Path(name).stem + ".o"),
                           extra=(*extra, "-D__MAINFRAME_LAB_TSO31__=1")))
    lib = sdk / "sysroots" / profile / "lib"
    libraries = [lib / name for name in ("libnewlib.a", "libextra.a", "libm-extra.a",
                 "libgcc-wide.a", "libgcc-soft.a")]
    for library in libraries:
        if bits == "64":
            run(checker, "input", library)
        else:
            run(checker, "input", profile, library)
    elf = out / "consumer.elf"
    ld = sdk / "host/binutils/bin/s390-linux-gnu-ld"
    run(ld, "-m", linker, "-static", "--gc-sections", "--emit-relocs",
        "--orphan-handling=error", "--wrap=fopen", "-T", layout,
        f"-Map={out / 'consumer.map'}", "-o", elf, "--start-group", *objects,
        *libraries, "--end-group")
    if bits == "64":
        run(checker, "image", elf)
    else:
        run(checker, "image", profile, elf)
    deck = out / "consumer.obj"
    run(writer, elf, deck)
    result = {"source_sha256": sha(source), "object_sha256": sha(objects[1]), "elf_sha256": sha(elf),
            "runtime_libraries_sha256": {path.name: sha(path) for path in libraries},
            "native_deck_sha256": sha(deck)}
    native = sdk / "native/source"
    if native.is_dir():
        classic = sdk / "host/classic/bin/mf-classic-ld"
        service = native / "mvssupa.obj"
        if bits == "24":
            routes = (("default", "tso24-entry.obj", "LABTS24", "24", "24", member_prefix + "24"),)
        elif bits == "31":
            routes = (("default", "tso31-any-entry.obj", "LABTSO", "31", "31", member_prefix + "31"),)
        else:
            routes = (("default", "tso64-any-entry.obj", "LABTS64", "64", "31", member_prefix + "64A"),
                      ("low-entry", "tso64-low-entry.obj", "LABTS64", "31", "31", member_prefix + "64L"))
        native_results = {}
        for route, entry, symbol, amode, rmode, member in routes:
            xmit = out / f"{member}.XMI"
            map_file = out / f"consumer-{route}.map"
            run(classic, "--oformat", "xmit", "--amode", amode,
                "--rmode", rmode, "-e", symbol, "-Map", map_file,
                "-o", xmit.name, native / entry, service, deck,
                cwd=out, env={"SOURCE_DATE_EPOCH": "0"})
            _, member_check = check_xmit_member(
                xmit.read_bytes(), member, int(amode),
                "24" if rmode == "24" else "ANY")
            native_results[route] = {"xmit_sha256": sha(xmit),
                                     "map_sha256": sha(map_file),
                                     "member": member_check["member"],
                                     "amode": member_check["native_directory_amode"],
                                     "rmode": member_check["native_directory_rmode"]}
        result["source_native_links"] = native_results
    if bits == "64" and (sdk / "native/high-launchers").is_dir():
        high_entry = out / "high-entry.o"
        run(sdk / "host/binutils/bin/s390-linux-gnu-as", "-m64",
            sdk / "adapters/tso64/high-entry.s", "-o", high_entry)
        high_elf = out / "consumer-high.elf"
        run(ld, "-m", "elf64_s390", "-static", "--gc-sections",
            "--emit-relocs", "--orphan-handling=error", "--wrap=fopen",
            "-T", sdk / "contracts/tso/image64-high.ld",
            f"-Map={out / 'consumer-high.map'}", "-o", high_elf,
            "--start-group", high_entry, *objects, *libraries, "--end-group")
        run(checker, "image", high_elf)
        run(sys.executable, sdk / "contracts/tso/check_high_entry.py", high_elf)
        high_deck = out / "consumer-high.obj"
        run(writer, "--high", high_elf, high_deck)
        run(writer, "--check-high", high_elf)
        result["rmode64_candidate"] = {
            "elf_sha256": sha(high_elf), "high_deck_sha256": sha(high_deck),
            "accepted_low_launchers_sha256": {
                name: sha(sdk / "native/high-launchers" / name)
                for name in ("LAU65O.obj", "LAVM65O.obj", "LAC65O.obj")},
            "qualification": "host converter and high-entry checks with pinned low launcher objects; classic load-module binding and modern z/OS execution remain separate"}
    elif bits == "64":
        result["rmode64_candidate"] = {"qualification": "not packaged; low launcher inputs remain with the application consumer"}
    return result


def main() -> None:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--sdk", type=Path)
    p.add_argument("--out", type=Path, required=True)
    p.add_argument("--profile", choices=(*PROFILES, "all"), default="all")
    p.add_argument("--tso-source", type=Path,
                   help="compile a supplied C consumer for one TSO profile")
    p.add_argument("--cms-source", type=Path,
                   help="compile a supplied C consumer for one CMS profile")
    p.add_argument("--command-adapters", type=Path,
                   help="stage maintained command adapters over a verified CMS31/TSO31 SDK")
    p.add_argument("--member-prefix", default="SDKTS",
                   help="one to five uppercase letters/digits for native member names")
    p.add_argument("--producer-c-decks", action="store_true",
                   help="build pre-package TSO C decks from source-built tools and sysroots")
    p.add_argument("--work", type=Path)
    for bits in ("24", "31", "64"):
        p.add_argument(f"--tso{bits}", type=Path)
    a = p.parse_args()
    if a.producer_c_decks:
        if not Path(__file__).with_name("compiler_sdk.py").is_file():
            p.error("--producer-c-decks requires the checked SDK source tree")
        if not a.work or any(getattr(a, f"tso{bits}") is None for bits in ("24", "31", "64")):
            p.error("--producer-c-decks needs --work and --tso24/31/64")
        producer_c_decks(a.work.resolve(strict=True),
                         {bits: getattr(a, f"tso{bits}") for bits in ("24", "31", "64")},
                         a.out.resolve())
        return
    if a.sdk is None:
        p.error("--sdk is required for an installed consumer")
    if not re.fullmatch(r"[A-Z][A-Z0-9]{0,4}", a.member_prefix):
        p.error("member prefix must be one to five uppercase letters/digits")
    if a.tso_source and not a.profile.startswith("tso-"):
        p.error("--tso-source requires one TSO profile")
    if a.cms_source and a.profile not in ("vm370-4381-v1", "cms20-esa31-v1"):
        p.error("--cms-source requires one CMS profile")
    if a.command_adapters and a.profile not in ("cms20-esa31-v1", "tso-zos31-v1"):
        p.error("--command-adapters requires CMS31 or TSO31")
    if (a.profile == "tso-zos24-v1" and a.tso_source and
            a.tso_source.name == "sdk-file-smoke.c"):
        p.error("the SDK file smoke requires TSO31 or TSO64; "
                "TSO24 dataset I/O is not in the 0.1.0 file profile")
    sdk = a.sdk.resolve(strict=True)
    out = a.out.resolve()
    if out.exists():
        p.error("refusing existing consumer output")
    receipt = verify(sdk)
    base_manifest_sha256 = sha(sdk / "SDK-MANIFEST.json")
    out.mkdir(parents=True)
    overlay = None
    if a.command_adapters:
        sdk, overlay = command_candidate(sdk, a.profile,
                                         a.command_adapters.resolve(strict=True), out)
    selected = PROFILES if a.profile == "all" else (a.profile,)
    results = {}
    for profile in selected:
        target = out / profile
        target.mkdir()
        source = target / "consumer.c"
        if profile.startswith("tso-"):
            if a.tso_source:
                shutil.copyfile(a.tso_source.resolve(strict=True), source)
            else:
                # A retained absolute pointer relocation exercises the native
                # exporter; a constant-return function would be too small a test.
                source.write_text("static volatile int answer=42; "
                                  "static volatile int * volatile pointer=&answer; "
                                  "int main(int argc, char **argv) { (void)argc; "
                                  "(void)argv; return *pointer; }\n")
        elif a.cms_source:
            shutil.copyfile(a.cms_source.resolve(strict=True), source)
        elif profile != "vmkernel":
            source.write_text("int main(int argc, char **argv) { (void)argv; return argc+41; }\n")
        if profile == "vmkernel":
            component = sdk / "sysroots/vmkernel/components/cms/storage.c"
            obj = compile_one(sdk, profile, component, target / "FSTCALC.o",
                              extra=(f"-I{sdk / 'sysroots/vmkernel/include/cms'}",))
            assembly = target / "FSTCODE.ASSEMBLE"
            run(sdk / "host/bin/elf_to_vmce_asm", obj, "FSTCODE", assembly)
            results[profile] = {"component_sha256": sha(obj),
                                "native_assembler_source_sha256": sha(assembly)}
        elif profile.startswith("tso-"):
            results[profile] = link_tso(sdk, profile, source, target,
                                        a.member_prefix)
        else:
            obj = compile_one(sdk, profile, source, target / "consumer.o",
                              extra=(f"-I{sdk / 'adapters'}",))
            results[profile] = link_cms(sdk, profile, obj, target)
    (out / "result.json").write_text(json.dumps({"format": "mainframe-compiler-sdk-consumer-v1",
        "sdk_manifest_sha256": base_manifest_sha256, "command_candidate": overlay, "profiles": results,
        "qualification": "host compile/link/package only; no new guest execution"},
        indent=2, sort_keys=True) + "\n")
    print(f"SDK consumer PASS: {out}")


if __name__ == "__main__":
    main()
