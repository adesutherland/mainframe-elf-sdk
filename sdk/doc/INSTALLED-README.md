# Mainframe ELF SDK local package

This directory is an installed cross SDK. The `host/gcc/` and
`host/binutils/` directories contain the host programs; `sysroots/` separates
CMS, TSO and freestanding component profiles. Run the installed consumer in
an empty output directory to verify the package and build a small program for
each advertised profile:

```sh
python3 tools/compiler_sdk_consume.py --sdk /path/to/this-sdk \
  --out /path/to/new-consumer-output
```

The package manifest records its exact source and runtime inputs. A successful
host consumer checks the installed compile, object, link and transport route;
it does not itself establish guest execution. Read `SDK-MANIFEST.json` and the
profile files before treating a transport as qualified for a particular guest.

Older local packages built by `compiler_sdk_package.py` include pinned
bootstrap runtime archives and native object decks. Those packages are not
source-only 0.1.0 release artifacts. The source-only release gate is described
in the project `docs/RELEASE-PLAN.md`.

A package produced by `compiler_sdk_source_package.py` includes source-built
Classic Assembler/Linker binaries and native TSO entry/service objects. Its
installed consumer also links TSO24, TSO31 and two TSO64 native XMIT
transports. The package manifest identifies it as a local candidate until
the host and guest release gates pass.

`notices/` contains the SDK licence scope and the z/PDOS Classic Assembler,
Classic Linker and PDPCLIB component notices. Each sysroot also carries its
runtime notices.
