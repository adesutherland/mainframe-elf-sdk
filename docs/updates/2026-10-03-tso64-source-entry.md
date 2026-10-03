# TSO64 source-built entry checkpoint — 3 October 2026

We now assemble both generic TSO64 entry variants on Apple Silicon macOS
(`Darwin arm64`) using the maintained z/PDOS Classic Assembler. This is a
source and host object result, not a z/OS guest result or a 0.1.0 release.
The SDK and z/PDOS checkouts were on `develop`; these changes were not yet
committed when this report was written.

## Source and tool path

Mainframe Lab retains the older z/OS 1.5 TSO64 experiment and its application
low launchers. Its 64-bit entry is the reference for the generic SDK source
in `libc/src/adapters/tso64/`. The maintained entries and independently
authored selected service macros now live in that SDK `src/` tree. No IBM
assembler, IBM macro library, private Lab object or prebuilt native deck is
an input to this entry build.

The z/PDOS assembler gained AMODE64 section metadata, selected z900
instructions, signed long displacement and signed immediate encoding, and
absolute eight-byte AD literals. It still rejects eight-byte relocations.
The selected TSO64 macros implement the entry's SVC 120 storage requests,
SVC 93 terminal calls, SVC 99 dynamic allocation and version-zero IARV64
GETSTOR/DETACH requests. The IARV64 list layout and PC linkage were checked
against the older private z/OS 1.5 macro reference; that IBM source is not
copied into the repository. The linkage sets R15 to EX=14 before `PC`, as in
that reference. The public [SVC 120](https://www.ibm.com/docs/en/zos/3.1.0?topic=descriptions-svc-120-0a78),
[TGET/TPUT](https://www.ibm.com/docs/en/zos/2.5.0?topic=tpg-register-form-tget-tput)
and [IARV64](https://www.ibm.com/docs/en/zos/2.5.0?topic=allocation-requestgetstor-option-iarv64)
documentation supplies the service descriptions.

From the SDK root, the retained cREXX recipe was run with the source-built
assembler at SHA-256
`ca3860b421fc17c16ce20a9fd92d9eaa2e8dec8b16c621b301829263ca54f02f`:

```sh
crexx -nokeep libc/scripts/build-tso-entries.crexx --args \
  /Users/adrian/CLionProjects/z-pdos/build/tools/assembler/mf-classic-as \
  build/tso-entries-source-20261003-v64c
```

The recipe assembled TSO24 (277 statements), TSO31 (295), TSO64 low entry
(333) and TSO64 AMODE64/RMODE ANY entry (368). All four passed
`sdk/scripts/tso_entry_source_check.py` for section mode, six intended
external identities and independently expected service instruction bytes.
For TSO64 the checks include two IARV64 PC instructions, their preceding
R15/EX linkage, six SAM64 instructions, SVC 120/93/99 and the AMODE64 entry
PSW probe. A changed SVC opcode was rejected in the earlier entry checker
control. The TSO64 object SHA-256 values are:

| Entry | SHA-256 |
| --- | --- |
| AMODE31/RMODE ANY low entry | `c15e74f6639eb7593e30f979490acb4987f1afa2153705ebffc7dc21a3146c74` |
| AMODE64/RMODE ANY accepted-route entry | `5a12f242e19b1f9a89ee0203e7f3e3d8f9f8a27253c4658954dd46e8114a603f` |

The unchanged TSO24 and TSO31 entry object hashes are respectively
`9ca288ce4ef17290c3c99f137663e209fa9e3225ba67c8c9747cab9080482a93`
and `14866e0635cfa7e1b1075098037b4027751c30a22e14e8bd16a18c9d61e8c8c9`.
The z/PDOS host suite passed 93/93 with `ctest --test-dir build/tools
--output-on-failure -j 4`. A preceding `-j 8` run had one transient
`pdpclib_profile_sources` cREXX module-read failure; that test passed alone
and in the subsequent full `-j 4` run.

## Host linker shape

An isolated Classic Linker XMIT run paired the AMODE64 entry with a deliberately
nonfunctional six-export fixture. `mf-classic-ld --oformat xmit --amode 64
--rmode 31 -e LABTS64` resolved `ELFPOC` and the five `@@A*`/`@@DYNAL`
exports, wrote a 3,840-byte FB80 XMIT, and placed `LABTS64` at offset zero.
The output SHA-256 was
`37f8e2c804cd209532b958b8228a581db0762b062f6e83521f8283269dd561b9`.
This proves linker acceptance of this entry shape only: the fixture routines
return immediately and cannot provide file or C behavior.

## Remaining release gates

The `tso31-lean` PDPCLIB service selection still stops at `PUTLINE MF=L`.
Its TSO list/execute forms and control-block mappings need source-owned,
edition-matched implementations, followed by full native links and guest
execution. The current package recipe still consumes pinned runtime archives
and ASMA90-produced native decks. Source-built C sysroots and entries have
not been integrated into a source-only installed SDK package. Linux host,
installed consumer and affected guest checks remain open for 0.1.0; native
Windows qualification remains the separate 0.1.1 stage.
