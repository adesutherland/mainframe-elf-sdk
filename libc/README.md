# SDK C library

This component owns the maintained newlib integration for CMS and TSO SDK
profiles. The checked upstream newlib archive supplies unmodified files;
`src/newlib/` contains the two currently maintained source files. Edit that
tree directly. The old recovery patch is frozen in `archive/` and is absent
from the normal preparation path.

The older bootstrap package consumes separately built runtime archives. A
source-input local candidate now packages five source-built sysroots. The
remaining qualification gates are in
[the release plan](../docs/RELEASE-PLAN.md) and [backlog](doc/BACKLOG.md).
The TSO24, TSO31 and both TSO64 entry sources assemble with z/PDOS Classic
Assembler and pass host object checks. A selected source-built TSO PDPCLIB
service and complete host native links pass; guest checks remain open.
