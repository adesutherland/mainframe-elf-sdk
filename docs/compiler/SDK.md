# Architecture and profiles

Mainframe ELF SDK separates three jobs: generating machine code, providing
the services a C program needs, and putting the result into a format its
operating system can load. A profile connects those jobs for one environment.
This guide describes the 0.1.0 architecture; the
[release record](../RELEASE-PLAN.md) states what has actually been run.

## From C source to a mainframe program

The **host** is the Mac or Linux computer running the build tools. The
**target** is the mainframe environment where the resulting program runs.
The cross-compiler runs on the host and generates instructions for the target.

1. **Compile and assemble.** GCC generates assembly using the selected
   instruction and C calling conventions. SDK helpers annotate that assembly
   with a profile identity. GNU `as` produces an ELF object; the SDK checks
   the object before it enters the link.
2. **Link the C part.** GNU `ld` combines the C objects, selected newlib
   routines, compiler helpers and applicable SDK adapters under a profile
   layout. The link retains relocation information: the places that must be
   adjusted when an image is loaded at a different address.
3. **Export a native format.** The SDK translates the checked ELF image
   into a CMS MODULE or a TSO native object deck. An ELF executable alone is
   not a CMS MODULE or a TSO load member.
4. **Complete the TSO package.** The Classic Linker combines the C deck with
   native entry and PDPCLIB service objects and writes an XMIT transport.
   On TSO, `RECEIVE` restores its load-library member. The CMS path writes
   its MODULE directly and does not use this TSO linking stage.

The TSO entry and service objects are built during SDK production using the
Classic Assembler. They are included in the binary SDK so application users
can link them without rebuilding the SDK. “Source-built” describes their
provenance; it does not mean an installed package contains no object files.

The implementation retains the GNU target name `s390-linux-gnu`. Here that
name identifies the cross-tool executables, not the operating system ABI of
the output. The recipes select non-Linux headers, static libraries, startup,
linker layouts and services. Invoking that GCC with its ordinary Linux
defaults is not equivalent to using an SDK profile.

## Components and responsibilities

| Component | What it does | Maintained home and origin |
| --- | --- | --- |
| Mainframe ELF C | Generates C machine code, including the conservative historical instruction path | [`compiler/src/gcc16/`](../../compiler/src/gcc16/): maintained changes to GNU GCC 16.2.0 |
| GNU assembler and ELF linker | Encode GNU assembly and link the intermediate ELF image | Unmodified GNU binutils 2.47, downloaded and hash-checked by the producer |
| C library and arithmetic helpers | Selected standard C routines, math routines, wide-integer and software floating-point support | [`libc/`](../../libc/README.md): newlib 4.6.0.20260123, selected GCC helpers and SDK integration |
| CMS and TSO adapters | Startup, arguments, storage, text/record handling and calls to native services | [`libc/src/adapters/`](../../libc/src/adapters/): project-maintained C and assembler |
| Profile checkers and exporters | Check object identity and supported layouts; write CMS and TSO formats | [`sdk/src/host/`](../../sdk/src/host/): original project code |
| Mainframe Classic Assembler | Assembles the native TSO entries and selected service definitions | z/PDOS `assembler/`: original MIT implementation |
| Mainframe Classic Linker | Links native object decks and writes the XMIT load-member transport | z/PDOS `linker/`: maintained PDLD-derived code |
| PDPCLIB native service layer | Implements the selected TSO dataset operations below the SDK C adapter | z/PDOS `pdpclib/`: Paul Edwards's library and contributors, with maintained changes |
| Producer and installed consumer | Assemble the SDK package, record its inputs and demonstrate its use | [`sdk/scripts/`](../../sdk/scripts/): Python producer/consumer tools and cREXX recipes |

The [source build guide](../../sdk/README.md) pins the z/PDOS revision used
for 0.1.0. The names `mf-elf-cc`, `mf-elf-as` and `mf-elf-pack` describe the
intended public compiler, assembler and packager interfaces. **0.1.0 does
not install commands with those names.** It installs `s390-linux-gnu-gcc`,
the GNU binutils commands and the individual SDK helpers. The
[installed consumer](../../sdk/scripts/compiler_sdk_consume.py) shows the
actual sequence and prints the commands it runs.

### Where newlib ends and PDPCLIB begins

A TSO file operation follows this path:

```text
C program -> newlib stdio -> SDK I/O adapter
          -> native service bridge -> selected PDPCLIB MVSSUPA routines
          -> TSO/MVS dataset services
```

The SDK uses newlib for its selected C library surface. It does not link a
second complete PDPCLIB C library alongside it. The reused part is the native
assembler support, built with the z/PDOS `tso31-sdk-files` selection.
This provides the release's sequential file and partitioned-dataset paths.
A partitioned dataset (PDS) is a library of named members; a DD name identifies
a dataset allocation available to the program.

For TSO64, the C side has 64-bit pointers while this service layer uses
narrower addresses. The SDK's [`filebridge.c`](../../libc/src/adapters/tso64/filebridge.c)
copies arguments and records into suitable low storage, checks pointer and
record limits, and calls through the native entry's service table. It is a
synchronous bridge with shared work areas, not a general reentrant service
interface. CMS uses its own adapters and SVC 202 or SVC 204 interfaces.

z/PDOS owns the maintained Classic tools and PDPCLIB source. Its operating
system can also exercise selected unchanged TSO binaries, but that is a
separate execution environment. The SDK neither builds nor includes the
z/PDOS OS, its classic C compiler or a dedicated PDOS application runtime.

## What a profile means

The **instruction set** is the set of operations the processor can execute.
The **application binary interface (ABI)** defines how compiled functions
exchange values and use registers and memory.

Choosing a profile selects these and the other contracts needed to build a
compatible program:

| Part of the contract | The question it answers |
| --- | --- |
| Instruction set | Which machine instructions may the compiled C use? |
| C ABI | How wide are C values, which registers carry arguments, and how is the stack arranged? |
| Addressing | Which addresses can the program and its native services use? |
| Runtime services | How do storage, files, console I/O and return to the operating system work? |
| Character handling | Where are application bytes converted to native EBCDIC? |
| Packaging and residence | What does the loader accept, and where may it place the code? |

The application profiles have separate **sysroots**: directories containing
their headers, libraries and runtime metadata. CMS sysroots also contain
startup and adapter objects. TSO C adapters are compiled by the installed
consumer and linked with the packaged native objects. `vmkernel` has component
headers and source, without application startup or libc.

ELF objects carry an `MLAB1` string in a `.lab.profile` section. For example,
the TSO31 identity is:

```text
MLAB1;isa=s370-int1;addr=31;abi=s390-ilp32-r1-v1;chars=utf8;svc=mvs31-native1;profile=tso-zos31-v1
```

The checkers require the expected identity on the applicable ELF inputs.
CMS31 and TSO31 cannot share library objects merely because they have the
same C type widths. Nor does putting an identity on an arbitrary object
prove that it obeys the contract. The historical checker also checks its
supported encoded instruction ranges and relocations; TSO64 relies on GNU
z900 target selection and structural/identity checks, without claiming an
independent decoder for the entire ISA. Native entry/service decks have
separate checks and guest qualification.

### How a profile is selected in practice

The installed consumer's `--profile` option selects a known build recipe:
compiler flags, the matching sysroot, checker, linker layout, exporter and
native entries. The JSON files describe those contracts; they are not a
general configuration engine that can create a new target on its own.

| Profiles | Compiler selection used by the consumer |
| --- | --- |
| CMS24, CMS31, `vmkernel` | `-ms370-profile=NAME`, using the exact profile name |
| TSO24, TSO31 | `-mexperimental-s370 -m31 -mesa -msoft-float -mno-mvcle`, followed by the appropriate TSO identity and runtime |
| TSO64 | `-m64 -mzarch -march=z900 -msoft-float -mno-vx`, followed by the LP64 identity and runtime |

These are the machine selectors, not complete standalone build commands.
The consumer also sets the C mode, compilation restrictions and include paths.
Changing a JSON label or an assembler flag does not convert one profile's
objects into another. A new profile needs corresponding compiler/runtime,
checking, packaging and execution work.

### The six active profiles

All use big-endian storage. **ILP32** means 32-bit `int`, `long` and pointers;
**LP64** means 32-bit `int` with 64-bit `long` and pointers. A 32-bit pointer
representation does not give a 24-bit machine access to all 32 address bits.

| Short name and exact identity | C and instruction model | Services and delivered output |
| --- | --- | --- |
| CMS24 — `vm370-4381-v1` | ILP32; conservative S/370 integer subset; 24-bit addressing | VM/370 CMS adapter, SVC 202; fixed-origin MODULE, with TEXT export also available |
| CMS31 — `cms20-esa31-v1` | ILP32; retained historical C lowering with a BSM native bridge; 31-bit addressing | CMS 20 adapter, SVC 204; relocatable AMODE31/RMODE ANY MODULE |
| TSO24 — `tso-zos24-v1` | ILP32; conservative historical C lowering; 24-bit application addressing | Native TSO bridge; AMODE24/RMODE24 member |
| TSO31 — `tso-zos31-v1` | ILP32; conservative historical C lowering; 31-bit addressing | Native TSO bridge; AMODE31/RMODE ANY member, with low service storage |
| TSO64 — `tso-zos64-v1` | LP64; z900 instructions, software floating point, no vector extension | Native bridge to narrower TSO services; below-bar image and high heap/data |
| `vmkernel` | Freestanding S/370 ILP32 C; 24-bit component contract | Explicit native entry/glue per component; exported assembler source |

The instruction description for a compiled C object does not describe every
instruction in its native service bridge. In particular, the TSO source
entries are assembled using the Classic Assembler's z900 profile. “TSO24”
therefore does not promise MVS 3.8 or arbitrary S/370 system compatibility.

[`sdk/src/profiles/`](../../sdk/src/profiles/) contains the machine-readable
contracts. Some descriptive fields still refer to the earlier bootstrap
implementation or removed Lab documents; [SDK-005](../../sdk/doc/BACKLOG.md#sdk-005-profile-and-manifest-description-drift)
tracks that issue. They are not release-status records. The old
`vmce-cms-kernel-v1` JSON is retained for migration/rejection history, with
no enabled sysroot.

### Addressing mode and code placement

**AMODE** describes the addressing mode at native entry. **RMODE** describes
where the load image may reside. The **line** is 16 MiB; the **bar** is 2 GiB.
For these classic load members, RMODE ANY allows placement above the line
but below the bar. It does not mean anywhere in 64-bit address space.

The TSO64 package includes two entries for the same LP64 C profile:

| Default example member | Native entry | C execution and code placement |
| --- | --- | --- |
| `SDKTS64A` | AMODE64 | 64-bit C; RMODE ANY image below 2 GiB |
| `SDKTS64L` | AMODE31 entry that switches to 64-bit C | 64-bit C; RMODE ANY image below 2 GiB |

These are two entry variants, not two C ABIs. The runtime can obtain high
data through IARV64 while retaining low native service workspaces. It does
not use IBM Language Environment's C ABI. The separate application experiment
with a low launcher and RMODE64 code above the bar is **outside the 0.1.0
package**. Its bounded PDOS results do not establish modern z/OS support.

## Runtime choices and limits

The source defaults reserve a 64 KiB heap for CMS24, 4 MiB for TSO24,
64 MiB for CMS31 and TSO31, and 128 MiB for TSO64. The applicable build-time
overrides are `LAB_CMS_HEAP_SIZE` and `LAB_TSO_HEAP_SIZE`; changing them means
building the affected runtime or adapter. CMS31 startup also reserves a
3 MiB C stack. Guest storage must accommodate the image, runtime and stack:
the release's CMS31 guest check used a 256 MiB virtual machine.

CMS24's ordinary TEXT exporter has a 2 MiB image cap; the direct MODULE path
has an 8 MiB cap. TEXT decks also have a 65,535-card record limit. Those are
packaging limits, not guarantees that every image of that size will fit in
available guest storage. CMS31's direct MODULE exporter also has an 8 MiB cap.

Application text and native EBCDIC meet in the adapters. All five application
sysroots install `<mainframe_text.h>`. Calling
`mainframe_set_text_conversion(0)` disables their character conversion;
a nonzero argument restores it. This switch belongs to the runtime instance,
not an individual stream. It does not change record boundaries, newline
handling, fixed-record padding or binary I/O. CMS plain `fopen` remains
byte-oriented; the switch affects CMS console and explicit text-adapter calls,
and TSO text-file and terminal I/O. A `chars=utf8` profile label is not a
promise of complete Unicode conversion at every native interface.

The ILP32 raw-console interface needs native entry service version 6; LP64
needs `0x6403`. Mixing a new C adapter with an older native entry can report
`ENOTSUP`. Keep the matching adapters and native objects together.

The selected routines and software arithmetic helpers are not a complete
C99, newlib or floating-point conformance claim. Shared/reentrant deployment,
threads and a general POSIX environment are not established by these profiles.
The [release limits](../RELEASE-PLAN.md#known-limits) identify the concrete
TSO file restrictions and unqualified guest paths.

## Source-built releases and historical bootstrap packages

The release producer builds from checked GNU/Sourceware archives and
maintained SDK and z/PDOS source. It records file hashes and installs each
component's notices. Follow the [source build guide](../../sdk/README.md)
for that route.

The retained `compiler_sdk_package.py` is an older local bootstrap packager.
It consumes pinned runtime archives and historical ASMA90-produced native
objects, including application-specific high launchers. Their hashes and
input names remain in that script and the frozen `sdk/archive/` records.
They are not inputs to `compiler_sdk_source_package.py`, not shipped in
0.1.0, and not a public alternative release recipe. Their redistribution
provenance remains a separate matter.

The SDK originated in Mainframe Lab. cREXX provided a substantial application
consumer for that work; application builds and older guest experiments retain
their own source and qualification records. They do not automatically qualify
the generic SDK release. See [licensing and provenance](../../LICENSING.md)
for the component terms and how the native source build avoids proprietary
assembler and macro-library inputs.
