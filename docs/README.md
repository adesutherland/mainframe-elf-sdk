# Mainframe ELF SDK documentation

- [SDK and profile guide](compiler/SDK.md): current interfaces, machine
  profiles, packaging and qualification limits.
- [Release plan](RELEASE-PLAN.md): 0.1.0 source-build and host gates, followed
  by Windows 0.1.1.
- [Source-build checkpoint](updates/2026-10-03-source-build.md): exact
  Mac host compiler, library and C sysroot results.
- [Native PDPCLIB and TSO64 review](updates/2026-10-03-funhead-pdpclib-tso64.md):
  source-built PDOS service assembly, the TSO macro boundary and the distinct
  TSO64 entry requirements.
- [TSO64 source entry checkpoint](updates/2026-10-03-tso64-source-entry.md):
  both source-built entry decks, selected service interface and host link
  shape at that checkpoint.
- [Source candidate checkpoint](updates/2026-10-03-sdk-source-candidate.md):
  complete host native links, installed six-profile macOS and Linux packages,
  CMS24/31 entry checks and the z/OS 1.5 TSO31/64 file subset.

Each component has its own README, UPSTREAM.md, AGENTS.md and single
`doc/BACKLOG.md`. Enduring guides explain current design, the release plan
owns acceptance, and dated reports retain exact observations. Old reports
remain historical evidence, not a statement that the current working tree
has been released.
