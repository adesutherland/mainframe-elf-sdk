# SDK C library backlog

## LIBC-001: Source-built profile runtimes

- Type: qualification
- Status: In progress
- Observation: The package assembler still requires three pinned runtime
  archives rather than building the selected libraries and adapters itself.
- Target: CMS24, CMS31, TSO24, TSO31 and TSO64 sysroots.
- Evidence: On 3 October 2026 all five selected newlib core, extra, math, wide integer and soft float archives passed source builds and object checks. Both CMS adapter/startup sets passed, five C sysroots were assembled, and source-built consumers linked/exported CMS24/31 MODULEs and TSO24/31/64 ELF decks. Native TSO objects, installed package and guest runs remain open.
- Acceptance: Build each advertised sysroot from maintained SDK source and the
  checked newlib archive with the freshly built compiler and assembler;
  validate startup, headers, archives, text behavior and affected guest runs.

## LIBC-002: Direct-source newlib preparation

- Type: qualification
- Status: Done
- Observation: Preparation previously applied a configuration patch.
- Target: newlib 4.6.0.20260123 selected source.
- Evidence: The 3 October 2026 direct-source preparation passed and the entire generated newlib source tree matched the previous patch-prepared tree byte for byte with `diff -qr`.
- Acceptance: A fresh preparation copies only checked files from `src/newlib/`
  and reproduces the prior prepared newlib source byte for byte.

## LIBC-003: Source-built TSO native bridge closure

- Type: qualification
- Status: In progress
- Target: TSO24, TSO31 and TSO64 entry and service objects.
- Observation: TSO24 and TSO31 entry objects assemble from maintained source with z/PDOS Classic Assembler after selected service calls are expressed through published register interfaces. Complete PDPCLIB `pdos-zarch` source now assembles with source-owned PDOS31 macros, but the MVS `tso31-lean` source stops at `PUTLINE MF=L`; it still needs TSO service definitions and guest qualification. The TSO64 low bridge stops at `GETMAIN`, and its AMODE64/RMODE ANY entry stops at the mode directive.
- Evidence: On 3 October 2026 `libc/scripts/build-tso-entries.crexx` produced `build/tso-entries-source/tso24-entry.obj` and `tso31-any-entry.obj` with source-built Classic Assembler; both passed `sdk/scripts/tso_entry_source_check.py`, including mode, external closure and independent SVC/EPSW byte checks. A changed SVC opcode was rejected. The TSO31 entry is AMODE31/RMODE ANY, with six unresolved external identities by design. The [native PDPCLIB and TSO64 review](../../docs/updates/2026-10-03-funhead-pdpclib-tso64.md) records the later source assembly, exact hashes and two TSO64 stops.
- Acceptance: Build all native entry and service objects with cross-platform tools, link each complete profile, and repeat affected guest service checks. Source-built entry cards alone do not meet this criterion.
