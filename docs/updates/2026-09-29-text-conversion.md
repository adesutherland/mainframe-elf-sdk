# Runtime text conversion switch — 29 September 2026

The standalone `mainframe-cross-sdk` checkout is the maintained source home
for the CMS24/31 and TSO24/31/64 newlib adapters. The lab adapter files were
byte-identical before this change; its dirty checkout contains separate SDK
and application work. I changed only this SDK source and retained the lab
baseline. Source revision before the local edit was `c5d50f0`.

`<mainframe_text.h>` declares `mainframe_set_text_conversion(int enabled)`.
Each linked runtime keeps one private, default-enabled flag. Zero bypasses
existing IBM1047/UTF-8 or ASCII/EBCDIC payload conversion; nonzero restores
it. The adapters still split and join text records at LF, preserve fixed-record
padding, and leave binary data and `printf` formatting alone. Call the setter
before text I/O; the runtime is single-threaded. CMS `fopen` remains opaque
byte I/O, while its explicit text adapter and console use the switch. TSO
text `fopen`, terminal input and output use it. The native TSO PUTLINE callback
retains its conversion by default and skips the `TR` instruction for the
runtime's raw request. Its version is now 6 for ILP32 and `0x6403` for LP64.
`fflush` behavior was not changed.

## Local build and checks

The unchanged, hash-verified local SDK v22 supplied GCC 16.2.0, binutils
2.47, the profile checkers, startup objects and profile-matched newlib core
archives. The retained SDK archive SHA-256 is
`bfcee2c5e570cff7f88afa47d96b115b0970661eff9ec58a8daa5aa24c153456`.
The ignored `build/text-switch/build.py` records the focused Mac build. It
recompiled three CMS C adapter members per profile and three TSO C adapter
members per profile, then reused the other objects and all generic newlib,
libm and compiler helper archives. It did not build the compiler, generic
newlib core, PDOS or cREXX. All five updated runtime archives passed their
profile object checker; five small switch programs linked and passed ELF plus
MODULE/native-deck format checks. The archive/deck hashes are in ignored
`build/text-switch/results.json`.

Small host service mocks executed the real CMS text and console adapter code,
plus the real TSO adapter in ILP32-like and LP64-like builds. They checked
default `A` conversion to native `0xc1`, a raw `0xc1` read after disabling,
raw `0xc2` output, re-enabled `B` decoding, terminal bytes and LF handling.
These mocks are local checks, not guest qualification.

After review, I placed the conversion decision inside the existing CMS and
TSO text loops. Both settings use the same LF and record handling. I rebuilt
the changed adapter objects and all five small programs from that final
source, reran the host mocks, and refreshed the source manifest. Their final
hashes are in ignored `build/text-switch/results.json`.

On the shared z/OS 1.5 guest, I used a leased LABA02 account and new
`LABA02.TC*` datasets. ASMA90 assembled changed TSO24, TSO31 RMODE ANY and
TSO64 RMODE ANY native entries with RC0 (JOB00327); IEWL bound all three
with RC0. The three new C programs ran to RC42, printed the raw native byte
as `C`, and reported `TEXT SWITCH ... PASS`. After the final source edit,
I sent the three new object decks to the guest, read them back with matching
SHA-256, and rebound them with IEWL RC0 in JOB00330. The final captured runs
are ignored `build/text-switch/run24-final.log`, `run31-final.log` and
`run64-final.log`, each returning RC42 and PASS.
The first TSO24 run hit the previously documented z/OS 1.5 PDPCLIB `@@AOPEN`
S0C4 defect. I rebound only TSO24 in JOB00328 with the retained patched
`CREXX.N24.OBJECT(PDPSUP)` service object, SHA-256
`e9039b5251ff16c2a3325e8d25bf9f23d103bab02657689e886988b32b548c7c`.
Its source read-back matched the documented patched source SHA-256
`50cad35c7e5fa4c88a2efcf4f8f3999948087edad828bc4d5b032b5f0a3bd3b1`.
The canonical SDK now carries the byte-identical repair as
`patches/pdpclib/0004-zos15-tso24-swareq.patch`, with a checked recipe in
`patches/pdpclib/README.md`. The lab's original patch remains a historical
receipt. The checked SDK v22 baseline `T24SUP` is unpatched; this test used
the repaired guest object explicitly.
The LABA02 session was logged off and its account lease released.

CMS24 and CMS31 MODULEs linked and passed host format checks. Their small
switch programs were not guest-run: the VM/370 CE and z/VM 4.4 input tape
devices were leased by other work. The host mocks show the changed conversion
logic, but do not promote either CMS profile to guest execution. The TSO
results likewise qualify this small C program on z/OS 1.5, not a cREXX build
or release package. Existing guest results for prior exact bytes remain
separate.
