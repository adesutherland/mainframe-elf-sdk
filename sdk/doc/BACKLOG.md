# SDK host and package backlog

## SDK-001: Source-only release package

- Type: qualification
- Status: Done for 0.1.0
- Target: macOS and Linux 0.1.0 installed SDK and its six advertised profiles
- Observation: The source-input 0.1.0 route packages rebuilt sysroots, Classic tools and native objects and passed independent installed consumers on macOS arm64 and Linux x64. CMS24/31 guest entry calls passed on VM/370 CE and z/VM 4.4. Simple TSO24/31/64 entry and TSO31/64 sequential/PDS file checks passed on z/OS 1.5. TSO24 dataset I/O remains excluded. The [0.1.0 release](https://github.com/adesutherland/mainframe-elf-sdk/releases/tag/v0.1.0) was published on 3 October 2026. The old bootstrap package recipe remains a separate historical route.
- Evidence: `scripts/compiler_sdk_source_package.py`, the producer C-deck mode of `scripts/compiler_sdk_consume.py`, `scripts/build-tso-native.crexx` and the [3 October source candidate checkpoint](../../docs/updates/2026-10-03-sdk-source-candidate.md) record the package and guest results. The release at `f5a57fe` records both final host assets and the passing Linux [run 37137522440](https://github.com/adesutherland/mainframe-elf-sdk/actions/runs/37137522440).
- Acceptance: Install all profile sysroots, host helpers and native objects from checked maintained source without bootstrap archives or private assembler inputs; run an independent installed consumer, inspect identities and notices, qualify Linux, then prepare reviewable 0.1.0 assets.

## SDK-002: Native Windows host release

- Type: qualification
- Status: Open
- Target: Windows 0.1.1 on the second Windows host
- Observation: The existing producer has no native Windows build or installed consumer evidence.
- Evidence: `docs/RELEASE-PLAN.md` stages Windows after non-Windows 0.1.0.
- Acceptance: Build Windows executables and package from the same checked source, run the installed consumer and failure controls on Windows, and inspect the distributable before a 0.1.1 tag.

## SDK-003: CMS math consumer link closure

- Type: defect
- Status: Done
- Target: source-built CMS24 and CMS31 sysroots with the selected math routines
- Observation: The simple source-built CMS consumers link and export, but `tests/cms-newlib/plain-core.c` exposed orphan `.rela.rodata.*` sections under the strict linker script and unresolved `fabs`, `floor` and `scalbn` from the first library grouping.
- Evidence: The first 3 October 2026 `build/cms-source-plain-core-link.log` failed. With 131 selected CMS core members and an explicit `.rela.rodata.*` rule, `build/cms-source-plain-core-v3.log` shows compile, checked link, MODULE write and independent MODULE verify for CMS24 and CMS31. GNU ld 2.47 documents explicit input-section wildcards and rejects unmatched sections with `--orphan-handling=error`.
- Acceptance: Put the selected CMS support members in their intended archive, retain the strict orphan check with explicit relocation sections, and link/export the full plain-C consumer from both source-built sysroots. Guest behavior remains a separate gate.

## SDK-004: Native XMIT member identity

- Type: defect
- Status: Done
- Target: Source-built TSO24, TSO31 and both TSO64 transports
- Observation: Classic Linker derives the IEBCOPY member from the first eight characters of its `-o` argument. Passing an absolute output path made z/OS restore a member named `/PRIVATE`.
- Evidence: The first z/OS 1.5 `RECEIVE` on 3 October 2026 loaded `/PRIVATE` with IEBCOPY severity 0. The source pipeline and installed consumer now link from their output directory with eight-character names, and the XMIT unload checker validates member and AMODE/RMODE before installation. The corrected `SDKTS64A` transport restored with IEBCOPY severity 0.
- Acceptance: All four transports expose stable, valid member names and fail the host check if the IEBCOPY directory disagrees.

## SDK-005: Profile and manifest description drift

- Type: metadata/documentation defect
- Status: Open; recorded during the 4 October documentation review
- Target: Descriptive fields in `src/profiles/*.json` and generated package
  manifests; no profile identity or code change is made by this review.
- Observation: TSO24's JSON describes HLASM native glue, TSO31/24 load-format
  text mentions the native binder, and TSO64 describes only its AMODE31 entry
  despite the source package including an AMODE64 entry. Some ABI source paths
  refer to documents left in Mainframe Lab. Profile status fields and the
  source packager's fixed “guest qualification pending”/“host candidate” text
  also predate the release record.
- Evidence: `src/profiles/tso-zos24-v1.json`, `tso-zos31-v1.json`,
  `tso-zos64-v1.json`, `cms20-esa31-v1.json` and `vm370-4381-v1.json`;
  `scripts/compiler_sdk_source_package.py` writes the manifest descriptions.
  The [architecture guide](../../docs/compiler/SDK.md) and
  [release record](../../docs/RELEASE-PLAN.md) explain current behavior.
- Acceptance: Reconcile descriptive metadata with the source-built pipeline
  and public documentation without changing the binary contracts. Keep a
  package producer's own host-only result distinct from later release guest
  qualification, and preserve the identities of already released artifacts.

## SDK-006: File example success and failure share a return code

- Type: qualification-fixture defect
- Status: Open; identified by source inspection on 4 October 2026
- Target: `tests/tso/sdk-file-smoke.c`, installed as
  `contracts/tso/sdk-file-smoke.c`.
- Observation: The PDS content/close failure branch returns 42, which is also
  the final success code. A harness checking only the return code could accept
  that failure. The fixture additionally requires a prepared PDS member
  beginning `/*`; `sdk-file-setup.rexx` allocates but does not create it.
- Evidence: The two `fgetc` comparisons and `fclose` branch return 42 before
  `SDK PDS PASS` and `SDK MISSING DD PASS`. The recorded release guest runs
  observed all three success messages and RC 42, so this observation does not
  invalidate those recorded passes. No new execution was performed for this
  documentation review.
- Acceptance: Give each failure path an unambiguous non-success code and
  qualify a deliberately wrong PDS member against the guest harness. Until
  then, require all three success messages as documented in the usage guide.
