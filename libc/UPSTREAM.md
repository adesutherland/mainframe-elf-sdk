# SDK C library upstream

The selected library base is newlib `4.6.0.20260123`, SHA-256
`6ff27e3bf022666f43f7802255be680eeff722ac181b1725d21e2e8318604ee3`.
The official archive supplies unmodified source. Maintained configuration and
header changes live in `src/newlib/`; `upstream-files.sha256` checks their
pristine inputs, and `source.sha256` checks the editable files before and
after they are copied into an ignored generated source tree.

The two files were reconciled from the historical CMS ELF proof patch, now
frozen in `archive/`. Normal builds do not apply that patch. The changes
allow generic-C s390 configuration with a non-Linux service boundary, and
identify GCC's big-endian IEEE soft-float representation in a header. They do
not establish full newlib, libm or floating-point qualification.

`LICENSE` is the unmodified upstream `COPYING.NEWLIB` notice collection.
Individual upstream files retain their own notices, including the imported
vfprintf implementation's Berkeley attribution. Reconcile later newlib work
directly into `src/newlib/` with its original notices intact.

The CMS and TSO adapters in `src/adapters/` are project-maintained rather
than newlib source. The selected explicit TSO storage, terminal and dynamic
allocation service calls in `entry24.asm` and `entry31-any.asm` follow the
independently authored z/PDOS `tso31-bridge/src/entry31.asm` and its public
service-interface record. The SDK entries retain their own ABI, addressing
checks and text modes; neither the source-built object checks nor the public
interface facts establish guest behavior.

The two maintained TSO64 entries descend from Mainframe Lab's z/OS 1.5
TSO64 experiment. The low launchers for its separate application high-code
route remain in that laboratory. The SDK owns the generic entry sources and
selected MIT-licensed service macros in `src/adapters/tso64/services/`.
Those macros express only the GETMAIN/FREEMAIN, TPUT/TGET, DYNALLOC and
IARV64 operations used by these entries. The IARV64 version-zero layout and
PC linkage were checked against the older IBM macro edition available in the
private laboratory; IBM macro source is neither copied nor a build input.
The public [SVC 120](https://www.ibm.com/docs/en/zos/3.1.0?topic=descriptions-svc-120-0a78),
[TSO TGET/TPUT](https://www.ibm.com/docs/en/zos/2.5.0?topic=tpg-register-form-tget-tput)
and [IARV64](https://www.ibm.com/docs/en/zos/2.5.0?topic=allocation-requestgetstor-option-iarv64)
references describe the selected interfaces. Source assembly and host object
inspection do not establish z/OS 1.5 service behavior.
