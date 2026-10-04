# Licensing and provenance

**The 0.1.0 SDK can be built without a proprietary mainframe compiler,
assembler, binder or IBM macro library.** Its build uses GNU tools, selected
newlib routines, our SDK code, and source-built Classic tools and PDPCLIB
services from z/PDOS. Each component keeps its own licence.

This claim concerns the build dependencies. It does not grant rights to an
IBM operating system, its images, documentation or other products. Running a
program on z/OS or z/VM requires separately authorised access to that system.
The SDK includes no guest image or entitlement to one.

## How the native build became independent

Earlier laboratory builds used ASMA90/HLASM and IBM macros to assemble native
TSO entry and service objects. The old local bootstrap packager could copy
those objects and previously built runtime archives into an SDK. That route
proved useful interfaces, but left native build inputs outside the public
source tree.

For the 0.1.0 release, we replaced that dependency chain:

| Build step | Source-built release implementation |
| --- | --- |
| Compile C and assemble GNU syntax | Maintained GCC target source and unmodified GNU binutils |
| Build the C runtime | Selected newlib and GCC helper source, plus SDK adapters |
| Assemble native TSO entries | Our original Mainframe Classic Assembler in z/PDOS |
| Supply native service definitions | Explicit SDK entry code and selected, independently authored service macros in the SDK and z/PDOS |
| Supply dataset services | The maintained PDPCLIB `tso31-sdk-files` source selection |
| Link native objects and create load-member transport | Mainframe Classic Linker, derived from PDLD |
| Write CMS MODULEs | The SDK's own ELF-to-MODULE exporters |

No prebuilt **target** runtime archive or native object deck is a release-build
input. Ordinary host C/C++ tools, development libraries, Python and a pinned
host cREXX runtime are prerequisites. The release workflow uses the cREXX
v1.0.0-beta.3 host runtime to execute build recipes; this is not a claim that
every tool in the host bootstrap was built from source during the SDK run.

The native service definitions implement only the operations and parameter
forms used by these entries and the selected PDPCLIB source. They are not a
replacement for the full IBM macro library. The z/PDOS
[TSO interface record](https://github.com/adesutherland/z-pdos/blob/930dbbc13119ab69d6b4ddc4f42e4d6edcf6fd59/pdpclib/doc/architecture/TSO31-INTERFACES.md)
identifies the public interface references and the original implementations.
The SDK's [runtime source record](https://github.com/adesutherland/mainframe-elf-sdk/blob/main/libc/UPSTREAM.md) also states that the older
IARV64 version-zero layout and PC linkage were checked against an IBM macro
edition held in the private laboratory. IBM macro source is neither copied
into this SDK nor consumed by its release build. This is a documented
provenance claim, not a claim that no IBM reference material was consulted.

The `vmkernel` profile stops at a checked component and exported assembler
source. Assembling or installing that component into a particular historical
kernel is a separate workflow, with that environment's own tools and terms.
It does not extend the application release's host build into a complete
source-built CMS or CP operating system.

## Component terms and attribution

The root [MIT licence](LICENSE), copyright 2026 Adrian Sutherland, covers
original project code and documentation. It does not replace inherited terms.

| Component | Origin and licence scope |
| --- | --- |
| GCC compiler and maintained target changes | GNU GCC 16.2.0; GPLv3 terms and file notices, retained in [`compiler/LICENSE`](https://github.com/adesutherland/mainframe-elf-sdk/blob/main/compiler/LICENSE). Modifications under `compiler/src/gcc16/` remain GCC-derived work. |
| GCC runtime helpers | Selected GCC library source; retain each file's terms and `COPYING.RUNTIME`. The runtime exception applies to files bearing its notice and subject to its conditions, not to the whole compiler. |
| GNU binutils | Unmodified GNU binutils 2.47; retain the upstream licences and notices for the installed assembler, linker and supporting material. |
| newlib | Sourceware newlib 4.6.0.20260123; a collection with multiple file-level notices, reproduced in [`libc/LICENSE`](https://github.com/adesutherland/mainframe-elf-sdk/blob/main/libc/LICENSE). The maintained configuration/header changes retain those terms. |
| SDK adapters, profile machinery, exporters and original guides | Original Mainframe Lab/SDK work under MIT, subject to any inherited file-level notice. |
| Mainframe Classic Assembler | Original implementation by Adrian Sutherland under MIT. Its [source record](https://github.com/adesutherland/z-pdos/blob/930dbbc13119ab69d6b4ddc4f42e4d6edcf6fd59/assembler/UPSTREAM.md) distinguishes it from inherited assemblers and opcode tables. |
| Mainframe Classic Linker | Inherited PDLD code and documentation are public domain according to their upstream notices. Original z/PDOS changes and integration use MIT; that grant does not restrict the inherited dedication. See its [licence](https://github.com/adesutherland/z-pdos/blob/930dbbc13119ab69d6b4ddc4f42e4d6edcf6fd59/linker/LICENSE) and [source record](https://github.com/adesutherland/z-pdos/blob/930dbbc13119ab69d6b4ddc4f42e4d6edcf6fd59/linker/UPSTREAM.md). |
| PDPCLIB native services | Paul Edwards's Public Domain C Library, with contributor notices retained. Its [licence](https://github.com/adesutherland/z-pdos/blob/930dbbc13119ab69d6b4ddc4f42e4d6edcf6fd59/pdpclib/LICENSE) preserves the public-domain dedication and fallback permission. Original z/PDOS changes and service definitions use MIT without relicensing inherited code. |
| Host cREXX | External build-recipe runtime from the [cREXX project](https://github.com/adesutherland/CREXX/tree/v1.0.0-beta.3), with its own component notices; it is not an SDK target runtime. |

PDPCLIB attribution includes Paul Edwards and the contributors named in its
[source record](https://github.com/adesutherland/z-pdos/blob/930dbbc13119ab69d6b4ddc4f42e4d6edcf6fd59/pdpclib/UPSTREAM.md),
including Dave Edwards, Gerhard Postpischil, J. Reginato, Chris Langford,
Dave Jones and Steve Rhoads. Our use of its TSO support is not a blanket
licensing statement about every historical PDPCLIB port. The maintained
source record identifies a later VSE implementation with a copyright question
that was not imported by the runtime merge.

## Using and redistributing the SDK

Open-source licences still impose their own conditions. Preserve the applicable
notices and meet the source-distribution requirements when redistributing
covered binaries. Calling the original SDK code “MIT” does not make the
bundled GCC compiler or its target changes MIT-licensed.

The licence of a compiled application is a separate question from the licence
of the compiler executable. The
[GNU GCC Runtime Library Exception](https://www.gnu.org/licenses/gcc-exception-3.1.en.html)
provides additional permission for covered runtime code combined with
independent modules through eligible compilation processes. Read its terms
for the actual code being linked. Newlib likewise has
[component-specific notices](https://sourceware.org/newlib/COPYING.NEWLIB);
there is no single SDK-wide grant overriding them.

Installed packages retain the SDK licence scope and the Classic Assembler,
Classic Linker and PDPCLIB licence/source records in `notices/`. Application
sysroots carry `COPYING.NEWLIB`, `COPYING3` and `COPYING.RUNTIME`. GNU host
tool installation trees retain their accompanying material. These records
explain provenance; the applicable licence texts govern redistribution.

## Source inventory and older inputs

[`sdk/source-files.txt`](https://github.com/adesutherland/mainframe-elf-sdk/blob/main/sdk/source-files.txt) is the release source allowlist.
`SOURCE-MANIFEST.json` records hashes for that inventory and the locked
upstream archive identities. The producer builds unmodified upstream content
from those archives and overlays the maintained GCC and newlib files.

| Maintained source | Classification |
| --- | --- |
| `compiler/src/gcc16/**` | GCC-derived source under GCC terms |
| `libc/src/newlib/**` | newlib-derived source under its component terms |
| Original adapters, `sdk/src/**`, recipes, checks and prose | Original project material under MIT, with file notices taking precedence |
| Licence texts and upstream notices | Retained under their own terms; not relicensed |
| Downloaded GNU, newlib and z/PDOS inputs | Separate components governed by the source and notice records above |

The frozen `compiler/archive/`, `libc/archive/` and `sdk/archive/` directories
preserve recovery history, including inherited repair patches. They remain in
Git under their applicable terms but are excluded from release source
extraction and normal release builds.

Historical ASMA90-produced TSO objects and cREXX application high launchers
are absent from the public source inventory and the 0.1.0 source-built package.
The older `compiler_sdk_package.py` records their pinned identities for local
bootstrap comparison. Those objects' redistribution provenance remains
separate; do not infer permission to distribute them from this project's MIT
licence. The public tree contains no IBM images, native object bundles,
credentials, private correspondence or cREXX application/platform source.
