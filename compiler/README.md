# Mainframe ELF C

This component adapts GCC 16.2.0 to generate C code for the SDK's historical
CMS, TSO and freestanding profiles. The installed compiler is named
`s390-linux-gnu-gcc`. **Mainframe ELF C** (`mf-elf-cc`) is the component's
public name; 0.1.0 does not install a wrapper command with that name.

The maintained target work provides conservative S/370 instruction lowering
and profile-specific restrictions within GCC's S/390 backend. TSO64 uses the
z900 LP64 path with software floating point. The compiler emits assembly and
ELF objects through GNU binutils; runtime adapters and native-format exporters
complete the operating-system-specific program. The retained GNU target name
does not make these CMS/TSO programs Linux executables.

Read [architecture and profiles](../docs/compiler/SDK.md) before selecting
compiler options. In particular, 24/31-bit addressability, C type widths,
native service linkage and code placement are separate choices. The current
recipes use GNU C99 mode with selected runtime support; this is not a complete
C99 or floating-point qualification claim.

`src/gcc16/` is the single maintained home for our GCC changes. A checked
official archive supplies the rest of GCC. Preparation verifies the upstream
files and copies the maintained files into a generated build tree; it does
not apply the frozen `archive/recovery/` patch series. GCC-derived changes
remain under GCC's licence terms.

- [Upstream version, hashes and origin](UPSTREAM.md)
- [Build the toolchain and SDK](../sdk/README.md)
- [Compiler backlog](doc/BACKLOG.md)
- [Released scope](../docs/RELEASE-PLAN.md)
