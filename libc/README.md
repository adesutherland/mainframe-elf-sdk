# SDK C library

This component owns the maintained newlib integration for CMS and TSO SDK
profiles. The checked upstream newlib archive supplies unmodified files;
`src/newlib/` contains the two currently maintained source files. Edit that
tree directly. The old recovery patch is frozen in `archive/` and is absent
from the normal preparation path.

The SDK package currently consumes separately built runtime archives. The
source-built runtime and installed sysroot gates are in
[the release plan](../docs/RELEASE-PLAN.md) and [backlog](doc/BACKLOG.md).
The TSO24 and TSO31 entry sources assemble with z/PDOS Classic Assembler and
pass host object checks; service objects and guest checks remain open.
