# SDK C library backlog

## LIBC-001: Source-built profile runtimes

- Type: qualification
- Status: In progress
- Observation: The package assembler still requires three pinned runtime
  archives rather than building the selected libraries and adapters itself.
- Target: CMS24, CMS31, TSO24, TSO31 and TSO64 sysroots.
- Evidence: On 3 October 2026 all five selected newlib core, extra, math, wide integer and soft float archives passed source builds and object checks. Both CMS adapter/startup sets passed, five C sysroots were assembled, and source-built consumers linked/exported CMS24/31 MODULEs and TSO24/31/64 ELF decks. macOS arm64 and Linux x64 installed consumers passed. CMS24/31 entry calls returned 42 on VM/370 CE and z/VM 4.4; selected TSO entry and file paths passed on z/OS 1.5. Wider runtime behavior remains separate work.
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
- Observation: TSO24, TSO31 and both TSO64 entry objects assemble from maintained source with z/PDOS Classic Assembler. The selected `tso31-sdk-files` PDPCLIB service assembles and closes all four host XMIT links with source-built C decks. Sequential write/read and PDS read passed on z/OS 1.5 for TSO31 and both TSO64 entry modes. VSAM/IDCAMS rejection and other service paths remain unqualified by guest execution.
- Evidence: On 3 October 2026 `libc/scripts/build-tso-entries.crexx` built all four entries. `sdk/scripts/build-tso-native.crexx` built and checked the PDPCLIB service, verified TSO31/64 deck closure and linked four source-input XMIT transports. The AMODE64/RMODE ANY TSO64 entry SHA-256 is `5a12f242e19b1f9a89ee0203e7f3e3d8f9f8a27253c4658954dd46e8114a603f`. The [source candidate checkpoint](../../docs/updates/2026-10-03-sdk-source-candidate.md) records the complete host and installed package result.
- Acceptance: Build all native entry and service objects with cross-platform tools, link each complete profile, and repeat affected guest service checks. Source-built entry cards alone do not meet this criterion.
