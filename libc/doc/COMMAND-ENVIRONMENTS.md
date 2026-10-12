# Native command environments

The first command client is the Mainframe ELF SDK/newlib application. Its
selected CMS31 and TSO31 adapters expose `command.h`: open a borrowed context,
query a named environment, execute a command synchronously, and close the
client handle. Application environment registration and callbacks remain later
work. The upstream newlib core is unchanged.

`mf_command_host_execute()` is the separate synchronous native host-command
entry. CMS opens and closes its logical CMS session around CMSCALL. TSO calls
IKJEFTSR directly without IRXINIT, so a basic command does not require an IBM
Rexx processor. The original session API still borrows a context for discovery
and optional PDOS invocation. Callers must not retain an OS ENVBLOCK pointer
across unrelated command calls; cREXX's selected driver borrows and releases it
for each discovery/invocation. No IBM processor is initialized by this API.

The current cREXX candidate also consumes generic native directory adapters:
CMS FST snapshots and TSO binary 256-byte PDS directory records. The SDK owns
record decoding, bounds, aliases, malformed/truncated refusal and balanced
stream ownership. cREXX owns its library-prefix and filename-root policy.
Canonical newlib core/extra/math archives are prepared from the checked
official source plus the two maintained SDK overlays; no cREXX-specific libc
stubs replace environment, pushback or mathematical routines. Candidate
consumption records explicit archive/source hashes and does not rewrite the
base release manifest or claim a new released SDK.

The optional `mf_pdos_command_execute` extension uses the separately named
`PDOS` vocabulary. It validates/encodes the whole command before querying
PDOS. Absence returns MF_COMMAND_UNSUPPORTED. CMS invokes SUBCOM PDOS; TSO
uses the separate PDOSCMD facility. The existing CMS and TSO names and their
native invocation contracts are unchanged.

Build the new optional clients and their child/fault controls with
`libc/scripts/check-pdos-command.crexx`, using the same two absolute arguments
as the baseline recipe below. It produces CMS PDENV, TSO PDENV31 and TSO31
ENVCH31/ENVFL31 packages. Package dataset aliases on PDOS are ENVCHILD and
ENVFAULT. These are separate fixtures; the previously qualified ENVFIX/ENV31
binaries remain frozen and unchanged for the PDOS compatibility run.
The PDOS fixture checks the shared vocabulary, a real child returning RC7,
a contained child fault, subsequent command success, unbuffered caller input
and a 128 KiB storage guard. Both original native clients and the optional
clients pass the bounded PDOS model5/model2/line matrix; the dated record
owns exact hashes and evidence. This does not supply a Rexx processor.

CMS uses SUBCOM query and CMSCALL/SVC 204 with CALLTYP SUBCOM and a four-word
extended parameter list. TSO borrows the current Rexx language processor with
IRXINIT FINDENVB and queries IRXSUBCM. Generic C invokes TSO commands through
the TSO/E Service Facility IKJEFTSR, with six native parameters and unauthorized
unisolated synchronous flags X'00010001'. Other Rexx host environments are
queryable, but C invocation returns MF_COMMAND_UNSUPPORTED without a native
call. IRXHST is documented for compiler runtime processors; our direct C
attempts failed natively and are retained in the qualification record.
FINDENVB return
codes 0 and 4 both mean success; 4 identifies a parent-task environment. The
client never initializes or terminates a language processor. An absent context
is reported explicitly. These are distinct native binary interfaces.

For CMS20, the SVC204 input word in R15 carries the call type in its high byte,
COPY/FENCE at X'0000A000' and EPLIST at X'00001000'. SUBCOM with the extended
list therefore uses X'0200B000'; the supervisor supplies the callee's USERSAVE
flags. This was checked against the actual CMS20 reference before execution.

The TSO31 native entry adds service-table version 7 with a command-call slot
after the established fields. C checks that version before reading the slot.
The native bridge performs LOAD with return requested, calls the selected
entry using BASSM, then balances each successful LOAD with DELETE. The local
native save area is chained to ENTRYSA, with the previous forward link restored;
C's R13 is not used as a native save-area address. Native
service calls stay outside TSO ELF objects. Earlier table versions remain
usable for their existing operations; command clients require version 7.
TSO24 and TSO64 command clients are outside this slice.
The selected Classic Assembler has no BASSM mnemonic; the native entry emits
its independently specified RR bytes X'0CEF', checked by the deck inspector.
This does not claim new assembler mnemonic coverage.

Names and command strings at the ELF C API are UTF-8, explicitly converted to
IBM-1047. Names are validated, uppercase and padded to eight bytes. Command
text is length-delimited, limited to 32767 input bytes and never tokenized,
truncated or silently replaced. Malformed UTF-8, an unrepresentable character
or an embedded zero is rejected before a native call. This conversion is
independent of the runtime file/console conversion switch.

`MfCommandResult` records the native service return and a valid command return
separately. TSO facility RC0/4 provides a valid signed binary command result;
other facility returns are native errors, with reason and abend fields retained.
Interpret reason/abend fields only for the facility returns that define them;
the native success/nonzero-command paths leave -1/X'FFFFFFFF' sentinels.
Module LOAD/DELETE failures remain separate from the service return. CMS
supplies a single native return; its service and command
values consequently coincide. A nonzero command return is a completed call.
Console output uses the native environment's terminal/DD and is delimited by
fixture markers; the API does not yet capture that output into a C buffer.

Query one actual name at a time. The fixture displays CMS/COMMAND/XEDIT or
TSO/MVS/ISPEXEC/ISREDIT plus an absent name. This is a known-name probe, not a
complete enumeration. No control-block chain is traversed or modified.
Sessions must be zero-initialized and used synchronously by one client. The
existing startup/runtime remains single-invocation; nested application
callbacks need their own later ownership and reentrancy design.

Run the maintained bounded host/build recipe from the SDK root:

```sh
crexx -nokeep libc/scripts/check-command.crexx --args \
  /absolute/path/to/verified-0.1.0-sdk /absolute/path/to/new-output
```

It verifies the installed SDK, copies a local candidate, rebuilds only the
changed adapter/native entry inputs, runs C89 sanitizer controls, checks the
encoded ELF profiles and produces a CMS MODULE and TSO XMIT. The original
SDK is preserved. `--command-adapters` is a development overlay; its copied
SDK manifest still identifies the base inputs and is not a new installed
release manifest. The result receipts identify the changed sources and
outputs. Guest execution requires the Mainframe Lab operator and leases.

CMS invocation is `ENVFIX ENVINPUT DATA A`. The file has two variable records
containing opaque ASCII bytes `ENV FIRST` plus LF and `ENV SECOND` plus LF,
of lengths 10 and 11. Plain CMS fopen concatenates these records. CMS31 was
qualified with 256 MiB virtual storage; the operator restores the original
64 MiB at handback. 256 MiB is a tested profile with headroom, not a measured
minimum.
The final CMS31 binary passed all 18 checks and native RC0 on CMS20. On TSO,
allocate DD ENVINPUT to `LABA01.ENVCMD.INPUT` containing those two native
variable text records without embedded LF (RECFM VB, LRECL 80, BLKSIZE 800),
then call ENV31 with `DD:ENVINPUT`. Fixed-record padding is deliberately
preserved by the runtime and does not match these short exact marker records.
The retained `tests/command/run-tso.rexx` supplied the original failed
active-parent control: native TSO queries passed, but the former IRXHST calls
returned IRX0812E/service RC20 while the Rexx caller was executing.
`tests/command/run-tso.clist` is the direct-from-READY context control, invoked
with `EXEC 'LABA01.ENVCMD.EXEC(ENV31C5)' CLIST` after exact member readback.
It saves &LASTCC immediately and exits on nonzero RC. DD ENVINPUT is allocated
before the harness runs. Its error routine prints even a symbolic abend code
and exits with numeric RC16. Neither harness initializes or registers a Rexx
processor. The repaired IKJEFTSR path passed natively on z/OS 1.5 through
this direct CLIST caller, including command RC0/8, later command success and
caller-file/storage preservation. An active Rexx caller of the repaired
binary, compiled Rexx and ISPF contexts have not been qualified.
The fixture holds that file open across TIME/QUERY TIME, the longer STATE/
LISTDS command, a controlled failing command and a successful subsequent call.
CMS uses an absent command; TSO uses LISTDS on a checked absent task dataset.
It checks
the caller's remaining file data, 128 KiB storage guard and close/reopen.
Input is unbuffered before the first read, so the later read reaches the native
file path instead of succeeding only from a prefetched C stdio buffer.

The [dated qualification record](qualification/COMMAND-ENVIRONMENTS-2026-10-10.md)
distinguishes host checks from real guest outcomes. This is not yet a claim
of complete CMS/TSO command support, Classic/PDPCLIB qualification or cREXX
runtime integration. It includes the selected unchanged-binary PDOS runs.

The ADDRESS campaign's full current G import exhausted the initial CMS31
64 MiB application heap on real CMS at 256M virtual storage. A subsequent
RXC -v and QUERY DISK succeeded, with the failed output still its placeholder.
This is an application-profile capacity finding, not evidence of a CMS OS
allocator defect. The initial revised 256 MiB heap was rejected at512M virtual
storage during RXC -v, before ADDRESS/import processing. Real CMS20's unchanged
CMSSTOR fixture expansion confirms the SDK's byte count, conditional flags and
32-byte parameter lists; the modern 40-byte/fence mapping does not describe this
old macro. The native serial oracle at 512M obtains, checks endpoints and
releases 64/128/192 MiB blocks; 256 MiB is unavailable, final RC 4, with normal
QUERY DISK recovery. The selected local candidate uses 192 MiB with temporary
512M virtual storage. That small native oracle does not measure full compiler
capacity/high-water. The full matching compiler/library chain and two B/two
Classic runtime lifetimes now pass on real CMS31 and TSO31 and their PDOS
emulations, with independent output/recovery and normal fleet handback. The
selected cREXX native tools use 192 MiB heaps; the native application high-water
remains unmeasured. Exact candidate/runtime/fixture boundaries are in CREXX's
`docs/qa/mainframe-address-2026-10-12-runtime-qualification.md`; the Lab's dated
operator record owns private guest receipts.
The released/default SDK selection is unchanged.
`build-cms-adapters.crexx` accepts an optional fifth CMS31 heap-byte argument;
the local consumer overlay has `--cms-heap-bytes`. Both require doubleword
alignment and 1 KiB..512 MiB, reject a 24-bit override, and record the selection.
The overlay replaces the known `cms-c2.o` syscall member rather than appending
a competing definition. Full Classic imports must retain the current G library.

`build-tso-fault-leaf.crexx` builds the no-allocation MFFLT31 native TSO control
from `tests/command/tso-fault-leaf.asm`. Run it only through the reviewed
IKJEFTSR CALL path; it is not a CMSCALL containment promise. Its object is
reproducible; XMIT transport timestamps require pinning each produced package.
Its four arguments are the classic assembler, classic linker, fresh absolute
output directory and independent SDK XMIT reader. The producer passes a
basename to classic-ld and checks the exact member MFFLT31, AMODE 31 and
RMODE ANY before freezing before/after input and output hashes. Native restore
must also list MFFLT31 before the containment control is invoked.

Interface references: IBM [CMS SUBCOM](https://www.ibm.com/docs/en/zvm/7.3?topic=functions-subcom),
[CMSCALL](https://www.ibm.com/docs/SSB27U_7.2.0/com.ibm.zvm.v720.dmsa6/cmscall.htm),
[USERSAVE](https://www.ibm.com/support/pages/zvm/pubs/cms730/usersave.html),
[IRXINIT parameters](https://www.ibm.com/docs/en/zos/2.5.0?topic=irxinit-parameters),
[IRXINIT return codes](https://www.ibm.com/docs/en/zos/2.5.0?topic=irxinit-return-codes),
[IRXSUBCM parameters](https://www.ibm.com/docs/en/zos/2.5.0?topic=irxsubcm-parameters),
[IRXHST parameters](https://www.ibm.com/docs/en/zos/2.5.0?topic=irxhst-parameters),
[IKJEFTSR parameters](https://www.ibm.com/docs/en/zos/3.1.0?topic=ikjeftsr-parameter-list),
[IKJEFTSR returns](https://www.ibm.com/docs/en/zos/2.5.0?topic=ikjeftsr-return-codes-from),
and the [MVS SVC 8/9 register interfaces](https://publibz.boulder.ibm.com/epubs/pdf/iea2v2a1.pdf).
Later manual editions establish interface facts; they do not establish behavior
on the older guests.
