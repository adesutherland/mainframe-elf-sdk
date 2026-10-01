# Mainframe ELF SDK

The component names are Mainframe ELF C (`mf-elf-cc`), Mainframe ELF Assembler
(`mf-elf-as`) and Mainframe ELF Packager (`mf-elf-pack`). These are the agreed
public interface names; the commands below describe the existing implementation.

This source tree reconstructs a GCC 16.2.0 and GNU binutils 2.47 C
cross-toolchain for historical CMS, TSO and freestanding kernel components.
It is the compiler project extracted from Mainframe Lab, with its own reviewed
source inventory. cREXX is an external application consumer of selected
profiles. The legacy GCCCMS/GCCMVS compiler remains useful in the lab; this
project does not replace it or change the production cREXX pipeline.

The supported profile identities are `vm370-4381-v1`, `cms20-esa31-v1`,
`tso-zos24-v1`, `tso-zos31-v1`, `tso-zos64-v1`, and the freestanding `vmkernel`.
The superseded `vmce-cms-kernel-v1` definition remains as historical migration
evidence, without an enabled sysroot. A dedicated PDOS application target and
runtime are outside this SDK. Shared PDLD/XMIT helpers are included because
PDOS tests unchanged TSO binaries.

See [the SDK guide](docs/compiler/SDK.md) for each profile's C, ISA, runtime,
addressing, packaging and qualification limits. `sdk/source-files.txt` is the
reviewed source list. `SOURCE-MANIFEST.json` hashes every file in an extracted
tree. The upstream source archives are downloaded or supplied from a checked
cache; their URLs and SHA-256 locks are in `tools/compiler_sdk.py`.

To reconstruct the host tools from this standalone tree:

```sh
python3 tools/compiler_sdk.py prepare --cache /path/to/archive-cache \
  --work /path/to/new-sdk-work --download
python3 tools/compiler_sdk.py build-tools --work /path/to/new-sdk-work
```

On Apple Silicon macOS, install Homebrew `gcc`, GMP, MPFR, MPC and ISL (the
producer uses `gcc-16`/`g++-16`). On Linux, install GCC/G++, GMP, MPFR, MPC,
ISL, make, patch, Python 3.12+ and archive utilities. `prepare` refuses an
existing output root, verifies all upstream archives and nineteen GCC recovery
patches, and checks the result against the maintained editable target source.
`build-tools` installs a fresh C cross-compiler and binutils into that root.
For maintained GCC changes, follow the [source and recovery
workflow](toolchain/gcc16/UPSTREAM.md): commit a focused source topic in this
repository, generate its recovery patch from that commit, refresh the source
manifest and replay the full patch series in a new work directory.

`tools/compiler_sdk_package.py` can then assemble a versioned local SDK from
the fresh host tools and exact, hash-checked CMS24, CMS31 and MVS/TSO runtime
archives. The runtime archives and ASMA90-produced native TSO objects are
**pinned bootstrap inputs**, not outputs of this fresh compiler build. Obtain
them only with their original notices and verified hashes. The package
command's explicit options name each input. The installed SDK carries its own
tools, profile-specific sysroots, manifests and a six-profile consumer check:

```sh
python3 tools/compiler_sdk_package.py --help
python3 /path/to/installed-sdk/tools/compiler_sdk_consume.py \
  --sdk /path/to/installed-sdk --out /path/to/new-consumer-output
```

The consumer runs outside the producer tree. It verifies every installed
file, then compiles, links and packages a small C program for each application
profile and a real freestanding CMS storage component for `vmkernel`. The
TSO64 check also emits a separate RMODE64 high-code deck and checks its entry,
stack and relocations against three pinned low launcher objects. It does
not by itself prove native guest execution or the complete RXC/RXAS/RXVM
application chain. Existing guest results apply only to their recorded bytes
and contracts. The z/OS 1.5 RMODE ANY TSO64 route is accepted; the separate
low-launcher/RMODE64 high route has bounded PDOS execution and remains
unverified on modern z/OS.

The separate laboratory cREXX 0017 consumer checks RXAS, RXVM and RXC against
this SDK using an external frozen cREXX source tree, matching host cREXX and
hash-pinned application glue. That harness, the application source and the
glue are outside this public compiler project. The public source tree contains
only generic compiler/runtime/packager code and generic C profile checks.

Original Mainframe Lab code and documentation are [MIT licensed](LICENSE);
inherited GCC, binutils, newlib, PDOS/PDLD and PDPCLIB material retains its
own terms. Read [licensing and provenance](LICENSING.md) before distributing a
package. The source producer has no required proprietary commercial licence. The
native assembler/macro regeneration and prebuilt object redistribution gaps
remain explicit. The CI definition builds source tools on macOS and Linux;
it does not publish artifacts or qualify the unavailable native inputs.
