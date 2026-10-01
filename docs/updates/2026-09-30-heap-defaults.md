# Application heap defaults — 30 September 2026

I use generous ordinary application heaps on 31-bit and 64-bit profiles.
Small heaps remain useful for explicit exhaustion probes. The CMS compiler
qualification exposed a 24 MiB exhaustion path; heap allocation guards and
accurate diagnostics are separate repairs in cREXX.

| Profile | Source default | Selection |
| --- | --- | --- |
| CMS24 | 64 KiB, unchanged | Historical profile |
| CMS31 | 64 MiB | `__MAINFRAME_LAB_CMS20_ESA31__` |
| TSO24 | 4 MiB, matching existing builders | `LAB_TSO24` |
| TSO31 | 64 MiB | ILP32 ordinary TSO |
| TSO64 | 128 MiB | `__LP64__` |

Explicit `LAB_CMS_HEAP_SIZE` or `LAB_TSO_HEAP_SIZE` values still take
precedence. CMS obtains the 31-bit heap through its native storage service;
TSO obtains its heap through the retained entry service. Historical CMS keeps
static storage. Allocation, exhaustion and cleanup behavior are unchanged.

## Focused check

`tools/check_heap_defaults.crexx` accepts absolute SDK root, newlib source
include directory, generated newlib include directory, output directory and
optional compiler path (default `/usr/bin/clang`). Run it with cREXX and its
matching library. The retained C fixture preprocesses the actual syscall
translation units. It checks all five defaults, three explicit overrides and
rejection of unaligned CMS31 and TSO31 overrides. Success is
`SUMMARY: PASS=10 FAIL=0` with RC 0; a mismatch returns RC 1. Per-case compiler
diagnostics are written only to the supplied output directory.

The macOS check passed all 10 cases with RC 0. A deliberate failing compiler
command returned runner RC 1. With the explicit 64 MiB CMS override, the old
and new syscall sources preprocess to identical bytes; the old source copy
was verified against its recorded SHA-256 before comparison. The host
preprocessor can warn about combined Apple/newlib headers; this check does not
compile or execute those headers as a host runtime.

This check verifies macro selection and existing configuration guards. It
does not claim a native runtime build or guest execution. Frozen archives and
the earlier passing TSO packages remain unchanged. Future TSO variants need a
new freeze and affected guest qualification; the current CMS repair uses an
explicit 64 MiB override and its own guest receipts.
