# Newlib CMS ELF proof configuration

Base: newlib `4.6.0.20260123`, pinned by `tools/compiler_sdk.py`.
`COPYING.NEWLIB` is the unmodified upstream collection of licence notices;
individual files retain their copyright and permission notices in the source
work tree. In particular, the imported vfprintf implementation carries its
Berkeley attribution. This is not a blanket licence for the project.

Mainframe Lab modifications, 19 September 2026: allow a generic-C `s390` CPU
configuration, select an explicit `s390-unknown-none` OS boundary, and identify
GCC's big-endian IEEE soft-float representation for headers. The last setting
does not qualify floating-point arithmetic or formatting on historical S/370.

The libc implementation itself is unchanged. The original laboratory proof
built a 58-object selected integer/stdio/allocator subset from
`tests/libc-poc/newlib-objects.txt`. This is not a full newlib or libm build.
The SDK package currently accepts separately checked runtime archives as
bootstrap inputs; its source producer does not claim to rebuild those archives.

The added configuration lines may be used, copied, modified and distributed
without restriction. Upstream context and source remain under their respective
notices in `COPYING.NEWLIB` and the source files.
