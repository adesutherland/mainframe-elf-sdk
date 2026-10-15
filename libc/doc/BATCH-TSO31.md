# TSO31 direct batch runtime

The development batch entry uses the same maintained native linkage and MVS
length-prefixed PARM as the foreground entry. Service-table version 8 selects
DD-backed SYSIN, SYSPRINT and SYSTERM. It preserves the version-7 native TSO
command-call slot. Native terminal callbacks are refused; the batch entry
object contains no SVC 93. Foreground versions retain their terminal route.

Validate arguments before allocating standard streams. Require all three DDs,
close a partial startup, and keep stdout/stderr buffers and UTF-8 decoders
separate. Standard streams translate UTF-8 C bytes and IBM1047 native records,
independently of the named-file conversion switch. Preserve fixed input
padding and logical newlines; wrap standard output at the declared record
width. Named files retain the existing encoding/binary policy and supported
record formats. Failed output is reported, and partial/normal termination
balances native handles, dynamic DDs and heap ownership.

`libc/scripts/check-batch.crexx` builds a private, source-derived overlay over
a verified SDK and runs host sanitizer controls, entry instruction checks and
native probe packaging. `--tso-batch` requires TSO31 and maintained adapter
sources. Its manifest identifies the unchanged base and changed source/runtime
inputs. This is an unpublished candidate; the base release is preserved.
Use the normal `build/tso-batch` location for this development profile. The
recipe cleans its consumer-owned output in place and uses one candidate SDK
for both native probes. `--clean` refuses an unowned directory, a symlink or
an output containing any source/runtime/SDK input. It verifies the base SDK
before removing an earlier generated consumer build.
The cREXX consumer `ports/single-threaded/mainframe-batch-pack.crexx` pins all
four tools and matching libraries. No application source rewriting is used.

Mainframe Lab and z/PDOS own paired guest execution. Direct MVS entry qualifies
the DD/runtime path; ADDRESS TSO also requires a named native TSO environment,
with background IKJEFT01/SYSTSIN/SYSTSPRT as the reference where needed.
Qualification must identify the same native program bytes and source/data
records on both systems, separate OS failure from application RC, and check
output and resource handback. TSO24/64 batch, arbitrary QSAM and LE are outside
this initial TSO31 runtime. The selected shared package passed native z/OS 1.5
then PDOS line/model2 qualification on 14 October, including five byte-identical
durable outputs, native ADDRESS, compiler exits, failure codes and normal
resource handback. Other guests, profiles and released SDK packages remain
separate qualification and publication decisions.
