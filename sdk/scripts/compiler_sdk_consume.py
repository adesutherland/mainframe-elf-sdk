#!/usr/bin/env python3
"""Build small profile consumers from an installed SDK, outside its producer."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

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


def link_tso(sdk: Path, profile: str, source: Path, out: Path) -> dict:
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
        extra = (f"-I{sdk / 'adapters/tso'}", f"-I{sdk / 'adapters/cms'}",
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
    result = {"object_sha256": sha(objects[1]), "elf_sha256": sha(elf),
            "runtime_libraries_sha256": {path.name: sha(path) for path in libraries},
            "native_deck_sha256": sha(deck)}
    native = sdk / "native/source"
    if native.is_dir():
        classic = sdk / "host/classic/bin/mf-classic-ld"
        service = native / "mvssupa.obj"
        if bits == "24":
            routes = (("default", "tso24-entry.obj", "LABTS24", "24", "24"),)
        elif bits == "31":
            routes = (("default", "tso31-any-entry.obj", "LABTSO", "31", "31"),)
        else:
            routes = (("default", "tso64-any-entry.obj", "LABTS64", "64", "31"),
                      ("low-entry", "tso64-low-entry.obj", "LABTS64", "31", "31"))
        native_results = {}
        for route, entry, symbol, amode, rmode in routes:
            xmit = out / f"consumer-{route}.XMI"
            map_file = out / f"consumer-{route}.map"
            run(classic, "--oformat", "xmit", "--amode", amode,
                "--rmode", rmode, "-e", symbol, "-Map", map_file,
                "-o", xmit, native / entry, service, deck,
                env={"SOURCE_DATE_EPOCH": "0"})
            native_results[route] = {"xmit_sha256": sha(xmit),
                                     "map_sha256": sha(map_file)}
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
    p.add_argument("--sdk", type=Path, required=True)
    p.add_argument("--out", type=Path, required=True)
    p.add_argument("--profile", choices=(*PROFILES, "all"), default="all")
    a = p.parse_args()
    sdk = a.sdk.resolve(strict=True)
    out = a.out.resolve()
    if out.exists():
        p.error("refusing existing consumer output")
    receipt = verify(sdk)
    out.mkdir(parents=True)
    selected = PROFILES if a.profile == "all" else (a.profile,)
    results = {}
    for profile in selected:
        target = out / profile
        target.mkdir()
        source = target / "consumer.c"
        if profile.startswith("tso-"):
            # A retained absolute pointer relocation exercises the native
            # exporter; a constant-return function would be too small a test.
            source.write_text("static volatile int answer=42; "
                              "static volatile int * volatile pointer=&answer; "
                              "int main(int argc, char **argv) { (void)argc; "
                              "(void)argv; return *pointer; }\n")
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
            results[profile] = link_tso(sdk, profile, source, target)
        else:
            obj = compile_one(sdk, profile, source, target / "consumer.o")
            results[profile] = link_cms(sdk, profile, obj, target)
    (out / "result.json").write_text(json.dumps({"format": "mainframe-compiler-sdk-consumer-v1",
        "sdk_manifest_sha256": sha(sdk / "SDK-MANIFEST.json"), "profiles": results,
        "qualification": "host compile/link/package only; no new guest execution"},
        indent=2, sort_keys=True) + "\n")
    print(f"SDK consumer PASS: {out}")


if __name__ == "__main__":
    main()
