# PDPCLIB TSO candidate

The pinned source is PDOS revision
`0fe81209e78d022b40301f86f97c7f4d3e406d0a`. MVSSUPA retains Paul Edwards's
public-domain notice and the original contributor history, including Gerhard
Postpischil. The vendor file is unchanged. The laboratory's retained baseline
used a checked PDSE-patched copy. The TSO31 high-residence candidate uses
`tools/tso31_native_source.py --upstream vendor/pdos/pdpclib/mvssupa.asm
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
