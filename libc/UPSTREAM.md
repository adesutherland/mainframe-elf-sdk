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
