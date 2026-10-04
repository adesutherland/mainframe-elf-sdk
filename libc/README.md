# C runtime and native adapters

The runtime makes compiled C useful on CMS and TSO: it supplies selected C
library routines and connects their requests to the target operating system.
It has three parts, with different origins and responsibilities.

| Part | Responsibility | Source |
| --- | --- | --- |
| newlib and GCC helpers | Selected C, math, wide-integer and software floating-point routines | Checked upstream archives, with maintained newlib configuration/header changes in `src/newlib/` |
| SDK adapters | Program entry/exit, arguments, storage, console and file/record handling | `src/adapters/cms/`, `src/adapters/tso/` and `src/adapters/tso64/` |
| Native TSO dataset services | The selected MVSSUPA routines called beneath the C adapter | Maintained PDPCLIB in the separate z/PDOS repository |

Newlib is the SDK's C library. We reuse PDPCLIB's native dataset layer rather
than linking its full C library. On TSO64, the SDK marshals 64-bit C arguments
into low storage for the narrower native service interface. CMS has separate
SVC 202/204 adapters and does not use this TSO layer.

`scripts/` builds five distinct application sysroots, containing headers and
selected libraries for CMS24, CMS31, TSO24, TSO31 and TSO64. The CMS sysroots
also contain startup and adapter objects. The installed TSO consumer compiles
its C adapters and links the source-built native entries and service object
packaged with the SDK. The freestanding `vmkernel` profile uses no application
runtime.

The 0.1.0 source build and installed consumers passed on macOS arm64 and Linux
x64. Guest checks establish CMS entry/return and the named TSO entry/file
subset; they do not establish every routine in the library. TSO24 dataset
I/O is specifically excluded. The [release limits](../docs/RELEASE-PLAN.md#known-limits)
state the remaining service and guest restrictions.

See [architecture and profiles](../docs/compiler/SDK.md#runtime-choices-and-limits)
for heap defaults and the text-conversion switch, and the
[source build guide](../sdk/README.md) for recipe order. The two maintained
newlib files are edited directly in `src/newlib/`; the old recovery patch in
`archive/` is historical and not used by normal builds.

[Upstream and adapter provenance](UPSTREAM.md) · [Component notices](LICENSE)
· [Runtime backlog](doc/BACKLOG.md)
