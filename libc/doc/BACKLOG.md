# SDK C library backlog

## LIBC-006: Native command-environment clients

- Type: implementation and qualification
- Status: In progress
- Target: CMS31 and TSO31 ELF/newlib clients first; other profiles separately.
- Direction: Query and invoke existing native environments before application
  registration/callback work. Preserve native context lifetimes, original
  command text and actual results through explicit binary adapters.
- Implementation: Borrowed sessions, SUBCOM/CMSCALL and IRXINIT/IRXSUBCM/
  IKJEFTSR adapters, TSO version-7 native service slot, common C fixture and
  cREXX host/build recipe. Upstream newlib is unchanged.
- Evidence: [Interface](COMMAND-ENVIRONMENTS.md) and [10 October record](qualification/COMMAND-ENVIRONMENTS-2026-10-10.md).
  Host C89 sanitizer and native packaging checks pass. CMS20 executed the
  exact CMS31 binary with all required checks and native RC0. The first TSO
  active-Rexx IRXHST caller failed with IRX0812E; its CLIST control abended.
  Generic C TSO invocation was replaced by the documented IKJEFTSR facility,
  with explicit unsupported results for other Rexx environments. The repaired
  TSO31 binary passed all 19 checks and native RC0 through a direct CLIST
  caller on z/OS 1.5. Final-source CMS5 also passed all 18 checks/native RC0.
  Operator handback passed, independently checked against processes, ports,
  disk handles and live fleet leases. Classic and PDOS matrix cells stay open.
  CMS recovery used approved
  FORCE NOAUTOLOG.
- Acceptance: Named CMS31/TSO31 success/error execution on real systems,
  preserved open input and caller storage, native output and return status,
  exact input/binary freeze and operator handback. Qualify Classic/PDPCLIB
  separately and run the frozen binaries unchanged on PDOS only after its
  native services are implemented. No complete-family or cREXX runtime claim.

## LIBC-001: Source-built profile runtimes

- Type: qualification
- Status: Released source-built subset; wider runtime qualification open
- Observation: The 0.1.0 source-package route uses rebuilt libraries and
  adapters. Only the retained historical bootstrap packager requires pinned
  runtime archives. The selected guest paths passed; broader text, math and
  service coverage remains separate work.
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
- Status: Released source-built subset; remaining guest service paths open
- Target: TSO24, TSO31 and TSO64 entry and service objects.
- Observation: TSO24, TSO31 and both TSO64 entry objects assemble from maintained source with z/PDOS Classic Assembler. The selected `tso31-sdk-files` PDPCLIB service assembles and closes all four host XMIT links with source-built C decks. Sequential write/read and PDS read passed on z/OS 1.5 for TSO31 and both TSO64 entry modes. VSAM/IDCAMS rejection and other service paths remain unqualified by guest execution.
- Evidence: On 3 October 2026 `libc/scripts/build-tso-entries.crexx` built all four entries. `sdk/scripts/build-tso-native.crexx` built and checked the PDPCLIB service, verified TSO31/64 deck closure and linked four source-input XMIT transports. The AMODE64/RMODE ANY TSO64 entry SHA-256 is `5a12f242e19b1f9a89ee0203e7f3e3d8f9f8a27253c4658954dd46e8114a603f`. The [source candidate checkpoint](../../docs/updates/2026-10-03-sdk-source-candidate.md) records the complete host and installed package result.
- Acceptance: Build all native entry and service objects with cross-platform tools, link each complete profile, and repeat affected guest service checks. Source-built entry cards alone do not meet this criterion.

## LIBC-004: TSO24 dataset I/O

- Type: defect
- Status: Open; excluded from 0.1.0
- Target: `tso-zos24-v1` with the source-built PDPCLIB service on z/OS 1.5.
- Observation: The file smoke faults during above-the-line SWA lookup. The
  simple TSO24 entry and return path passes; that does not establish file I/O.
- Evidence: The [3 October candidate checkpoint](../../docs/updates/2026-10-03-sdk-source-candidate.md)
  records the fault and exclusion. The installed consumer rejects the named
  file-smoke fixture for TSO24, but this is not a general prohibition on
  file calls in arbitrary user programs.
- Acceptance: Repair the owning native service/addressing path, qualify the
  affected TSO24 file operations on the named guest, and only then extend the
  advertised file subset. Maintained PDPCLIB changes belong in z/PDOS.

## LIBC-005: Extend the selected native service qualification

- Type: qualification and compatibility
- Status: Open
- Target: TSO31/64 native services beyond the released sequential/PDS subset.
- Observation: FBS extend/positioning is excluded after bypassing the inherited
  NOTE/TRKCALC path. VSAM/IDCAMS and supervisor switching return unsupported
  by source contract; their rejection paths, dynamic allocation and command/
  prefix services lack the required guest checks. Inherited `TCBFA` access is
  not a designated IBM programming interface.
- Evidence: The [release limits](../../docs/RELEASE-PLAN.md#known-limits) and
  the linked 3 October checkpoint distinguish the source implementation from
  the paths executed on z/OS 1.5.
- Acceptance: Record supported operations individually, resolve unsafe or
  undocumented interface dependencies in the owning component, and qualify
  the affected success/error paths on each newly advertised guest. Do not
  infer modern z/OS compatibility from the existing z/OS 1.5 result.
