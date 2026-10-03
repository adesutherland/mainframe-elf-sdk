# Mainframe ELF SDK

I want to make it practical to build CMS and TSO programs on an everyday
computer, starting with source, producing checked target objects and packaging
them for a named mainframe environment. The SDK uses GCC, binutils and newlib
with explicit historical machine profiles. Its source-build goal needs no
z/OS assembler, binder or prebuilt target runtime object.

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

## Source ownership

| Component | Maintained source | Foundation |
| --- | --- | --- |
| [Compiler](compiler/README.md) | `compiler/src/gcc16/` | Checked official GCC 16.2.0 archive; the old patch series is frozen in `compiler/archive/` |
| [C library](libc/README.md) | `libc/src/newlib/` and `libc/src/adapters/` | Checked official newlib 4.6.0.20260123 archive, with original notices |
| [SDK host tools](sdk/README.md) | `sdk/src/` for checkers, exporters, layouts, profiles and the kernel component; `sdk/scripts/` for recipes | Original Mainframe Lab work under MIT; unchanged official binutils 2.47 source is checked and built by the producer |

Optional `archive/` directories are frozen provenance and recovery records.
They remain in Git but are excluded from the release source extraction and
its build inputs.
We maintain current code in each component's `src/` tree and use Git history
for previous versions. The separate [z/PDOS repository](https://github.com/adesutherland/z-pdos)
owns the Mainframe Classic Assembler, Linker and PDPCLIB. Its maintained source
is a cross-platform input to the complete native SDK path.

See [the documentation index](docs/README.md) and [SDK guide](docs/compiler/SDK.md) for each profile's C, ISA, runtime,
addressing, packaging and qualification limits. [The release plan](docs/RELEASE-PLAN.md)
records the source-build and platform gates. `sdk/source-files.txt` is the
reviewed source list. `SOURCE-MANIFEST.json` hashes every source file. The
upstream source archives are downloaded or supplied from a checked cache;
their URLs and SHA-256 locks are in `sdk/scripts/compiler_sdk.py`.

To reconstruct the host tools from this standalone tree:

```sh
python3 sdk/scripts/compiler_sdk.py prepare --cache /path/to/archive-cache \
  --work /path/to/new-sdk-work --download
python3 sdk/scripts/compiler_sdk.py build-tools --work /path/to/new-sdk-work
```

On Apple Silicon macOS, install Homebrew `gcc`, GMP, MPFR, MPC and ISL (the
producer uses `gcc-16`/`g++-16`). On Linux, install GCC/G++, GMP, MPFR, MPC,
ISL, make, patch, Python 3.12+ and archive utilities. `prepare` refuses an
existing output root, verifies the upstream archives, and copies the checked
maintained GCC files from [`compiler/src/gcc16/`](compiler/src/gcc16/) into its
generated build tree. `build-tools` installs a fresh C cross-compiler and
binutils into that root. Edit GCC source directly under `compiler/src/gcc16/`;
Git history preserves changes. The initial recovery series is frozen reference
material and is not used by the build. The selected newlib configuration is
maintained directly under [`libc/src/newlib/`](libc/src/newlib/).
The selected runtime recipes under `libc/scripts/` also need a host cREXX
executable. They build checked libraries for the five application profiles;
the CMS recipes also build checked adapters and startup objects. The
`build-tso-entries.crexx` recipe uses a source-built z/PDOS Classic Assembler
to produce checked TSO24, TSO31 and both TSO64 native entry objects.

The `sdk/scripts/build-tso-native.crexx` recipe adds the maintained z/PDOS
PDPCLIB TSO file service and links source-built C decks to complete host XMIT
transports. The file profile supports sequential and partitioned datasets and
explicitly rejects VSAM. `sdk/scripts/compiler_sdk_source_package.py`
installs the source-built sysroots, host tools and native objects. The macOS
arm64 and Linux x64 builds both passed an independent six-profile installed
consumer, including four native TSO XMIT links. The macOS CMS24/31 MODULEs
returned 42 on VM/370 CE and z/VM 4.4. All four simple TSO C members
restored and returned 42 on z/OS 1.5. TSO31 and both TSO64 entry modes passed
sequential write/read and PDS read there; TSO24 dataset I/O remains outside
the 0.1.0 file subset. The
[source candidate checkpoint](docs/updates/2026-10-03-sdk-source-candidate.md)
records the exact host and guest evidence and remaining service limits.

The older `sdk/scripts/compiler_sdk_package.py` can assemble a versioned local SDK from
the fresh host tools and exact, hash-checked CMS24, CMS31 and MVS/TSO runtime
archives. The runtime archives and ASMA90-produced native TSO objects are
**pinned bootstrap inputs**, not outputs of this fresh compiler build. Obtain
them only with their original notices and verified hashes. The package
command's explicit options name each input. This retained bootstrap route is
not a source-only 0.1.0 release and runs only from the checkout that retains
the frozen archives. Its installed SDK carries its own
tools, profile-specific sysroots, manifests and a six-profile consumer check:

```sh
python3 sdk/scripts/compiler_sdk_package.py --help
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
package. The source producer has no required proprietary commercial licence.
The source-built native route removes prebuilt objects from the local
candidate. The CI definition builds source tools on macOS and Linux; it does
not publish artifacts or qualify the changed native inputs in a guest.
