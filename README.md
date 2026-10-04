# Mainframe ELF SDK

**Build C programs for CMS and TSO on a Mac or Linux computer.** Mainframe
ELF SDK brings a modern GCC compiler, a selected C runtime and mainframe
packaging tools together. It produces CMS MODULEs and TSO load-library
transports without using a proprietary mainframe compiler, assembler or binder
in the build.

CMS is the interactive environment on VM systems; TSO is the interactive
environment on MVS and z/OS. A CMS MODULE is a runnable program. A TSO XMIT
file is a transport that can be restored into a native program library.

I want mainframe development to be accessible from an ordinary computer,
with source you can inspect and rebuild. The first release completes that
build path for a small, explicitly qualified set of environments. It is an
early SDK for experimentation and porting, with more runtime and system
coverage still to do.

[Download 0.1.0](https://github.com/adesutherland/mainframe-elf-sdk/releases/tag/v0.1.0)
· [Architecture and profiles](docs/compiler/SDK.md)
· [Build from source](sdk/README.md)
· [Licensing and provenance](LICENSING.md)
· [Documentation](docs/README.md)

## What is available

Version **0.1.0**, released on 3 October 2026, has **macOS arm64** and
**Linux x64** packages. Windows is planned for 0.1.1. The SDK runs on the
host computer; the resulting program runs in the selected mainframe
environment. No operating system image or emulator is included.

| Target | What the SDK produces | What has been demonstrated for 0.1.0 |
| --- | --- | --- |
| CMS24 | A fixed-origin CMS MODULE for the historical 24-bit environment | A small C program returned 42 on VM/370 Community Edition |
| CMS31 | A relocatable, 31-bit CMS MODULE | A small C program returned 42 on CMS 20 under z/VM 4.4 |
| TSO24 | An AMODE24/RMODE24 load member in an XMIT transport | Entry and return on z/OS 1.5; dataset I/O is excluded |
| TSO31 | An AMODE31/RMODE ANY load member in an XMIT transport | Entry and return, sequential file write/read, PDS member read and missing-DD handling on z/OS 1.5 |
| TSO64 | 64-bit C with a below-2-GiB load member; two native entry variants | The same selected file operations on z/OS 1.5 through both entries |
| `vmkernel` | A freestanding C component exported as native assembler source | Host compilation and export of a CMS storage component; no complete kernel build is supplied |

Both host packages passed the installed build checks for all six profiles.
The [release record](docs/RELEASE-PLAN.md) distinguishes those host checks
from the exact outputs executed on guests. These results do not establish
complete C99 or newlib coverage, compatibility with every CMS or z/OS version,
or modern z/OS execution of code above 2 GiB.

## How it fits together

The SDK uses GCC 16.2.0 with maintained mainframe target changes, GNU
binutils 2.47 and selected newlib 4.6.0.20260123 routines. **ELF** (Executable
and Linkable Format) is the object format used between compilation and
packaging; CMS and TSO receive their own native formats.

```text
C source + a chosen profile
          |
    GCC and GNU binutils
          |
    checked ELF + C runtime
          |
    SDK format exporters
          +---- CMS: MODULE
          +---- TSO: native object deck
                      + entry and file-service objects
                      + Classic Linker -> XMIT -> load-library member
```

We developed the historical GCC target changes, CMS/TSO runtime adapters,
profile checks and native-format exporters. The separate
[z/PDOS project](https://github.com/adesutherland/z-pdos) supplies our original
**Mainframe Classic Assembler**, the **PDLD-derived Classic Linker**, and a
selected native file-service layer from **Paul Edwards's PDPCLIB**. Newlib
provides the C library; PDPCLIB supplies the TSO dataset services beneath it.
The SDK does not include the z/PDOS operating system.

A **profile** ties together the allowed instructions, C calling convention,
address width, operating-system services and output format. For example,
CMS31 and TSO31 both use 32-bit C pointers, but their service calls and native
packages differ. The SDK checks an identity in each ELF object to prevent
those runtimes from being mixed. The
[architecture guide](docs/compiler/SDK.md) explains that check and the
difference between addressing mode and code placement.

## Start with a release package

Download the archive for your host and `SHA256SUMS` from the
[0.1.0 release](https://github.com/adesutherland/mainframe-elf-sdk/releases/tag/v0.1.0).
Verify the archive against that file, then unpack it into a new directory.
Use Python 3.12 or later to run the included example builder, replacing the
paths below with your unpacked SDK and a new output directory:

```sh
python3 /path/to/sdk/tools/compiler_sdk_consume.py \
  --sdk /path/to/sdk --profile tso-zos31-v1 \
  --out /path/to/new-example
```

This checks the installed file hashes, compiles a small C program and creates
`tso-zos31-v1/SDKTS31.XMI` beneath the output directory. It does not connect
to a mainframe. The [usage guide](sdk/doc/INSTALLED-README.md) explains the
other outputs and how to supply your own TSO C file. Building the SDK itself
is a separate [source workflow](sdk/README.md).

## Open build tools, explicit component licences

The 0.1.0 release build uses source-built native objects and independently
authored, limited service definitions in place of IBM assembler macros. It
needs no IBM HLASM/ASMA90, z/OS binder, IBM macro library or prebuilt target
runtime object as a build input. Ordinary host tools and a pinned host cREXX
runtime are still prerequisites.

Original project code is MIT-licensed; GCC, binutils, newlib, PDLD and PDPCLIB
retain their respective terms and credits. Access to a target IBM operating
system is a separate matter: this SDK supplies neither that system nor a
licence to use it. See [licensing and provenance](LICENSING.md) for the exact
scope, source origins and distinction from older private bootstrap packages.

For current limitations and next steps, see the
[release scope](docs/RELEASE-PLAN.md#known-limits) and
[component backlogs](docs/README.md#contributing).
