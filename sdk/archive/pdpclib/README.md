# PDPCLIB TSO candidate

Maintained PDPCLIB source and new library fixes belong to
[z-pdos/pdpclib](https://github.com/adesutherland/z-pdos/tree/develop/pdpclib).
This SDK preserves patches for its pinned native bootstrap inputs; they do
not form a second maintained library fork. Regenerating those inputs still
requires the documented native assembly and affected qualification.

The pinned source is PDOS revision
`0fe81209e78d022b40301f86f97c7f4d3e406d0a`. MVSSUPA retains Paul Edwards's
public-domain notice and the original contributor history, including Gerhard
Postpischil. The vendor file is unchanged. The laboratory's retained baseline
used a checked PDSE-patched copy. The TSO31 high-residence candidate uses
`sdk/scripts/tso31_native_source.py --upstream vendor/pdos/pdpclib/mvssupa.asm
--out BUILD-DIRECTORY` to apply patches 0001–0003, in order, to another fresh
directory. The preparer checks the pristine source hash, patch application,
five retained entry points and the two fullword dynamic-request stores. It
copies `entry31.asm` and `pdptop.mac` beside the prepared source and records
every source hash in `source-manifest.json`.

The PDSE candidate corrects the compatibility definition of the format-1
DSCB SMS flags from offset 62 to offset 78, and skips the PDS-only DS1LSTAR
test for PDSEs. The original field reference pointed into DS1SYSCD and did
not read the PDSE bit. The actual z/OS 1.3 IECSDSL1 expansion confirms these
offsets. The original service and a branch-only first candidate both returned
-2044 for an existing PDSE member; retain those results independently.

Patch 0002 omits the unused TSO data-set-prefix parser `@@GETEPF`, its
`@@PCLST` RMODE24 table and parser-only setup. The five entry-call services
remain. Patch 0003 repairs `@@DYNAL`'s two SVC 99 pointer words: a high
relocated template address must not supply the high byte when the temporary
request arena is below 16 MiB. These changes preserve the original notices
and leave the separate upstream file intact. The exact native ASMA90 object,
classic XMITs, high-placement proof and application checks remain in the
private laboratory's dated implementation evidence.

IBM documents DS1LSTAR as unsuitable for PDSEs in the
[PDSE Usage Guide](https://www.redbooks.ibm.com/redbooks/pdfs/sg246106.pdf)
and [DSCB field description](https://www.ibm.com/docs/en/zos/3.2.0?topic=types-format-1-format-8-dscbs).
The working correction is a source candidate; the dated TSO report records
its actual guest checks. It is not a change to the separately preserved CMS
or historical MVS baselines, and has not been submitted upstream.

## z/OS 1.5 TSO24 service source

This is the canonical newlib SDK source patch for the z/OS 1.5 TSO24
`@@AOPEN` JFCB-token lookup. The laboratory's
`systems/tso15/mvssupa24-zos15.patch` is the earlier, byte-identical
historical receipt. The SDK copies this patch into
`adapters/pdpclib-patches/` and the pinned pristine PDPCLIB source into
`adapters/tso/mvssupa-upstream.asm`. The checked local SDK v22 native
`T24SUP` object predates this repair; its presence does not imply a patched
service object. The patch changes only the native TSO24 service source, not
the five C newlib adapter archives or the TSO31/64 service source.

On the pinned upstream `mvssupa.asm` (SHA-256
`27758af986baae46fb726fe9bb35ed993898b98d547e315587328940c2f41e31`),
apply 0001 first. That yields SHA-256
`ab7fd659f8bde628c5c8a871ef40378841c1c61656d9b3067e92c02f6be3cc2a`.
Stage that file as `tso24/native/mvssupa.asm` and apply 0004 with `patch -p1
--fuzz=0`. The resulting source must be SHA-256
`50cad35c7e5fa4c88a2efcf4f8f3999948087edad828bc4d5b032b5f0a3bd3b1`.
For example, from an installed SDK and an empty private work directory:

```sh
SDK=/absolute/path/to/installed-sdk
mkdir -p work/pdpclib work/tso24/native
cp "$SDK/adapters/tso/mvssupa-upstream.asm" work/pdpclib/mvssupa.asm
(cd work && patch --batch --fuzz=0 -p1 \
  -i "$SDK/adapters/pdpclib-patches/0001-pdse-no-lstar-check.patch")
cp work/pdpclib/mvssupa.asm work/tso24/native/mvssupa.asm
(cd work && patch --batch --fuzz=0 -p1 \
  -i "$SDK/adapters/pdpclib-patches/0004-zos15-tso24-swareq.patch")
shasum -a 256 work/tso24/native/mvssupa.asm
```

Set `SDK` to the absolute installed SDK path. Assemble the resulting source with the
guest's ASMA90 and IBM macros, then bind its object into the TSO24 module.
The laboratory's 25 September qualification and 29 September text-switch
report record guest execution with this source. Native assembly and binding
remain separate from rebuilding the C adapters.
