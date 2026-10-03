# SDK host tools and source inventory

`src/` owns the current host checkers, exporters, profile contracts, linker
layouts and freestanding kernel component. `scripts/` owns the maintained
producer, packager and consumer interfaces. The historical patch inputs under
`archive/` are frozen. The separate z/PDOS repository owns the live Classic
Assembler, Linker and PDPCLIB source.

This directory also owns the reviewed source inventory for Mainframe ELF SDK.
It is a **standalone source extraction**, not a claim that every runtime and
native adapter can already be rebuilt from source. The full lab
retains its legacy compiler and independent OS work. The dedicated PDOS
application target and runtime are outside this SDK; PDOS may exercise
unchanged TSO binaries through its supported services.

The separate public source repository uses MIT for original Mainframe Lab
files, with inherited GCC, binutils, newlib and PDOS/PDPCLIB terms preserved
as described in the repository's `LICENSING.md`. Local packages containing pinned native
object decks are not public artifacts until their redistribution provenance is
reviewed.

`source-files.txt` is the reviewed copy list and excludes the frozen
`archive/` directories. This standalone repository owns
the maintained source and producer; Mainframe Lab retains experiments and
qualification records. From this SDK checkout, make an independent source tree:

```sh
python3 sdk/scripts/compiler_sdk.py extract --out /tmp/mainframe-compiler-sdk-source
```

After extraction, all reconstruction commands run **inside that tree**. The
producer reads three locked upstream archives (GCC 16.2.0, GNU binutils 2.47,
newlib 4.6.0.20260123), checks SHA-256, and copies the maintained compiler and
newlib files directly from their `src/` trees into a new work root. Frozen
recovery patches are not used. `--cache` can point to a
verified read-only archive cache. Add `--download` to fetch missing official
archives; a changed response fails its pinned hash.

```sh
cd /tmp/mainframe-compiler-sdk-source
python3 sdk/scripts/compiler_sdk.py prepare --cache /tmp/sdk-archives --work /tmp/sdk-build
python3 sdk/scripts/compiler_sdk.py build-tools --work /tmp/sdk-build
```

On Apple Silicon macOS, the default build uses Homebrew `gcc-16`/`g++-16` and
the installed GMP, MPFR, MPC and ISL formulae. On Linux it uses the host
`gcc`/`g++` and development packages for those libraries. A working cREXX
host executable is needed for the selected runtime build recipes and external
application recipes;
no target cREXX binary is used to build the compiler. The source inventory
deliberately includes no cREXX application or platform source, RXBIN library,
guest image, credential, private correspondence, historical manual, or PDOS
kernel source. Generic C and newlib adapters remain in the SDK.

The source package preserves six active profile JSON contracts and the retired
`vmce-cms-kernel-v1` record for migration/rejection evidence. The active
profiles are historical CMS24, CMS31, TSO24, TSO31, TSO64, and common
`vmkernel`. Profile identity is checked by the existing checkers; `vmkernel`
has only freestanding component output. TSO64 has a qualified RMODE ANY route
and a separately reviewed low-launcher/RMODE64 package route; execution on
modern z/OS remains unverified.

GCC is GPLv3 with its retained runtime exception; binutils and newlib carry
their own upstream licences and file notices. PDPCLIB native service source
retains its own notice. The baseline native decks and accepted RMODE ANY
`PDPL34.obj`/`E64.obj` adapters were assembled with ASMA90. They are pinned
bootstrap inputs until a
complete open-source regeneration path is demonstrated. No proprietary
commercial licence is required by this producer, but a source-only claim for
those decks would be premature. See the [SDK guide](../docs/compiler/SDK.md)
and the laboratory's dated SDK report for the exact matrix and remaining gates.
