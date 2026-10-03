# Source-built ELF SDK checkpoint — 3 October 2026

I approved a non-Windows 0.1.0 first, with native Windows builds deferred to
0.1.1 on a Windows host. This report records local development evidence, not a
release. The SDK checkout is on `develop`, based on
`6f969ec3ea1f8a6ded74fd4f0a977bd7bc2c8ac7`, with the changes in this
report uncommitted. z/PDOS is on `develop`, based on
`4ddee3a56f4e8077b8731d4da7888b444edf498f`, with the EPSW addition
uncommitted. The tested host is Apple Silicon macOS (`darwin-arm64`).

## Maintained source and producer

The GCC 16.2.0 selected target files now live in `compiler/src/gcc16/`, and
the selected newlib 4.6.0.20260123 files in `libc/src/newlib/`. Preparation
checks the official upstream archives and those maintained files, then copies
the selected current source into generated build trees. It no longer applies
the old recovery patches. The old series is frozen under each component's
`archive/`. The previous patch-prepared and new direct-source generated GCC
trees compared equal with `diff -qr`, as did the full generated newlib trees.

The fresh compiler/binutils build passed. Its producer receipt is
`build/elf-direct-work/tools.json`; the GCC executable SHA-256 is
`d339ac737184d640656b4dbadc8addbc95535024380eaa7b267e5f2c54d6a517`
and the GNU assembler SHA-256 is
`7f60c55291a02fe0554bf45c67553c0d579cd44e28485984180edb638f41c368`.
The `vm370-4381-v1` profile object smoke passed its post-assembly checker.

An independent build from an archive-free 171-file source extraction also
passed in `build/elf-final-work-v2`; its GCC SHA-256 is
`daa00867d6b23c8c4141358908a2c3e8bcb5f4e17bb8ab64b00b00c1107b13a7`
and assembler SHA-256 is
`7bab3006c2806957b8cd26ccbb1fc61c89a4c653cc8d74219afeac9294036ef2`.
Its checked historical-profile smoke object is SHA-256
`43d300131e4ec81f616917bd93b740898d6ad1b49f3d309310c4a736850861b6`.
The later 173-file extraction includes the TSO24/31 entry recipe and no frozen
archives; it independently passed pinned source preparation and the entry
build. Nine host checkers/exporters compiled from that extracted source.

The current host helpers, profile contracts, linker layouts and kernel
component are in `sdk/src/`; maintained recipes are in component `scripts/`.
The historical PDLD/PDPCLIB patches are in `sdk/archive/`. The release source
inventory excludes all frozen `archive/` directories. A fresh extraction and
preparation from the three checked upstream archives passed on 3 October;
the generated GCC and newlib trees match the earlier direct-source trees.

## Source-built runtime and host consumers

The selected newlib core, extra and math groups and selected GCC wide-integer
and soft-float helpers passed source builds and object checks for CMS24,
CMS31, TSO24, TSO31 and TSO64. The CMS core has 131 unique selected members;
the TSO cores have 106. Both CMS adapter archives and startup objects passed.
The `libc/scripts/` recipes assembled five C sysroots, using only the new
compiler, generated upstream source and maintained SDK adapters for those
stages. The TSO sysroots contain native entry *source*, not a completed native
object.

| Profile | Host consumer result | Output SHA-256 |
| --- | --- | --- |
| `vm370-4381-v1` | Full plain-C consumer linked; CMS MODULE written and verified | `2579da31d0404a0706fd49e17f891dcf52987230929a162c7ba474f858e43cbd` |
| `cms20-esa31-v1` | Full plain-C consumer linked; CMS MODULE written and verified | `944956b3cb2aa86d361e22fbf0ad400925e7292cb35a7083cf49af9d14e70526` |
| `tso-zos24-v1` | Generic C adapters linked; ELF load deck exported | `2c0ce53cfadce65ad90c9bb43c5b026a79e65fbf44ea881d02498e5a008c0c3a` |
| `tso-zos31-v1` | Generic C adapters linked; ELF load deck exported | `441f064ff9af2183eb4dbd9da3ffff281b8960fcffc7f3fa1948eadbb4b12145` |
| `tso-zos64-v1` | Generic C adapters linked; ELF load deck exported; no application high launcher | `6459bb1fe86cff940f5b15a5cfe146db6b4f37bad16d26dfe4d8eac0854e321c` |
| `vmkernel` | Freestanding component compiled and exported as assembly | `3f6f9549775f23eed7cfe1e30272a8b159f866cc1de4954bebe9a5d563647703` |

The fuller CMS consumer initially found wrongly grouped math support and
unplaced `.rela.rodata.*` sections under the strict linker script. Moving the
selected CMS support members into the core archive and explicitly placing
read-only-data relocation sections resolved both. The final CMS24 and CMS31
MODULE writes passed independent verification. These are host checks; none
is a new guest run.

## Native and release gates still open

The z/PDOS Classic Assembler now has the IBM-defined EPSW RRE encoding;
all 37 local assembler checks pass. SDK TSO24 and TSO31 entries now use
selected explicit storage, terminal and dynamic-allocation register
interfaces documented by the independently authored z/PDOS TSO31 bridge.
`libc/scripts/build-tso-entries.crexx` used the source-built Classic
Assembler (SHA-256
`9cf88c0603b38962ead88b9e532000b73603e5e1547db2e12fe64891bb43b7ab`)
to build `build/tso-entries-source/tso24-entry.obj` (SHA-256
`9ca288ce4ef17290c3c99f137663e209fa9e3225ba67c8c9747cab9080482a93`)
and `build/tso-entries-source/tso31-any-entry.obj` (SHA-256
`14866e0635cfa7e1b1075098037b4027751c30a22e14e8bd16a18c9d61e8c8c9`).
`sdk/scripts/tso_entry_source_check.py` passed their section modes, external
closure and independently expected service/EPSW bytes; a changed service
opcode was rejected. These are entry-only host object checks, not completed
load members or guest behavior. The 64-bit entry still needs its IARV64
interface and wider AMODE/instruction/object coverage. The maintained
`tso31-lean` PDPCLIB source was prepared from z/PDOS and Classic Assembler
stopped at `FUNHEAD` on line 679 (unsupported feature status 3); its service
object needs further macro/language and object qualification. No ASMA90 or IBM
macro library is an acceptable 0.1.0 release build input.

The existing `sdk/scripts/compiler_sdk_package.py` still consumes historical
runtime archives and native decks. It is a bootstrap recipe, not a 0.1.0
release producer. The release source extraction excludes all frozen archives
and independently reconstructs the checked GCC/newlib trees. A complete
source-built Classic native route, source-only installed package, Linux host
build/consumer, affected guest checks and asset inspection remain before
tagging or publishing 0.1.0. Windows host evidence is the separate 0.1.1 stage.
