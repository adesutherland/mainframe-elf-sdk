# Mainframe ELF SDK documentation

Start with the [project overview](../README.md) for the purpose of the SDK
and the released platforms. Choose a guide according to what you want to do:

| I want to… | Read |
| --- | --- |
| Download the SDK and build an example | [Using an installed SDK](../sdk/doc/INSTALLED-README.md) |
| Understand the pipeline and choose a profile | [Architecture and profiles](compiler/SDK.md) |
| Understand our code, the PDOS components and licence dependencies | [Licensing and provenance](../LICENSING.md) |
| Rebuild the SDK from source | [Source build guide](../sdk/README.md) |
| See what 0.1.0 proves and what remains open | [Release status and limits](RELEASE-PLAN.md) |

## Contributing

The maintained implementation has three homes in this repository:

| Component | Overview and origin | Current work |
| --- | --- | --- |
| GCC target changes | [Compiler](../compiler/README.md) · [Upstream](../compiler/UPSTREAM.md) | [Compiler backlog](../compiler/doc/BACKLOG.md) |
| newlib integration and native adapters | [C runtime](../libc/README.md) · [Upstream](../libc/UPSTREAM.md) | [Runtime backlog](../libc/doc/BACKLOG.md) |
| Profiles, exporters and packaging | [SDK tools/builds](../sdk/README.md) · [Origin](../sdk/UPSTREAM.md) | [SDK backlog](../sdk/doc/BACKLOG.md) |

Classic Assembler, Classic Linker and PDPCLIB changes belong in
[z/PDOS](https://github.com/adesutherland/z-pdos). Read the applicable
`AGENTS.md` before editing a component. Development takes place on `develop`;
`main` contains approved integration and releases.

## Detailed development records

Dated reports describe the exact inputs and results at their checkpoints.
Read the current release guide first: an early candidate's limitations or
publication status may have changed later that day.

- [Source-built SDK candidate and guest results, 3 October](updates/2026-10-03-sdk-source-candidate.md)
- [Initial source build, 3 October](updates/2026-10-03-source-build.md)
- [PDPCLIB native service and macro review, 3 October](updates/2026-10-03-funhead-pdpclib-tso64.md)
- [TSO64 source entries, 3 October](updates/2026-10-03-tso64-source-entry.md)
- [Source publication, 1 October](updates/2026-10-01-source-publication.md)
- [Heap defaults, 30 September](updates/2026-09-30-heap-defaults.md)
- [CMS empty-input repair, 30 September](updates/2026-09-30-cms-empty-input.md)
- [Text-conversion switch, 29 September](updates/2026-09-29-text-conversion.md)

Frozen `archive/` directories preserve earlier recovery material. Normal
source builds use the maintained `src/` trees and exclude those archives.
