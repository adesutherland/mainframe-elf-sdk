# Mainframe ELF C

`mf-elf-cc` is the SDK's GCC 16.2.0-based C cross-compiler. The current host
program is `s390-linux-gnu-gcc` with explicit historical CMS, TSO and
freestanding profiles. It emits ELF objects; profile checks and the SDK
packager own the later target-specific steps.

`src/gcc16/` is the only maintained compiler implementation. The checked
GCC upstream archive supplies the rest of the generated build tree. Edit
`src/gcc16/` directly and use Git history for changes. `archive/recovery/`
preserves the initial imported patch history but is absent from normal builds.
See [UPSTREAM.md](UPSTREAM.md) for the exact base and licence, and
[the SDK guide](../docs/compiler/SDK.md) for profile limits.
