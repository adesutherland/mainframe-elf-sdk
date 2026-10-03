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
| GCC 16.2.0 | Locked official GNU archive; `compiler/LICENSE` and GCC runtime exception in the archive | GPLv3 applies to GCC and the maintained S/390 target changes in `compiler/src/gcc16/`. The old recovery series is frozen reference material. |
| GNU binutils 2.47 | Locked official GNU archive and its file notices | Apply the upstream terms to assembler, linker and related host tools. |
| newlib 4.6.0.20260123 | Locked Sourceware archive; `libc/LICENSE` and file notices | Maintain selected changes in `libc/src/newlib/` and preserve component notices for headers and libraries. |
| Original Mainframe Lab adapters, checkers, exporters, tests and guides | Exact paths classified as original below | MIT, copyright 2026 Adrian Sutherland. Inherited file-level notices remain effective. |
| PDPCLIB TSO service source | Maintained source in z/PDOS, SDK `pdptop.mac`, and original source notice | The source notice credits Paul Edwards and contributors and states its own public-domain claim. This does not describe other project files. The SDK repository's frozen repair patches are excluded from its release source extraction. |
| Historical native TSO object decks | Baseline `TENTRY.obj`/`PDPSUP-tso.obj`, accepted `PDPL34.obj`/`E64.obj`, and RMODE64 low launchers `LAU65O.obj`/`LAVM65O.obj`/`LAC65O.obj`; hashes in `sdk/scripts/compiler_sdk_package.py` | These are separately supplied bootstrap inputs for the older local package route. The source-built 0.1.0 candidate uses maintained TSO entry source and the z/PDOS PDPCLIB service selection instead. Historical deck redistribution provenance remains to be reviewed. |

The source tree contains no IBM images, private correspondence,
credentials, cREXX application/platform source, RXBIN libraries or native
object decks. The cREXX-specific low-launcher source remains outside this
public compiler extraction. Local SDK packages assembled with those
objects remain local until provenance and redistribution rights are reviewed.
The source producer's use of GPL and other open-source terms is intentional;
it does not imply that a proprietary commercial compiler licence is required.

## Source allowlist classification

The exact publication inventory is `sdk/source-files.txt`. The following
classification applies to that list:

| Source paths | Origin class | Licence scope |
| --- | --- | --- |
| `compiler/src/gcc16/**` | GNU GCC source and modifications | GCC GPLv3 terms and retained file notices; never covered by a grant for original lab code. |
| `compiler/LICENSE` | GNU licence text | Retained verbatim. |
| `libc/src/newlib/**` | newlib modifications | Original newlib component terms and notices. |
| `libc/LICENSE` | newlib licence text | Retained verbatim. |
| All other files in `sdk/source-files.txt`, except the exact root `LICENSE` text | Original Mainframe Lab code, tests, metadata or prose | MIT applies here, subject to each file's own inherited notice. This class does not include downloaded GCC, binutils, newlib, PDOS or PDPCLIB source archives. |
| Root `LICENSE` | Standard MIT licence text | States the grant for the original class only; `LICENSING.md` controls the component scope. |

This is a provenance classification, not an attempt to relicense inherited
work. A file-level notice takes precedence if review finds a further inherited
component. The separately supplied historical native object decks and runtime bootstrap
archives are absent from this public source allowlist. The repository's frozen
`compiler/archive/`, `libc/archive/` and `sdk/archive/` histories are also
excluded from the release source extraction; their inherited terms still apply
to the files retained in Git.
