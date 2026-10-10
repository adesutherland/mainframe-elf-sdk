# Command client qualification — 10 October 2026

## Candidate and source review

The primary slice is the common C fixture and CMS31/TSO31 ELF/newlib command
adapters. At the qualification freeze, source was local and uncommitted on
develop, based on
`8801bfe1f4b554b21a435bdf8b049b20bb2eb49e` in mainframe-cross-sdk.
No commit, publication, SDK installation or managed PDOS replacement is part
of that run. Source commit and publication were subsequently authorized on
10 October; this development checkpoint is not a new SDK binary release.
The [interface guide](../COMMAND-ENVIRONMENTS.md) owns the contract.

The pre-guest review covered the complete adapter and build selection, native
parameter layout, high-bit list terminators, encoding/length rejection,
service versus command status, heap cleanup, module LOAD/DELETE ownership,
borrowed context lifetime, optional environment results, source packaging and
the caller's file/storage/console ownership. Review resolved FINDENVB's
successful parent-task RC4 handling and retained native SVCs in the versioned
TSO entry rather than allowing them into ELF objects. Native 32-bit pointer
and long widths are checked at compile time.

Freeze 4 is reviewed for the bounded CMS31/TSO31 real-system runs. Before CMS
execution, the operator extracted the actual CMS20 CMSCALL member from DMSGPI
S2. Independent coordinator review confirmed that SVC204 receives call type in
R15's high byte and the extended-list flag at X'00001000'. The helper was
corrected accordingly, rather than prefilling the callee's USERSAVE. PROGRAM
query uses X'0000A000'; SUBCOM with an extended list uses X'0200B000', including
COPY/FENCE. The 444 sequenced native reference records have SHA-256
`8bc467d88e067de6fb85764e580db793813372406493810ed9a333245af97254`.
The original IBM library is untouched; its macro implementation is private
qualification evidence, never a product or build input.

## Exact local inputs and host results

The verified base is the released 0.1.0 darwin-arm64 SDK at
`build/release-candidates/mainframe-elf-sdk-0.1.0-macos-arm64`.
The operator independently checked all 1652 manifest files. Its manifest is
`f77640686dbd0c298e4ea1c501193171f5799e3732960f70770e3567181f2d28`.
Tools are GCC 16.2.0, GNU binutils 2.47 and the source-built Classic Assembler/
Linker from that package; the libraries are its selected newlib
4.6.0.20260123 profile libraries. The native TSO dataset service is unchanged.

The initial reviewed output was `build/command-environment/freeze4`, produced by
`libc/scripts/check-command.crexx`. Both actual C adapters passed C89
Clang/ASan/UBSan controls with deterministic native stubs: missing names,
nonzero signed command RC, exact double spaces/quotes/punctuation, malformed
or unrepresentable UTF-8, embedded zero, excessive length, invalid result,
loader failure, closed-session rejection and parent-task/no-context outcomes.
These are host checks, not native execution.

Both complete profile images passed the encoded ISA/identity checker and
native export/link packaging. CMS MODULE write/verify passed. TSO source-entry
inspection verified AMODE31/RMODE ANY, existing file-service closure, LOAD,
DELETE and BASSM bytes; native-deck and XMIT member inspection passed.

| Frozen artifact | SHA-256 |
| --- | --- |
| inputs.sha256 | `3018d58f7e829f231a0f317d0fe507c37e7077824e90537426db17e792ed874d` |
| outputs.sha256 | `874f55c6343ba6e3d112fb1e170da5a21085f86f35f817c36a1030314f92c7b0` |
| CMS consumer.module | `cf473a4b99c999e6fbe59989297fdd05859862d8ba4916ff1e271f36c8a0ffcc` |
| TSO ENV31.XMI | `59fa6fb9539315759f87cd6acfbda054de8276a645fbe2a661cf6f1e20928888` |
| TSO command-entry.obj | `034ef89c06105b566e8b758bec9de0cb46d25c87583a7ee48287547b7231dd97` |

Input and output manifests were independently reread with shasum -c before
dispatch. Earlier failed development builds and freezes 1/2 are not selected guest
candidates. The retained recipe is cREXX; the existing Python ELF/native
consumer and object inspectors remain their necessary tool interfaces.

A subsequent package-path review repaired producer_c_decks staging: it now
copies the common command header/internal header/conversion source alongside
the selected TSO adapter. The affected producer mode built checked TSO24,
TSO31 and TSO64 C decks in `build/command-environment/producer-check3`.
That function is not used to build freeze 2; the repair changed no fixture,
adapter or native-entry bytes and no frozen binary. The earlier input manifest
retains its historical consumer-script hash; the current source manifest owns
the package-only repair and documentation. Native binary results can be reused
on that explicit dependency boundary.

Review also identified that two short records could be prefetched by stdio.
Freeze 3 requires successful unbuffered input selection before the first read,
so the post-command read reaches the native file/record path. That fixture
change passed the retained host/package recipe and both manifests were checked
again. The native TSO entry is byte-identical to freeze 2. Adapter/native source
is unchanged; the selected fixture binary and historical freeze-2 receipts
remain distinct. The CMS-only native correction then produced freeze 4;
focused disassembly verified the high-byte shift, conditional extended-list
flag and COPY/FENCE constants before its complete recipe passed. The TSO
XMIT and native entry are byte-identical between freezes 3 and 4 (cmp PASS),
so the freeze-3 TSO result applies to the same frozen binary. The native
Rexx RC harness is retained in
`tests/command/run-tso.rexx`, SHA-256
`6a7409b60785c9aca3bb246af19e6d51b3fec8e75863069db677eb207f58165c`.

## Native matrix and current limits

The operator owns z/VM 4.4 service 0302/CMS20 service302 on the existing
2064/512 MiB instance, leased LABA01 A191, and z/OS 1.5 ADCD on the existing
2064/256 MiB instance, leased LABA01 and fresh owned datasets. Native success
requires fixture RC0, every required check PASS, the real command output
between markers, a nonzero controlled command result, successful subsequent
commands, preserved input/guard storage and return to the owned terminal.
Optional environments are observations; absence of ISPF or XEDIT is valid.

CMS warm-start recovery reported invalid data; the operator first selected
STOP. Adrian separately
approved FORCE NOAUTOLOG, which completed spooling initialization and reported
111 available files. The leased LABA01 A191 was restored, and CMS20 service302
and original 64 MiB virtual storage were observed. TSO's first allocation on
OS39M1 failed for space; bounded fresh datasets on LABW01 succeeded without
deleting unrelated data. INPUT uses VB80/800 and two exact native text records;
fixed-record padding would not match this fixture.

The first and only TSO run from the native Rexx harness returned RC16 and
`ENV FIXTURE TSO31 FAIL failures=4`, then returned to READY. FINDENVB found a
parent environment (RC4), TSO/MVS queries succeeded, and ISPEXEC/ISREDIT/
ZZNOENV were absent. Every TIME and LISTDS invocation emitted IRX0812E,
reported IRXHST service RC20 and command RC-3, and failed its required success
check. The absent command met the same context error, so its nonzero result
does not establish absent-command dispatch. Unbuffered input before/after,
the 128 KiB caller guard, close/reopen and input-close passed. This is failed
native qualification, not a successful command client.

The failure is consistent with the parent Rexx processor remaining active
during ADDRESS TSO CALL; that cause is an inference from the native diagnostic,
not yet established by a successful control. IBM documents IRXHST as a compiler
runtime routine and requires an invoked but currently unprocessed exec for
IRXEXCOM variable access. A separate direct-from-READY CLIST control retains
the identical binary, saves the numeric &LASTCC immediately, exits on failure
and preserves the original failure as a regression case. Its reviewed source
is `tests/command/run-tso.clist`, SHA-256
`5b606ade812d03d2bc87178dbba25598ff1d233d87a360dd5c3c2628420d1b20`.
It initializes no Rexx processor, registers no environment and alters no Rexx
control blocks.

That single CLIST control also failed: its first TIME call abended S0C4,
reason4, before the remaining input/guard/close checks were reached. LASTCC
was the symbolic value S0C4, so the original harness's EXIT expression caused
a recursive CLIST error. It returned to READY but supplied no numeric C
return. Its capture `zos15/freeze3-clist-control.screen` has SHA-256
`46c85cfcf8cb30ef5d9a190f2fb6eb0896ef5fac56027e76a325d4f66a71d611`.
Changing the caller did not qualify IRXHST; neither failed run is retried.

CMS freeze 4 ran once on CMS20 with 256 MiB virtual storage, after exact
host-to-guest-to-host MODULE and variable-record readback. All 18 required
checks passed, with `ENV FIXTURE CMS31 PASS failures=0` and native `Ready;`
(RC0). CMS was present; COMMAND, XEDIT and ZZNOENV were absent in this session.
QUERY TIME produced the native clock/CPU output and returned 0; the command
with preserved double spaces, STATE  ENVINPUT DATA A, returned 0. ZZNOEXST
returned native -3, followed by a successful QUERY TIME. Unbuffered input
before/after, the 128 KiB guard, closed-handle rejection, reopen and the final
command passed. The unknown command emitted no diagnostic text in this
capture; its exact native return is the qualified failure result. The complete
capture `zvm44-recovery/freeze4-execution.screen` has SHA-256
`e702b8218a58a481a471a7df75dabc762f31bc10b2e815c10df5f74a91ba158f`.
Coordinator reread the full capture and independently matched the readback
MODULE to the frozen hash. Restoring the original 64 MiB virtual storage and
operator handback remain separate checks.

## Revised generic C TSO path

The selected final candidate is `build/command-environment/freeze5`. Generic
C TSO invocation now uses documented IKJEFTSR with six parameters, a direct
native command-buffer address and X'00010001' flags. Facility RC0/4 carries a
valid signed command return. Other facility failures preserve reason and
abend codes without a success-shaped command result; LOAD/DELETE status is
also exposed. Generic C invocation of other Rexx environments returns
MF_COMMAND_UNSUPPORTED before a native call. FINDENVB/IRXSUBCM still query
actual existing environments without creating or terminating a processor.
IRXHST integration belongs to a later compiled Rexx runtime contract.

The native bridge's local save area is now chained to ENTRYSA and restores
the old forward link on both successful and failed LOAD paths. C's R13 is
not a native save area. Coordinator reconstructed the native TXT image and
checked the actual setup/restore instructions and relocated ENTRYSA constant;
GNU disassembly confirms BASSM at offset X'322', LOAD/DELETE and the chain.
This is an independent linkage repair; the earlier failures do not isolate
its contribution from IRXHST's runtime-context requirements.

The revised C89 sanitizer controls passed normal and signed nonzero command
returns, parameter failure with reason code, contained command abend, module
failure and unsupported named-environment rejection. The full retained host
recipe passed both images, encoded ISA/closure, MODULE/XMIT and native-entry
checks. Input and output manifests were independently reread with shasum -c.
The affected producer C-deck staging path also rebuilt TSO24/31/64 successfully
in `build/command-environment/producer-check5`; this is package-path evidence,
not new command-client support for TSO24/64.
The CMS service helper is unchanged; the common result structure and fixture
detail output changed, so final-source CMS execution is repeated once.

| Freeze 5 artifact | SHA-256 |
| --- | --- |
| inputs.sha256 | `1ef9bf71620387b640f34eb9aed5d0e3b1c545329452407122da6e1b732c2b56` |
| outputs.sha256 | `5cbcc7cfef9266cb802778a7f84028c6befaff9a8be817b5f69a13f6bbe457a4` |
| CMS consumer.module | `e9e37d92540a304abc21e2d2ad80e2a794606f30b7776d39515c0810a59237fb` |
| TSO ENV31.XMI | `bfa29910968fdfc48b8e7f3e8edf9fbd3ce67e2544e8f73b14ea2c3dacb7e007` |
| TSO command-entry.obj | `d5d6638dea7783bf4b83339b710ba92a0080395ab796a623aee0acb23bf749e4` |

The hardened CLIST source has SHA-256
`7eb7f13a5120450f34ce55058454171e633223bbf5b13972367763ae819dd23a`.
It prints the exact LASTCC and exits with numeric 16 from its error routine,
including symbolic system/user abends. Native successful-call RC is still
captured immediately and must be zero. TSO's controlled failing command is
LISTDS on the checked absent task dataset LABA01.ENVCMD.ABSENT; successful
LISTDS on INPUT and later TIME must pass. The operator confirmed IKJEFTSR
exists as a non-alias in actual SYS1.LPALIB; its LINKLIB lookup was absent.

TSO freeze 5 passed its first and only direct CLIST run: all 19 checks PASS,
`ENV FIXTURE TSO31 PASS failures=0`, numeric native RC0, caller-survival and
harness-PASS markers, followed by READY. TIME produced real output three
times. LISTDS with full quoted INPUT operands and double spaces returned
facility0/command0 and reported VB80/800 PS on LABW01. LISTDS on ABSENT
returned facility4/command8 with the native not-in-catalog diagnostic, then
TIME succeeded. The unbuffered second input read, 128 KiB guard, logical
close/reopen, closed-query rejection and input-close passed. Module status
was zero throughout. Reason -1 and abend X'FFFFFFFF' are unused native output
sentinels on these returns, not a command abend. Coordinator independently
reread the full capture and hashed the complete native XMIT readback.
`zos15/freeze5-execution.screen` has SHA-256
`3cb3d6622a286648ce103a5ecec14441f202a06c124c50435ea5c4aced3cd5b1`.
An active Rexx caller of this repaired binary, compiled Rexx and ISPF contexts
remain unqualified.

CMS freeze 5 passed its single final-source run after a fresh native reset
and exact MODULE readback. All 18 checks passed with native RC0/Ready;,
including the actual QUERY TIME output, STATE0, ZZNOEXST-3, later successful
TIME, unbuffered file continuation, 128 KiB guard and close/reopen. Its new
detail fields were zero as defined by the CMS adapter. Coordinator reread
the complete capture and independently matched the native readback to the
selected MODULE hash. `zvm44-recovery/freeze5-execution.screen` has SHA-256
`7836c2e74c26b4d4fdb6a619c6680851d5fab082bfe369d7dd7bc3d4c81b368f`.
Both final primary ELF/newlib matrix cells are therefore natively qualified
for this bounded query/invoke fixture.

The earlier attempted new harness member in the existing LABA01.SOURCE library
failed B14-10 on full OS39M1. Its post-failure directory audit did not list
ENV31Q3; the complete pre-transfer directory was not captured. The original
library and unrelated members were kept, with no deletion based on inferred
prior absence. All final harnesses, binary transport/load and input use fresh
LABA01.ENVCMD datasets on LABW01; the B14 diagnostic remains private evidence.

## Operator handback and coordinator closure

Handback passed. CMS's original 64 MiB virtual storage and physical 0121
CP SYSTEM LABA01 mapping were restored. Both virtual tapes were detached and
the real tape devices reset empty. The owned accounts logged off; CMS reached
shutdown wait 0961, and z/OS completed JES2 termination, HALT EOD and QUIESCE
wait 0CCC before both emulators exited. Final replay files and private failed
controls remain available; the temporary native macro copy was erased after
its interface facts were retained. The original macro library is unchanged.

The operator's `final-handback.json` records both receipts. Coordinator then
independently fetched live fleet status: all six guests stopped, no recorded
processes and no leases. Direct socket checks found all six guest listener
ports and the two fixture script ports closed. All ten named emulator/terminal
processes were gone, and lsof found no handles under either operated instance.
Final native MODULE/XMIT readback hashes were checked again. The independent
receipt is `coordinator-final-validation.json` beside the private operator
report. The maintained source inventory verifies 202 files and the working
diff passes its whitespace check. No commit, push, SDK installation or PDOS
replacement occurred. This closes the primary ELF/newlib native slice;
the remaining matrix and runtime work below stays open.

Private operator receipts are under
`/Users/adrian/MainframeLab/private/command-environment-20261010`.
The failed TSO capture `zos15/freeze3-execution.screen` has SHA-256
`3c9ecef112f49cddac6501f0405ba34adf1d7bf6b9be1fbace637adc8a6ea48c`.
Native transfer readback matched the full XMIT hash above and the two input
records (19 EBCDIC bytes, SHA-256
`74060e5de95af4362ed5434744faf87eba34ba524b71eb99443792bf19983d46`).

Context references: IBM [IRXEXCOM](https://www.ibm.com/docs/en/zos/2.5.0?topic=services-variable-access-routine-irxexcom),
[compiler runtime routines](https://www.ibm.com/docs/en/zos/3.1.0?topic=customization-support-rexx-compiler),
[LASTCC](https://www.ibm.com/docs/en/zos/2.5.0?topic=codes-lastcc),
[ERROR](https://www.ibm.com/docs/en/zos/3.1.0?topic=reference-error-statement)
and [error-routine exits](https://www.ibm.com/docs/en/zos/3.1.0?topic=routines-subprocedures-error).

Classic C/PDPCLIB routes require their own native ABI/runtime qualification;
independent operator/source review found neither a maintained whole-library
CMS31 recipe nor a qualified Classic TSO31 whole-runtime/caller ABI route.
PDPCLIB's PCL-002 owns those prerequisites; its TSO SDK assembler service is
not a whole Classic C client.
The inherited CMS system() success placeholder is not such evidence. Native
registration, callbacks, complete enumeration, command output capture,
TSO24/64 and cREXX package integration remain open. The subsequent PDOS
qualification below uses the reviewed exact native binaries.

## PDOS 0.2.2 bounded command-environment qualification

The original freeze-5 CMS31 ENVFIX MODULE and TSO31 ENV31 XMIT run unchanged
on the reviewed PDOS F4 candidate. CMS retains `ENVINPUT DATA A`; TSO uses
its existing direct dataset allocation with `LABA01.ENVCMD.INPUT`, rather
than the real-z/OS run's preallocated DD argument. No native CMS/z/OS run
was repeated for this campaign.

The optional `mf_pdos_command_execute` client and its separate CMS/TSO
fixtures were built with the existing 0.1.0 SDK and selected newlib adapters.
The installed SDK and upstream newlib core are unchanged. The optional
function validates/encodes before querying `PDOS`, reports absence explicitly,
and invokes SUBCOM PDOS or the separate PDOSCMD facility. It does not change
the selected native `CMS` and `TSO` meanings. Retained recipe:
`libc/scripts/check-pdos-command.crexx`; fixtures:
`tests/command/pdos.c`, `child.c` and `fault.c`.

Model5 colour, model2 colour and line primary each pass 18 original CMS
checks, 19 original TSO checks and 13 checks for each optional client: 189
application checks in total. The real TSO31 child prints its output once
per caller and returns RC7. A volatile write to unmapped page 0x2000 supplies
the contained hardware-fault control. CMS returns native RC-4; TSO reports
facility12 with invalid command RC. Subsequent command success, unbuffered
input continuation, 128 KiB caller guard and close all pass.

The complete stopped transcripts/captures, actual geometry, clean shutdown,
zero invocation/console-owner state and STORE/non-STORE readbacks were
independently reviewed. The [PDOS qualification record](../../../../z-pdos/pdos/doc/qualification/COMMAND-ENVIRONMENTS-2026-10-10.md)
owns F4's exact K/U/package hashes, resolved development failures and
per-profile receipts. It also records the 106-check foundation diagnostic
run and event-based capture repair. This is a local development checkpoint,
not a new SDK release or managed PDOS adoption.

The PDOS context is opaque, with read-only known-name queries and a bounded
CMS/TSO command subset. It is not a traversable IBM ENVBLOCK or a Rexx
processor. Application registration/callbacks, Classic whole-library clients,
additional client modes and actual cREXX ADDRESS integration remain later
work. The maintained source inventory now verifies 206 files.
