# Licensing and provenance

This project combines separately licensed components. The standard
[MIT licence](LICENSE), copyright 2026 Adrian Sutherland, applies to the
original Mainframe Lab code and documentation classified below. It does **not**
relicense GCC or GCC-derived changes, binutils, newlib, PDOS/PDLD/PDPCLIB
changes, inherited licence texts or native object decks. Preserve the original
copyright and licence notices in every upstream archive and runtime bootstrap
package.

| Component | Source and notice | Treatment |
| --- | --- | --- |
| GCC 16.2.0 | Locked official GNU archive; `toolchain/gcc16/COPYING3` and GCC runtime exception in the archive | GPLv3 applies to GCC; the maintained S/390 target changes and generated recovery patches accompany it. |
| GNU binutils 2.47 | Locked official GNU archive and its file notices | Apply the upstream terms to assembler, linker and related host tools. |
| newlib 4.6.0.20260123 | Locked Sourceware archive; `patches/newlib/COPYING.NEWLIB` and file notices | Preserve component notices for headers and libraries. |
| Original Mainframe Lab adapters, checkers, exporters, tests and guides | Exact paths classified as original below | MIT, copyright 2026 Adrian Sutherland. Inherited file-level notices remain effective. |
| PDPCLIB TSO service source | Pinned upstream `mvssupa.asm`, `pdptop.mac`, visible source patches and original source notice | The source notice credits Paul Edwards and contributors and states its own public-domain claim. This does not describe other project files. |
| Native TSO object decks | Baseline `TENTRY.obj`/`PDPSUP-tso.obj`, accepted `PDPL34.obj`/`E64.obj`, and RMODE64 low launchers `LAU65O.obj`/`LAVM65O.obj`/`LAC65O.obj`; hashes in `tools/compiler_sdk_package.py` | These are separately supplied bootstrap inputs. ASMA90 and IBM macro-dependent regeneration is not yet replaced by a complete open-source path. Redistribution provenance requires review before any public binary artifact includes them. |

The source extraction contains no IBM images, private correspondence,
credentials, cREXX application/platform source, RXBIN libraries or native
object decks. The cREXX-specific low-launcher source remains outside this
public compiler extraction. Local SDK packages assembled with those
objects remain local until provenance and redistribution rights are reviewed.
The source producer's use of GPL and other open-source terms is intentional;
it does not imply that a proprietary commercial compiler licence is required.

## Source allowlist classification

The exact publication inventory is `sdk/source-files.txt`. The following
classification applies to that list, including the generated top-level copies
of `sdk/PROJECT-README.md`, `sdk/LICENSING.md`, `sdk/.gitignore` and
`sdk/LICENSE-MIT`:

| Source paths | Origin class | Licence scope |
| --- | --- | --- |
| `toolchain/gcc16/source/**` and `patches/gcc16/maintained/*.patch` | GNU GCC source and modifications | GCC GPLv3 terms and retained file notices; never covered by a grant for original lab code. |
| `toolchain/gcc16/COPYING3` | GNU licence text | Retained verbatim. |
| `patches/newlib/0001-cms-elf-poc.patch` | newlib modification | Original newlib component terms and notices. |
| `patches/newlib/COPYING.NEWLIB` | newlib licence text | Retained verbatim. |
| `patches/pdos390/*.patch` | PDLD writer modifications only; the original combined PDLD/PDOS loader patch remains in the private laboratory | Follow the original PDLD source notices; no grant for original lab code overrides them. |
| `patches/pdpclib/*.patch` | PDPCLIB modifications | Follow the source notice and attribution for each patched file. |
| All other files in `sdk/source-files.txt`, except the exact `sdk/LICENSE-MIT` text | Original Mainframe Lab code, tests, metadata or prose | MIT applies here, subject to each file's own inherited notice. This class does not include downloaded GCC, binutils, newlib, PDOS or PDPCLIB source archives. |
| `sdk/LICENSE-MIT` and extracted `LICENSE` | Standard MIT licence text | States the grant for the original class only; `LICENSING.md` controls the component scope. |

This is a provenance classification, not an attempt to relicense inherited
work. A file-level notice takes precedence if review finds a further inherited
component. The separately supplied native object decks and runtime bootstrap
archives are absent from this public source allowlist.
