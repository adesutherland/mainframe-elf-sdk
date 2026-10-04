# Building the SDK from source

Use a [release package](https://github.com/adesutherland/mainframe-elf-sdk/releases/tag/v0.1.0)
if you want to compile an application. This guide is for rebuilding the
compiler, libraries, native objects and installed SDK itself.

The complete release route uses this repository plus three maintained
components from [z/PDOS](https://github.com/adesutherland/z-pdos): Classic
Assembler, Classic Linker and PDPCLIB. It does not need the private Mainframe
Lab checkout, IBM macro libraries, a guest system or prebuilt target objects.
The [licensing guide](../LICENSING.md) explains the source and host bootstrap
requirements.

## Inputs and host prerequisites

For the **0.1.0** input set, use SDK tag `v0.1.0` and z/PDOS revision
`930dbbc13119ab69d6b4ddc4f42e4d6edcf6fd59`. Later `develop` commits describe
subsequent work, not the exact source of the published archives. The
[producer workflow](../.github/workflows/compiler-sdk-producer.yml) records
the complete hosted Linux sequence and the pinned host cREXX download.

| Host | Build dependencies |
| --- | --- |
| Apple Silicon macOS | Homebrew GCC (`gcc-16`/`g++-16`), GMP, MPFR, MPC (`libmpc`) and ISL; working `cc` for SDK helpers |
| Linux | Host GCC/G++, GMP, MPFR, MPC and ISL development packages; the release workflow uses Ubuntu 24.04 x64 |
| Both | Python 3.12+, make, patch, CMake, common archive/shell utilities and a working host cREXX runtime |

The hosted build uses the pinned cREXX **v1.0.0-beta.3** runtime. Its
`crexx` command executes the runtime and native-object recipes; no target
cREXX program bootstraps the SDK. If downloading that runtime, use the asset
and checksum recorded in the pinned z/PDOS `.github/crexx-bootstrap.txt`.
Windows remains a later host qualification target.

The producer locks official GCC 16.2.0, GNU binutils 2.47 and newlib
4.6.0.20260123 archive URLs and SHA-256 values in
[`scripts/compiler_sdk.py`](scripts/compiler_sdk.py). Downloads with a
mismatched hash are rejected. A cache can supply the same checked archives
without network access.

## 1. Prepare an independent source tree and build GNU tools

Run from the SDK checkout. Replace these example absolute paths with fresh
locations; extraction and preparation refuse existing destinations.

```sh
python3 sdk/scripts/compiler_sdk.py extract --out /tmp/elf-sdk-source
cd /tmp/elf-sdk-source
python3 sdk/scripts/compiler_sdk.py prepare \
  --cache /tmp/elf-sdk-archives --work /tmp/elf-sdk-work --download
python3 sdk/scripts/compiler_sdk.py build-tools --work /tmp/elf-sdk-work
```

Extraction copies the allowlist in `sdk/source-files.txt` and writes
`SOURCE-MANIFEST.json`. Preparation checks the archives and maintained-file
hashes, then overlays `compiler/src/gcc16/` and `libc/src/newlib/` onto the
upstream source. `build-tools` builds C-only GCC and GNU binutils into the
work directory. It does not build the profile runtimes or a complete SDK.

Keep subsequent SDK commands in the extracted source root. Generated output
belongs outside that source tree or under its ignored `build/` directory.
The strict source inventory rejects unexpected files; frozen component
`archive/` directories are excluded from extraction and normal builds.

## 2. Build the five application runtimes

The same sequence applies to each application profile. The loop below records
the existing recipe order; it is also present in the hosted workflow.
All output directories must be new.

```sh
SDK_WORK=/tmp/elf-sdk-work
SDK_RUNTIME=/tmp/elf-sdk-runtime
for profile in vm370-4381-v1 cms20-esa31-v1 tso-zos24-v1 tso-zos31-v1 tso-zos64-v1; do
  output="$SDK_RUNTIME/$profile"
  mkdir -p "$output"
  if [ "$profile" = tso-zos64-v1 ]; then
    crexx -nokeep libc/scripts/build-newlib64.crexx --args \
      "$SDK_WORK" "$output/core"
  else
    crexx -nokeep libc/scripts/build-newlib.crexx --args \
      "$SDK_WORK" "$profile" "$output/core"
  fi
  crexx -nokeep libc/scripts/build-newlib-extras.crexx --args \
    "$SDK_WORK" "$profile" "$output/core" "$output/extras"
  for kind in wide soft; do
    crexx -nokeep libc/scripts/build-libgcc.crexx --args \
      "$SDK_WORK" "$profile" "$kind" "$output/$kind"
  done
  if [ "$profile" = vm370-4381-v1 ] || [ "$profile" = cms20-esa31-v1 ]; then
    crexx -nokeep libc/scripts/build-cms-adapters.crexx --args \
      "$SDK_WORK" "$profile" "$output/core" "$output/adapters"
    crexx -nokeep libc/scripts/package-cms-sysroot.crexx --args \
      "$SDK_WORK" "$profile" "$output/core" "$output/extras" \
      "$output/wide" "$output/soft" "$output/adapters" "$output/sysroot"
  else
    crexx -nokeep libc/scripts/package-tso-sysroot.crexx --args \
      "$SDK_WORK" "$profile" "$output/core" "$output/extras" \
      "$output/wide" "$output/soft" "$output/sysroot"
  fi
done
```

Each sysroot contains selected newlib, extra/math, wide-integer and
software-float libraries, headers, profile metadata and notices. CMS adds its
startup and adapter library. The TSO native decks are built next. `vmkernel`
has no application libc; the final packager installs its component source.

## 3. Build Classic tools and the native TSO objects

Use a separate z/PDOS checkout at the pinned revision. Set `ZPDOS_ROOT` to its
absolute path. Run the existing build recipe there, then return to the SDK
source root:

```sh
ZPDOS_ROOT=/absolute/path/to/z-pdos
(cd "$ZPDOS_ROOT" && crexx -nokeep scripts/build.crexx --args build)
```

Build the TSO C decks using the new sysroots, then the native entries and
selected PDPCLIB service, and finally four complete XMIT links:

```sh
python3 sdk/scripts/compiler_sdk_consume.py --producer-c-decks \
  --work "$SDK_WORK" \
  --tso24 "$SDK_RUNTIME/tso-zos24-v1/sysroot" \
  --tso31 "$SDK_RUNTIME/tso-zos31-v1/sysroot" \
  --tso64 "$SDK_RUNTIME/tso-zos64-v1/sysroot" \
  --out /tmp/elf-sdk-c-decks
crexx -nokeep sdk/scripts/build-tso-native.crexx --args \
  "$ZPDOS_ROOT" build/pdpclib-sdk-release /tmp/elf-sdk-c-decks \
  /tmp/elf-sdk-native
```

The `build/pdpclib-sdk-release` argument is relative to the z/PDOS checkout;
the final output directory is an SDK build product. The recipe uses
`build/tools/assembler/mf-classic-as` and `build/tools/linker/mf-classic-ld`
from that checkout. It selects PDPCLIB's `tso31-sdk-files` service and the
SDK's TSO24, TSO31 and two TSO64 entry sources. Native object and transport
checks are part of the recipe; they do not execute the resulting programs.

## 4. Package and use the installed SDK

```sh
python3 sdk/scripts/compiler_sdk_source_package.py \
  --work "$SDK_WORK" \
  --cms24 "$SDK_RUNTIME/vm370-4381-v1/sysroot" \
  --cms31 "$SDK_RUNTIME/cms20-esa31-v1/sysroot" \
  --tso24 "$SDK_RUNTIME/tso-zos24-v1/sysroot" \
  --tso31 "$SDK_RUNTIME/tso-zos31-v1/sysroot" \
  --tso64 "$SDK_RUNTIME/tso-zos64-v1/sysroot" \
  --native /tmp/elf-sdk-native \
  --classic-as "$ZPDOS_ROOT/build/tools/assembler/mf-classic-as" \
  --classic-ld "$ZPDOS_ROOT/build/tools/linker/mf-classic-ld" \
  --zpdos-root "$ZPDOS_ROOT" \
  --out /tmp/elf-sdk-package
mkdir /tmp/elf-sdk-unpacked
tar -xzf /tmp/elf-sdk-package.tar.gz -C /tmp/elf-sdk-unpacked
python3 /tmp/elf-sdk-unpacked/elf-sdk-package/tools/compiler_sdk_consume.py \
  --sdk /tmp/elf-sdk-unpacked/elf-sdk-package \
  --out /tmp/elf-sdk-installed-examples
```

The packager installs the five sysroots, the freestanding component, GNU and
Classic tools, SDK helpers, source-built native objects and component notices.
It records hashes in `SDK-MANIFEST.json`. The independent consumer uses only
the installed package to compile and export all six profiles, including
native TSO links. Guest execution and publication remain separate actions.
The [installed usage guide](doc/INSTALLED-README.md) describes its outputs.

This sequence rebuilds the SDK from the named inputs; it is not a promise of
byte-for-byte identical host archives across different host installations.
Use the published `SHA256SUMS` to identify the actual release downloads.

## Maintaining source and documentation

Current host checks, exporters, profiles and layouts live in `src/`;
producer, packager and consumer interfaces live in `scripts/`. Edit the
maintained compiler and runtime source in their own components. Classic tools
and PDPCLIB fixes belong in z/PDOS.

The source manifest covers documentation as well as code. After an authorised
edit to listed files, refresh it from the checkout:

```sh
python3 sdk/scripts/compiler_sdk.py refresh-manifest
```

This updates inventory hashes; it is not a compiler build or a guest check.
New files also need an entry in the source allowlist. The older
`compiler_sdk_package.py` consumes historical runtime archives and native
decks for local bootstrap comparison; use `compiler_sdk_source_package.py`
for the released source-built route.

[Architecture](../docs/compiler/SDK.md) · [Source provenance](UPSTREAM.md)
· [SDK backlog](doc/BACKLOG.md) · [Release scope](../docs/RELEASE-PLAN.md)
