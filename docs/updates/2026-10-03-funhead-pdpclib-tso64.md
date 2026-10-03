# Native PDPCLIB assembly and TSO64 review — 3 October 2026

This is a source-build checkpoint on Apple Silicon macOS (`Darwin arm64`),
not a guest qualification or SDK release. The SDK analysis started from
`develop` commit `f96e5171d86ec2c900dc3231528d1c99cdc2eb01`;
the maintained z/PDOS assembler and PDPCLIB work is `develop` commit
`e35412dd11e8c3a2e0f6607a0a2b9a875319c13c`. The SDK remains their consumer.

## FUNHEAD and a source-built PDPCLIB object

The earlier stop at the `FUNHEAD` invocation in `tso31-lean` was misleading.
The assembler expands the source-owned macro; the first unsupported operation
inside that expansion was `GETMAIN`. The Classic Assembler CLI now reports the
expanded operation and macro-model coordinate before its invocation error.
The new fixture extracts the actual maintained `FUNHEAD` definition and
independently checks the resulting entry, branch, EBCDIC identifier and
linkage bytes. No assembler instruction or macro-language change was needed
for `FUNHEAD` itself.

PDPCLIB already owns selected PDOS31 service and mapping macros in
`pdpclib/src/interfaces/pdos31/` and selected linkage macros in
`pdpclib/src/interfaces/classic-linkage/`. A new retained cREXX recipe uses
those source files with the source-built Classic Assembler. It assembled the
complete maintained `pdos-zarch` MVSSUPA selection: 3,981 statements, 20
sections, 908 symbols and 50 fixups. The exact source SHA-256 is
`8327b212648224f25dc0d0268b8d32fffb69ca36cce5cd304c08f75852f3c3d0`;
the resulting classic object deck is
`9a7a69a5501edd1dbba0b60dd371c16636ce577d2cb1a41f59fade04f8f3fb58`.
The assembler executable is
`cad7d79bd3310bee5d22f8f1e843fa735673fd7432c983197bf7ee69cad505ca`.
The `pdpclib_source_macros` check now includes whole-source assembly; the
full local z/PDOS suite passed 93/93. The check's object and the independent
native recipe's object have the same SHA-256 shown above.

This is a native PDPCLIB **assembly** result for the PDOS profile. It does
not establish a linked SDK service member, a newly run PDOS guest, or TSO
compatibility. The PDOS31 macro implementations express selected PDOS
services, not IBM TSO interface layouts.

## TSO31 service source still open

The same source-built recipe, selecting `tso31-lean`, advances to prepared
source line 1917 and stops at `TERMDCB PUTLINE MF=L`. This is a TSO terminal
parameter-list definition, rather than a missing assembler opcode. The
remaining MVS/TSO path also references GETLINE, service forms and mappings
for terminal I/O, VSAM, TSO control blocks and selected system services.
For example, source paths reference `TCBJSCB` and `TCBFSA`, which the
PDOS31 `IKJTCB` mapping does not provide. A draft file-only gating experiment
under ignored `build/` moved past terminal and optional system routines but
then reached `TCBJSCB`; it was not retained as product source or counted as a
passing object. Skipping those routines without an agreed service contract
would hide unsupported SDK behavior.

The next TSO31 implementation step is a source-owned, edition-matched set of
the selected TSO list/execute service forms and control-block mappings, or an
explicitly scoped service profile that fails unsupported calls truthfully.
The five entry callbacks use `@@AOPEN`, `@@AREAD`, `@@AWRITE`, `@@ACLOSE`
and `@@DYNAL`; their object exports, parameter layouts, classic link and
actual guest behavior need independent checks. IBM's [PUTLINE list form](https://www.ibm.com/docs/en/zos/3.1.0?topic=instructions-list-form-putline-macro-instruction),
[PUTLINE execute form](https://www.ibm.com/docs/en/zos/2.5.0?topic=instructions-execute-form-putline-macro-instruction)
and [TSO IOPL](https://www.ibm.com/docs/en/zos/3.1.0?topic=routines-inputoutput-parameter-list)
describe separate interfaces; the target z/OS 1.5 edition and guest behavior
must govern the implementation. The current [TCB information](https://www.ibm.com/docs/en/zos/3.1.0?topic=xtl-tcb-information)
describes offsets but also distinguishes supported programming-interface
fields, so a guessed map from the PDOS implementation is insufficient.

## TSO64 needs

There are two distinct maintained source paths:

1. `libc/src/adapters/tso64/entry64.asm` starts with AMODE31/RMODE ANY,
   enters LP64 C through `SAM64`, and keeps native service calls low. The
   current assembler stops at `GETMAIN` on line 19. This route needs selected
   z/OS storage, TPUT/TGET, dynamic allocation and IARV64 interfaces; it also
   uses z900 LP64 instructions and 64-bit constants. The source owns both
   IARV64 execute and list forms. Complete source assembly, independent deck
   checks, service link and a guest run remain open.
2. `libc/src/adapters/tso64/entry64-any.asm` is the accepted low-residence
   AMODE64/RMODE ANY entry for the existing application baseline. The current
   assembler stops at `AMODE 64` on line 4. That requires explicit AMODE64
   section/object metadata as well as the selected service interfaces,
   instruction/constant coverage and linker acceptance. The accepted native
   object described in the SDK guide is still a pinned bootstrap input, not
   a source-built release output.

The optional **high-residence** application path is separate. It needs
full-width linkage/relocation and a suitable load format or binder proof;
AMODE64/RMODE ANY support alone does not prove code can reside above the bar.
The low launchers may remain in Mainframe Lab as application consumers, as
already agreed. A 0.1.0 SDK must source-build and qualify the generic TSO64
path it advertises, then package that path without a prebuilt native deck.
The 0.1.0 release remains open, including the TSO31 service object, TSO64
entry, complete native links, installed consumer and affected guest checks.
Windows remains the planned 0.1.1 host stage.
