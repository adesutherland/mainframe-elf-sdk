# Source-built SDK candidate checkpoint — 3 October 2026

Reading this after publication: [0.1.0 was released later on 3 October](https://github.com/adesutherland/mainframe-elf-sdk/releases/tag/v0.1.0).
The sequence below preserves the observations and open gates at each
checkpoint. Use the [current release record](../RELEASE-PLAN.md) for the final
host assets, guest scope and remaining limits.

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

## Final source-built file service and guest result

The selected SAM/BPAM DCB macro initially omitted unopened access-method
placeholders and DCBOFLGS initialization. The corrected z/PDOS source supplies
the 88-byte template expected by OPEN. After OPEN, inherited NOTE/TRKCALC
positioning abended in the selected file profile; that branch is now bypassed
for `tso31-sdk-files`. This bounds the 0.1.0 file subset to sequential
write/read and PDS read without FBS extend or positioning. The final service
object SHA-256 is
`58efd091aa78765cc4b0f9b00002c96c181bb4be353022c40b0e69b1d783e5c3`.
The z/PDOS host suite passed 93/93 with these source bytes, committed as
`930dbbc13119ab69d6b4ddc4f42e4d6edcf6fd59` on `develop`.

The macOS source-built candidate archive SHA-256 is
`cbfe66ce728f9543e31b729578d5af07524ffa8ff2171442a0325e3b0898e3af`.
Its independent installed consumer passed all six advertised host profiles.
Using that installed package, `SDKF31`, `SDKF64A` and `SDKF64L` were compiled,
linked and restored on the leased z/OS 1.5 LABA01 account. Each printed
`SDK SEQUENTIAL PASS`, `SDK PDS PASS`, `SDK MISSING DD PASS` and returned RC 42.
The input XMIT SHA-256 values were, respectively,
`f82f47c5cc339a65cb1ade6bd108bf8a11ff51b5580155b86fbcd7b6dcc8f43d`,
`496a27c2e66f5ce52583145e27eed936f13877af208b37b732dd0546df251234`
and `ec2c291d3feabd0a1c38ff50cb35a7efd4c782ec647ecc1b062451494c6e6579`.
The same candidate's `SDKTS24` XMIT
(`5f2444fe9bc9e9515c35d22e262aab526beb56a0e860b6a861a617549bd8bb6a`)
restored and returned RC 42. Private guest transfer, RECEIVE and execution
receipts are under `/Users/adrian/MainframeLab/private/sdk-elf-20261003/`.

The TSO24 file smoke faulted during above-line SWA lookup and is outside the
0.1.0 file subset. Basic TSO24 entry and return passed with the final service.
The selected service rejects VSAM/IDCAMS and supervisor switching by source
contract, but their rejection paths, dynamic allocation, command and prefix
services still need separate guest checks. The file result is specific to the
tested z/OS 1.5 guest and the named entry and service objects.

The first complete Linux x64 workflow at SDK `126bfb7` built the cross tools
and five runtime sysroots, then stopped before C-deck packaging. The host
cREXX driver had left generated `*.crexx-driver.lock` files beside source
scripts; the strict source inventory treated them as new maintained files.
The inventory now excludes only that transient lock suffix while still
rejecting other unlisted files. A fresh local source extraction with a lock
present passed source package creation, and an unrelated extra file was
correctly rejected.

## Checked-source host and CMS completion

At SDK `f6dcb4702b5c51a1a43150943debecb4902c3dc8` and z/PDOS
`930dbbc13119ab69d6b4ddc4f42e4d6edcf6fd59`, hosted run
[37132921683](https://github.com/adesutherland/mainframe-elf-sdk/actions/runs/37132921683)
passed all three jobs: the full Linux x64 source-built package and unpacked
six-profile consumer, plus macOS and Linux ARM source-tool builds. The Linux
candidate archive SHA-256 is
`9932aae71a18b92469ab49042a93b90beaffa503b091d8f1f227a1d099f40e3b`.
Its manifest records source-only inputs, all six profiles and the same
source-built native TSO object hashes as the macOS package.

The first committed macOS candidate passed its installed consumer, but a
cross-host package comparison found that its copied TSO `native/entry.asm`
files came from an older sysroot. We rebuilt all five macOS sysroots from the
exact checked source and repackaged them. The corrected macOS candidate
archive SHA-256 is
`f78e53651f2755a8c3d6dc84c6e7985f4e73a29435e44b23be7214adabbed8b2`;
its unpacked six-profile consumer passed. The three TSO entry source copies
match the maintained assembly byte for byte. The source-package recipe now
rejects a stale TSO entry source in any supplied sysroot.

The Mainframe Operator transferred the earlier macOS candidate's CMS MODULEs
through the shared tape procedure. CMS24 returned native RC 42 on VM/370 CE;
CMS31 returned native RC 42 on z/VM 4.4 with temporary 256 MiB storage.
Guest tape readback was byte-identical for both; the operator removed the
test files, restored CMS31's original 64 MiB setting, logged off and released
all acquired leases. The exact MODULE SHA-256 values were
`0ae1e1b96fe0ea00e4b388c6cb826655f3dd4e65a031ce1d87d2725b2995b8c9`
and `681dd9ab7b77e95deccbebcc4cbf5cba3bfddf55954482c7a36924ce6429aa30`.
The corrected macOS package's installed consumer reproduced those MODULEs
byte for byte. Its four simple TSO XMITs and three TSO31/64 file-smoke XMITs
also match the corresponding z/OS 1.5 guest-tested bytes exactly. Private
operator receipts are under
`/Users/adrian/MainframeLab/private/sdk-elf-20261003/operator/`.

## Release limits

The guest calls do not prove all selected z/OS 1.5 macro offsets, SVC linkage,
dataset behavior or the full IARV64 surface. The `TCBFA` test
in inherited PDPCLIB is not a designated IBM programming interface. The
selected 0.1.0 subset has host and guest evidence for the paths above; the
release tag and asset publication still need separate approval. Windows
remains the planned 0.1.1 host stage.
