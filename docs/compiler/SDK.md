# Mainframe ELF SDK

The component names are Mainframe ELF C (`mf-elf-cc`), Mainframe ELF Assembler
(`mf-elf-as`) and Mainframe ELF Packager (`mf-elf-pack`). These are the agreed
public interface names; the commands below describe the existing implementation.

This guide describes the local SDK producer and its limits. I keep the modern
GCC 16.2 route available as a versioned C cross-compiler for historical CMS,
TSO and declared CP/CMS kernel components. cREXX is an external application
consumer. The legacy GCCCMS/GCCMVS tools and their repair
history remain in the laboratory for other work. This extraction is separate
from a clean z/PDOS64 kernel project; it does not include a dedicated PDOS
application target, runtime, image or guest. Shared PDLD/XMIT tools remain
relevant because PDOS tests unchanged TSO binaries.

## Included profiles

Every object has one exact `MLAB1` identity from `sdk/src/profiles/`. The
SDK installs six active profiles. The superseded `vmce-cms-kernel-v1` JSON is
retained under `profiles/` for migration/rejection evidence and has no sysroot.

| Profile | C and addressing | Runtime and output | Current limit |
| --- | --- | --- | --- |
| `vm370-4381-v1` | S/370 integer subset, ILP32, 24-bit guest addresses | CMS newlib, startup, TEXT/fixed-origin MODULE | 2 MiB ordinary TEXT cap, 8 MiB direct MODULE cap, 65,535 card records |
| `cms20-esa31-v1` | Same conservative C lowering plus BSM service bridge, 31-bit addresses | CMS 20 newlib, SVC 204, relocatable MODULE | CMS 20 guest slice; modern z/CMS remains unqualified |
| `tso-zos24-v1` | S/370 integer subset, ILP32, 24-bit service boundary | MVS/TSO newlib, AMODE24/RMODE24 native deck | No complete cREXX fit claim |
| `tso-zos31-v1` | S/370 integer subset, ILP32, 31-bit addresses | MVS/TSO newlib, native deck/XMIT, accepted RMODE ANY route | Low native service workspaces retain their address limits |
| `tso-zos64-v1` | z900 LP64, 64-bit C pointers, high data | MVS/TSO newlib, accepted below-bar RMODE ANY package; separate low launcher and RMODE64 high code candidate | High route has bounded PDOS execution; modern z/OS execution remains unverified |
| `vmkernel` | Freestanding S/370 ILP32 C, 24-bit CP/CMS component | Native assembler entry/glue and explicit component helpers; no application startup/libc | Component-specific closure, no 64-bit kernel ABI |

The compiled object identity, C ABI, ISA, service contract, residence, encoding
and native package form are distinct checks. An ELF file is not a CMS MODULE or
an MVS load member. TSO64's high route is explicit and does not replace the
default RMODE ANY package. External application checks use a frozen cREXX
baseline and matching libraries; ongoing codec work is outside this
qualification. This public source tree contains no cREXX application source.

## Standalone producer

`sdk/source-files.txt` lists every copied file. The extraction command copies
only that allowlist and writes `SOURCE-MANIFEST.json` with SHA-256 for every
file. It excludes private correspondence, manuals, IBM images, credentials,
book drafts, unrelated experiments and downloaded vendor trees. The extracted
tree carries editable GCC target source, frozen historical recovery material,
profile definitions, checked runtime adapters, package tools and relevant
source guides. The producer reads the maintained source directly from
`compiler/src/gcc16/` and does not read the frozen recovery material.

```sh
python3 sdk/scripts/compiler_sdk.py extract --out /tmp/mainframe-compiler-sdk-source
cd /tmp/mainframe-compiler-sdk-source
python3 sdk/scripts/compiler_sdk.py prepare --cache /tmp/sdk-archives \
  --work /tmp/sdk-work --download
python3 sdk/scripts/compiler_sdk.py build-tools --work /tmp/sdk-work
```

`prepare` checks the official GCC 16.2.0, GNU binutils 2.47 and newlib
4.6.0.20260123 archives against locked SHA-256, checks the selected pristine
GCC files, then copies maintained compiler and newlib files from their
respective `src/` trees. It refuses
an existing work root. The cache may instead contain read-only checked
archives. `build-tools` builds and installs binutils and C-only GCC from that
fresh source. It does not build GCC inside historical CMS or select a new
triplet. The retained target remains `s390-linux-gnu`, with explicit profile
options and separate non-Linux services.

On Apple Silicon macOS, the current host prerequisites are Homebrew `gcc-16`,
`g++-16`, GMP, MPFR, MPC and ISL; Python 3.12+, make, patch and common archive
tools are also needed. On Linux, install the corresponding development
packages and a working host GCC/G++. A host cREXX executable is also needed
for the selected runtime and sysroot recipes in `libc/scripts/` and for the
laboratory's external cREXX application consumer.
A target RXC or RXVM does not bootstrap this SDK.

## Runtime/sysroot packaging

The maintained recipes in `libc/scripts/` build selected newlib, math and
GCC helper archives for all five application profiles. They also build the
CMS adapters and startup and assemble five checked C sysroots. On 3 October
2026, the Mac host route passed all five source-built C consumers: CMS24/31
MODULEs and TSO24/31/64 ELF decks. The freestanding `vmkernel` component
also compiled and exported. These local results are described in the dated
source-build report. Classic Assembler now builds checked TSO24, TSO31 and
both TSO64 entry objects from maintained source. The selected PDPCLIB TSO
file service now assembles from z/PDOS source and four complete native XMIT
links pass. A source-input installed candidate passes all six profiles on
macOS. Linux, clean-checkout and affected guest gates remain open.

The older bootstrap package assembler accepts the separately qualified CMS24, CMS31
and three-profile MVS/TSO newlib archives as **checked bootstrap inputs**.
Their exact archive hashes are pinned in `sdk/scripts/compiler_sdk_package.py`. It
copies each profile's generated headers, static archives, startup and native
source into a separate sysroot, installs the fresh host compiler/binutils and
host checkers/exporters, and writes `SDK-MANIFEST.json` with per-file hashes,
source/runtime identities and supported actions. The package is a deterministic
local tarball. An independent consumer can unpack it elsewhere and verify
every installed file without the lab checkout or producer build directory.
The TSO31/64 XMIT route requires a separately built and checked Classic Linker;
the bootstrap package carries historical PDLD patches and an unload helper,
but does not install the linker or claim an end-to-end XMIT action without it.
The SDK's `sdk/archive/pdld/0001-pdld-only-multi-csect.patch` contains only
PDLD writer changes. The original combined PDLD/PDOS loader patch and its
kernel changes remain in Mainframe Lab. On the pinned PDOS source archive, the
split yields identical PDLD source files and XMIT fixture bytes.
The bootstrap package step does not consume the newly source-built sysroots;
its input archives retain their own older provenance. The separate
`sdk/scripts/compiler_sdk_source_package.py` takes source-built sysroots,
Classic tools, native objects and the z/PDOS source root for component
notices, and installs them without those archives.
Its independent installed consumer passed on macOS, including complete TSO
XMIT links. The [source candidate checkpoint](../updates/2026-10-03-sdk-source-candidate.md)
records inputs, outputs and open release gates.

The retained baseline native TSO entry and service objects are pinned bootstrap
inputs. `TENTRY.obj` is SHA-256
`9d1b7813847ad9f27b67f8401dac5f3972d14216c8794460f6a76d6c10d25270`;
`PDPSUP-tso.obj` is
`79c232b808201729d54d62f5ba1017dee5fc68decb6ae4790141cf819509f182`.
The accepted RMODE ANY paths use lean service `PDPL34.obj` SHA-256
`2e513bb7535c1a7fceecd5954790845cd3d4affb521a68644fcf773c0e6c8e53`;
the accepted TSO64 entry is `E64.obj` SHA-256
`00088a98ea3392363464422b0bbc7bf054f5e92304fbdfa48fc6fc3a7b6a0df0`.
These are distinct native inputs. The retained `tso-native-decks-v1` manifest
names its baseline source hashes and ASMA90 job. `libc/src/adapters/tso/entry31.asm`,
`libc/src/adapters/tso64/entry64-any.asm`, PDPCLIB `mvssupa.asm` at pinned PDOS
revision `0fe81209e78d022b40301f86f97c7f4d3e406d0a`, its three visible
patches, and `pdptop.mac` are their source trail. The SDK's
[`0004-zos15-tso24-swareq.patch`](../../sdk/archive/pdpclib/0004-zos15-tso24-swareq.patch)
extends the 0001-patched PDPCLIB source for z/OS 1.5 TSO24; the
[`PDPCLIB patch guide`](../../sdk/archive/pdpclib/README.md) pins the input and
output hashes. The packaged baseline `T24SUP` is not that patched object;
reassemble the patched source for z/OS 1.5 TSO24 dataset I/O. PDPCLIB's source notice
credits Paul Edwards and contributors and states its own public-domain claim;
that notice does not apply to all SDK material. The IBM macro library and
ASMA90 are native/private prerequisites of the retained bootstrap route.
The source-input candidate replaces these native decks with maintained source
objects. The private bootstrap deck bundle is retained only for historical
comparison and must not be distributed as part of that candidate.

The separate RMODE64 candidate pins the three cREXX low launchers
`LAU65O.obj`, `LAVM65O.obj` and `LAC65O.obj`, respectively SHA-256
`f0edb7af6caf446259c5f5de303c0ac570c63a173d58e161c5b526bd5e53bcdb`,
`66ecc7b09dc775ff89b68a66fd2dbafb886e6d1860821467b2618d0a8e9fa7fc`
and `1a5bb86fe381b0ea1de813bc179fc9dcd3d6f38996b25a62daff8855cc9b85fe`.
They came from a retained laboratory application launcher source with ASMA90
and IBM macros; that app-specific source is outside this public extraction.
The package keeps them in a distinct `native/high-launchers/` directory;
`libc/src/adapters/tso64/high-entry.s`, `sdk/src/layouts/tso64-high-image.ld` and the high
exporter mode define the portable code side. No high launcher is substituted
for the accepted RMODE ANY default.

GCC is covered by its GPLv3 and runtime exception, binutils and newlib by
their original notices. The package retains those terms rather than applying
one blanket licence. No proprietary commercial licence is introduced as an
SDK policy. The native regeneration gap prevents describing the whole SDK as
freshly built solely from open-source inputs.

The package command takes four explicit, checked bootstrap archives and the
separately accepted RMODE ANY native objects. It
requires a completed `tools.json` from `build-tools`:

```sh
python3 sdk/scripts/compiler_sdk_package.py --work /tmp/sdk-work \
  --cms24 /path/to/cms-newlib-vm370-4381-v1-closeout-20260924.tar.gz \
  --cms31 /path/to/cms-newlib-cms20-esa31-v1-closeout-20260924.tar.gz \
  --mvs /path/to/mvs-newlib-2026-09-24-v1.tar.gz \
  --native-decks /path/to/tso-native-decks-v1.tar.gz \
  --native-service-any /path/to/PDPL34.obj \
  --native-entry64-any /path/to/E64.obj \
  --native-high-launchers /path/to/checked-high-launcher-directory \
  --out /tmp/mainframe-compiler-sdk-0.1.0-local
```

After unpacking the resulting `.tar.gz` at another location, run the
consumer matrix from the installed package:

```sh
python3 /new/location/mainframe-compiler-sdk-0.1.0-local/tools/compiler_sdk_consume.py \
  --sdk /new/location/mainframe-compiler-sdk-0.1.0-local \
  --out /new/location/sdk-consumer
```

The consumer checks every installed file, compiles and checks each profile's
object, links a small C program to the CMS runtime and writes/reads a MODULE,
links the TSO profile samples and emits native C decks, and checks a bounded
`vmkernel` component object. It also links a TSO64 high-entry ELF image,
checks the entry/stack and full-width relocations, and emits a high native
deck while verifying the pinned low launcher objects. Those host checks are
not a new native execution
claim. The RXC/RXAS/RXVM host consumer is a separate laboratory check; new runtime
source rebuilds, modern z/OS RMODE64 execution, and exact packaged beta 3
manual release checks remain separate gates. Previous guest evidence applies only while the
retained bytes and contracts match; changing native adapter bytes calls for
focused guest execution.

For the frozen cREXX 0017 application chain, the laboratory supplies its
checked source tree, source manifest, separately built matching host cREXX,
and a hash-pinned set of application recipes and platform glue. A lab-side
consumer verifies these inputs and the installed SDK, then builds and checks
RXAS, RXVM and RXC TSO64 native decks. That source and its application glue
are not part of this public compiler SDK. The retained host result does not
create XMITs or establish fresh guest execution.

`.github/workflows/compiler-sdk-producer.yml` defines candidate macOS arm64
and Linux arm64 fresh-source tool builds. A YAML definition is not a hosted
result. The laboratory's dated SDK report records the actually executed
producer/consumer matrix and artifact identities; it is not a runtime
dependency of the standalone source tree.

## Application heap defaults

Future builds use a 64 MiB heap for CMS31 and TSO31, and 128 MiB for LP64
TSO64. The historical CMS24 default remains 64 KiB; TSO24 uses the maintained
4 MiB limit. `LAB_CMS_HEAP_SIZE` and `LAB_TSO_HEAP_SIZE` remain explicit build
overrides. A deliberately small heap belongs to a named failure probe, rather
than an ordinary application build.

The CMS31 startup reserves a 3 MiB C stack. Its paint count, scan bound and
stack reservation agree; the existing guard checks remain in place. A native
assembler measurement used 113,356 bytes, exceeding the earlier 64 KiB stack.

These source defaults do not alter frozen runtime archives or installed
packages. New runtime variants need separate build and affected guest
qualification. See the [heap-default change and focused checks](../updates/2026-09-30-heap-defaults.md).

## Application text conversion

New SDK packages built from this source install `<mainframe_text.h>` in all
five CMS24/31 and TSO24/31/64 application sysroots. Call
`mainframe_set_text_conversion(0)` before text I/O
to leave native character bytes unchanged; a nonzero argument restores the
default conversion. The flag belongs to one runtime instance, not a stream.
The existing record, newline, fixed-record padding and binary paths retain
their behavior. CMS plain `fopen` stays byte-oriented; the switch applies to
the explicit CMS text adapter and the CMS console. TSO `fopen` text and
terminal I/O use the switch. Generic newlib and `printf` formatting are
unchanged. The narrow implementation and exact checks are in the
[dated text-switch report](../updates/2026-09-29-text-conversion.md).

TSO raw console output needs the matching native entry service version 6
(ILP32) or `0x6403` (LP64). The existing PUTLINE callback skips its translation
for the runtime's raw request. A changed C adapter linked with an older native
entry reports `ENOTSUP` on disabled console output; build the updated entry
object with the same native assembler and bind it with the profile's retained
PDPCLIB service object.
