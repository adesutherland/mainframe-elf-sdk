# Mainframe ELF SDK release plan

The first binary release is 0.1.0 for qualified macOS and Linux hosts.
Windows follows as 0.1.1 after a native build and installed-package check on
the Windows host. The 0.1.0 assets will make no Windows support claim.

The release build starts with checked upstream GCC, binutils and newlib source
and maintained SDK and z/PDOS source. Ordinary host C/C++ tools and development
libraries are prerequisites. No z/OS assembler, binder, IBM macro library,
prebuilt target runtime archive or native object deck may be a build input for
an SDK release asset. Guest execution is a separate qualification gate; it
does not supply build artifacts.

## Required 0.1.0 gates

1. Give each maintained component one editable `src/` tree, a clear upstream
   record and licence, a single backlog and build documentation. Optional
   frozen archives are excluded from normal builds and required tests.
2. Build GCC, GNU binutils, the selected newlib libraries, CMS/TSO adapters,
   native entry and service objects, and host packagers from checked source.
   Use z/PDOS `mf-classic-as` for classic native assembly and the maintained
   linker where needed. Extend those tools in z/PDOS for demonstrated SDK
   consumer gaps, with their own independent object checks.
3. Build the six advertised profiles and their applicable final MODULE or
   load-member transport from a fresh source tree. Run an installed consumer
   outside the producer tree and check package identities and notices.
4. Record exact source, tool, archive and output identities. Repeat affected
   guest checks when changed bytes or contracts make old evidence inapplicable.
5. Qualify each advertised host platform and inspect the complete distributable
   before creating a release tag or publishing assets.

The low RMODE64 launchers currently belong to a Mainframe Lab application
consumer. Keep that application source outside the generic SDK. A generic
SDK release must not ship those objects as opaque required inputs: either a
source-built generic launcher is established and qualified, or the application
HIGH route remains a separately documented consumer outside 0.1.0.

## Windows 0.1.1

On the Windows host, build native Windows executables from the same maintained
source and checked upstream inputs, package the SDK, and run its installed
consumer and failure controls. Record Windows-specific dependencies and
qualification. The 0.1.1 release follows only after those gates pass.

## Current status

This plan is approved direction, not a completed release. On Apple Silicon
macOS, the direct-source producer builds GCC/binutils and all five selected C
sysroots. Source-built host consumers compile, link and export CMS24/31
MODULEs, TSO24/31/64 ELF decks and the `vmkernel` assembly component. The
fuller CMS plain-C consumer also links and exports after correcting archive
membership and the strict linker script.

The existing package recipe remains a bootstrap route: it consumes pinned
runtime archives and ASMA90-produced native decks. A separate source-input
candidate route now installs all five source-built C sysroots, source-built
host and Classic tools, and source-built TSO native entry and PDPCLIB service
objects. Its installed consumer passed CMS24/31 MODULEs, TSO24/31/64 C decks,
four complete native XMIT links, and the freestanding component on macOS.
The TSO31/64 service file subset supports sequential write/read and PDS read;
VSAM, IDCAMS and supervisor-mode switching are rejected by the selected
service. All four source-built TSO transports restored and returned 42 for a
simple C consumer on z/OS 1.5. TSO31 and both TSO64 entry modes passed the
file smoke there. TSO24 dataset I/O faults during above-line SWA lookup and is
outside the 0.1.0 file subset; its simple entry/return path passed. The
selected file service bypasses inherited NOTE/TRKCALC positioning, so FBS
extend is outside this subset. The complete maintained
PDOS-profile PDPCLIB native source also assembles with source-owned PDOS service definitions.
The
[3 October native review](updates/2026-10-03-funhead-pdpclib-tso64.md) and
[TSO64 source entry checkpoint](updates/2026-10-03-tso64-source-entry.md)
record the earlier sequence; the [source candidate checkpoint](updates/2026-10-03-sdk-source-candidate.md)
records the new host result and guest evidence. A clean-checkout release build,
Linux installed-package qualification and remaining service-path guest checks
are open 0.1.0 gates. Windows remains
the separately staged 0.1.1 host gate.
