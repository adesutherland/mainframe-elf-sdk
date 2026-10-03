# Source-built SDK candidate checkpoint — 3 October 2026

We have a local Apple Silicon macOS source-input SDK candidate with an
installed consumer pass for all six advertised profiles. It is a host build
and transport result. The changes in the SDK and z/PDOS `develop` checkouts
were uncommitted at this checkpoint, and there is no 0.1.0 tag or published
release. Linux and affected z/OS guest checks remain open.

## Native source route

The SDK's maintained TSO24, TSO31, TSO64 low and TSO64 AMODE64/RMODE ANY
entries assemble with z/PDOS Classic Assembler. The maintained PDPCLIB
`tso31-sdk-files` selection assembles with selected source-owned TSO
terminal, EXTRACT and control-block forms. This selection keeps sequential
and partitioned dataset paths; VSAM, IDCAMS and supervisor-mode switching
return unsupported status. The broader inherited `tso31-lean` path remains
separate. IBM macro source, ASMA90 objects and frozen upstream archives are
not native build inputs.

On this host, the service assembled 4,331 statements, 25 sections, 971
symbols and 55 fixups. Its joined source SHA-256 was
`09ff331cb4218032c6d3fda3e33d517ee3e6ef282eea490929ac3c7339600e05`;
the service object SHA-256 was
`313d8cd68070889decdf1770a568ad2deeeb075c2e54ed52022c54e7f75214a9`.
The independent TSO fixture checked SVC 6 terminal forms and the SVC 40
PSCB EXTRACT list. The z/PDOS host suite passed 93/93 at `-j 4`.

`sdk/scripts/build-tso-native.crexx` rebuilds the four entries and this
service with the source-built Classic Assembler, runs the native deck checks,
and links them to source-built TSO C decks with the Classic Linker. All four
TSO24, TSO31, TSO64 low-entry and TSO64 AMODE64 entry XMIT outputs linked.
Their XMIT SHA-256 values were respectively
`b4364822bbaf89316059174549d6a3a0d65364f6e1172dcdd7d0a44ebf4e5739`,
`ee378c2f6afaf386ae8522f11924c2341e1793acfc4237b365473a41717172c2`,
`a19034372e8e99e06bf50b9cdce26aa292e2bae878e83f5a71d508d0187413de`,
and `47482a1d207e3b80956109d0cde518b2c56447135562d54fb5e6a24ae1007c`.
The generic SDK uses the TSO64 AMODE64/RMODE ANY route; the application
low launchers for high-code placement stay in Mainframe Lab.

## Package and installed consumer

`sdk/scripts/compiler_sdk_source_package.py` copied the five source-built
sysroots, source-built host GCC/binutils and Classic tools, source-built
native objects, profiles, layouts, adapters, checkers and notices into a
local candidate. It takes no runtime archive or prebuilt native-deck argument.
The final local package recipe also copies SDK and z/PDOS component licence
and provenance records into `notices/`, alongside the sysroot runtime notices.
The first candidate manifest recorded 1,641 installed files and no
bootstrap-input fields. That 67 MiB archive SHA-256 was
`3a62eda5f8a03a89804652afc4b6042a0bc927161e61444c7a1abc3077329615`.

After unpacking under `/tmp` outside the producer checkout, the installed
consumer verified the package manifest and passed CMS24/31 MODULE builds,
TSO24/31/64 C ELF deck builds, the `vmkernel` component, and complete TSO
native XMIT links using installed Classic Linker and native service objects.
The TSO64 installed consumer covered both generic entry modes. That first result
receipt SHA-256 was
`fcea10ab67f28b25a01e6dab88f97131af05ff154ac3f716e070667baa2e4050`.

## First guest transport check and correction

After committing z/PDOS `c156bdd` and SDK `b8cf649` on `develop`, I leased
LABA01 on the shared z/OS 1.5 guest. The first installed TSO64 XMIT was
transferred and read back byte-identically, but `RECEIVE` restored `/PRIVATE`:
Classic Linker derives the member name from the first eight characters of its
`-o` argument, which had been an absolute `/private/...` path. IEBCOPY
reported severity 0, but that name is unsuitable for a normal SDK member.
The SDK recipes now link from their output directory with explicit names
`SDKTS24`, `SDKTS31`, `SDKTS64L` and `SDKTS64A`. A host XMIT directory check
requires the intended name and AMODE/RMODE. The corrected TSO64 ANY XMIT SHA-256
was `35c3f85ce8dfab4ee113efad84025f34bae8ebf785d8848dd221c584baaa142f`;
its guest transfer read back identically, and `RECEIVE` restored `SDKTS64A`
with IEBCOPY severity 0. The correction is in SDK `95ff1d4` on `develop`.

The committed `sdk-member-smoke.rexx` then called each corrected member on
that guest. The four XMITs read back byte-identically, all restored with
IEBCOPY severity 0, and the REXX harness observed return code 42 for
`SDKTS24`, `SDKTS31`, `SDKTS64L` and `SDKTS64A`. The exact XMIT SHA-256 values
were `fd690fc910bcbb599d51d652835f96041a92cccdbb693456cd767d54e373e06b`,
`e1dfb4d98db88988e3e55293411ed40624d76db6c288fb7ff0adb77fd4d5bc81`,
`28a52770c36b048f522970140cb2e9cd13e92808882f7aead6bccd49db949e21`
and `35c3f85ce8dfab4ee113efad84025f34bae8ebf785d8848dd221c584baaa142f`.
These calls check entry, load and simple C return; file paths and larger
applications still need separate guest exercises.

## Release limits

The four simple guest calls do not prove all selected z/OS 1.5 macro offsets,
SVC linkage, dataset behavior or the full IARV64 surface. The `TCBFA` test
in inherited PDPCLIB is not a designated IBM programming interface. No
Linux host package, clean-checkout build or guest file operation has passed
for these changed native bytes. The source-only candidate is therefore
reviewable but not a qualified 0.1.0 release. Windows remains the planned
0.1.1 host stage.
