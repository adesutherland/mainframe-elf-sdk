# SDK host and package backlog

## SDK-001: Source-only release package

- Type: qualification
- Status: In progress
- Target: macOS and Linux 0.1.0 installed SDK and its six advertised profiles
- Observation: The old package recipe still accepts pinned runtime archives and ASMA90-produced native decks. A separate source-input candidate packages rebuilt sysroots, Classic tools and native objects and passes an independent installed macOS consumer. Simple TSO24/31/64 guest entry and TSO31/64 sequential/PDS file checks passed on z/OS 1.5. TSO24 dataset I/O faults in SWA lookup and is outside the 0.1.0 file subset. A clean-checkout build, Linux installed package and remaining service-path checks remain open.
- Evidence: `scripts/compiler_sdk_package.py` names the old bootstrap inputs. `scripts/compiler_sdk_source_package.py`, the producer C-deck mode of `scripts/compiler_sdk_consume.py`, `scripts/build-tso-native.crexx` and the [3 October source candidate checkpoint](../../docs/updates/2026-10-03-sdk-source-candidate.md) record the local package, manifest and six-profile installed consumer and guest results. The Linux installed-candidate workflow is the outstanding host gate.
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
