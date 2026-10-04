# Using Mainframe ELF SDK 0.1.0

The SDK runs on your host computer and builds programs for a selected CMS or
TSO environment. It includes the compiler, target libraries and packaging
tools. It does not connect to a guest or install programs there automatically.

## Install

Download the **macOS arm64** or **Linux x64** archive and `SHA256SUMS` from
[the 0.1.0 release](https://github.com/adesutherland/mainframe-elf-sdk/releases/tag/v0.1.0).
Compare the archive's SHA-256 with its entry in that file. On macOS use
`shasum -a 256 ARCHIVE`; on Linux use `sha256sum ARCHIVE`, replacing `ARCHIVE`
with the downloaded filename. Extract it into a new directory with `tar -xzf`.

Use Python 3.12 or later and a host compatible with the chosen archive.
The recipes below need no separate cREXX installation: the packaged tools,
objects and libraries are already built. Source-producing recipes have
additional prerequisites described in the
[source build guide](https://github.com/adesutherland/mainframe-elf-sdk/blob/main/sdk/README.md).

Set `SDK` to the absolute path of the unpacked directory containing
`SDK-MANIFEST.json`:

```sh
SDK=/absolute/path/to/unpacked-sdk
```

## Build the included example

Choose one profile and a new output directory:

```sh
python3 "$SDK/tools/compiler_sdk_consume.py" \
  --sdk "$SDK" --profile tso-zos31-v1 \
  --out /absolute/path/to/new-example
```

The consumer verifies installed file hashes, prints the compiler/linker
commands it uses, and writes outputs under a subdirectory named after the
profile. It refuses an existing output directory. The example is intended
to return **42**, so that value is success for the supplied guest harness.
Omit `--profile` to build the included examples for all six profiles.

| Profile | Principal output below its output subdirectory |
| --- | --- |
| `vm370-4381-v1` | `consumer.module`: fixed-origin CMS24 MODULE |
| `cms20-esa31-v1` | `consumer.module`: relocatable CMS31 MODULE |
| `tso-zos24-v1` | `SDKTS24.XMI`: AMODE24/RMODE24 member |
| `tso-zos31-v1` | `SDKTS31.XMI`: AMODE31/RMODE ANY member |
| `tso-zos64-v1` | `SDKTS64A.XMI` and `SDKTS64L.XMI`: AMODE64 and AMODE31 entries into 64-bit C, both with the image below 2 GiB |
| `vmkernel` | `FSTCODE.ASSEMBLE`: exported CMS storage-component source |

The application builds also retain intermediate ELF files and, for TSO,
`consumer.obj` native decks. `result.json` in the output root records hashes
and results. These are host build checks, not guest execution.

## Build your own TSO C file

For a first program, save this as `answer.c` outside the installed SDK:

```c
int main(void)
{
    return 42;
}
```

The consumer accepts one C source file for one TSO profile:

```sh
python3 "$SDK/tools/compiler_sdk_consume.py" \
  --sdk "$SDK" --profile tso-zos31-v1 \
  --tso-source /absolute/path/to/answer.c --member-prefix DEMO \
  --out /absolute/path/to/new-answer-build
```

This produces `tso-zos31-v1/DEMO31.XMI`. A prefix must start with an uppercase
letter and contain at most five uppercase letters/digits in total. The
consumer appends `24`, `31`, `64A` or `64L` to form the native member name.
Use `--profile tso-zos64-v1` for both 64-bit entry variants. This helper is a
small example builder, not a general application build system; custom CMS
source and multi-file projects require adapting the documented command
sequence in `tools/compiler_sdk_consume.py`.

Transfer an XMIT in binary form to a suitably allocated dataset, then use
TSO `RECEIVE` to restore its member into a load library. Transfer and dataset
allocation depend on the target installation. The included
`contracts/tso/sdk-member-smoke.rexx` checks a restored example's return code;
its arguments are the load-library dataset and member name. CMS MODULEs
likewise need a transfer procedure that preserves their native file framing;
a text-mode upload is not suitable.

## File-service example and limits

`contracts/tso/sdk-file-smoke.c` demonstrates the qualified sequential
write/read, PDS read and missing-DD paths. Build it through `--tso-source`
with TSO31 or TSO64 and a distinct member prefix. On the guest, the companion
`contracts/tso/sdk-file-setup.rexx` creates `PREFIX.TXT` and allocates it to
`SDKTXT`. It also allocates the **already existing** `PREFIX.SRC(SDKCHK)`
member to `SDKPDS`; prepare that member with text beginning `/*` before
running the setup. Use your own dataset prefix, leave `SDKMISS` unallocated,
and read the setup script before running it. It refuses an existing `.TXT`
dataset; the C example then writes and reads that new sequential dataset.

Require all three messages — `SDK SEQUENTIAL PASS`, `SDK PDS PASS` and
`SDK MISSING DD PASS` — as well as return code 42. The current fixture also
returns 42 for a PDS content/close failure, so the return code alone cannot
prove this file check passed. This is recorded as
[SDK-006](https://github.com/adesutherland/mainframe-elf-sdk/blob/main/sdk/doc/BACKLOG.md#sdk-006-file-example-success-and-failure-share-a-return-code).

TSO24 dataset I/O is excluded from 0.1.0. The selected service also excludes
FBS positioning and VSAM/IDCAMS support. The guest results are specific to
z/OS 1.5; they are not a general compatibility claim. See the
[release limits](https://github.com/adesutherland/mainframe-elf-sdk/blob/main/docs/RELEASE-PLAN.md#known-limits)
and [architecture guide](https://github.com/adesutherland/mainframe-elf-sdk/blob/main/docs/compiler/SDK.md)
for addressing, memory, text handling and wider runtime limits.

## Installed layout

| Directory or file | Purpose |
| --- | --- |
| `host/gcc/`, `host/binutils/` | Host cross-compiler and GNU tools |
| `host/bin/` | SDK profile checkers and format exporters |
| `host/classic/bin/` | Source-built Classic Assembler and Linker |
| `sysroots/`, `profiles/` | Per-profile libraries, headers and contracts; the retired kernel JSON has no sysroot |
| `adapters/`, `native/source/` | C/assembler adapter source and source-built native TSO objects |
| `contracts/` | Link layouts, examples and guest harnesses |
| `tools/` | Installed consumer and supporting package checks |
| `notices/`, `sysroots/*/notices/` | Component licence and provenance records |
| `SDK-MANIFEST.json` | Installed file hashes and recorded build inputs |

Keep an installed package intact: editing a hashed file causes the consumer's
verification to fail. Put your program and build outputs elsewhere. Manifest
wording such as “host candidate” describes the producer's role; use the
[release record](https://github.com/adesutherland/mainframe-elf-sdk/releases/tag/v0.1.0)
for subsequent guest checks and publication status. Older local bootstrap
packages containing IBM-assembled objects are distinct from this release.
