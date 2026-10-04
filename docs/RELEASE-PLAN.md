# Release status, limits and next steps

**0.1.0 was released on 3 October 2026 for macOS arm64 and Linux x64.**
The [release page](https://github.com/adesutherland/mainframe-elf-sdk/releases/tag/v0.1.0)
contains both archives, `SHA256SUMS`, the source revision and host qualification
record. Windows remains planned for 0.1.1 after a native Windows build and
installed-package qualification.

## What the milestone establishes

The release was built from SDK revision
`f5a57fec88fe38678663c7015261131875bdfcde`, checked GCC 16.2.0, GNU binutils
2.47 and newlib 4.6.0.20260123 archives, and z/PDOS revision
`930dbbc13119ab69d6b4ddc4f42e4d6edcf6fd59`. Its host build recipes use the
pinned cREXX v1.0.0-beta.3 runtime.

The compiler, selected C libraries, adapters, native TSO objects and host
packagers were built without a z/OS assembler, binder, IBM macro library or
prebuilt target runtime archive. Both host packages passed an unpacked,
six-profile consumer outside the producer tree, including four complete TSO
XMIT links. The Linux release build is recorded in
[run 37137522440](https://github.com/adesutherland/mainframe-elf-sdk/actions/runs/37137522440).

| Output checked on a guest | Recorded result |
| --- | --- |
| CMS24 MODULE | Returned 42 on VM/370 Community Edition; transferred bytes read back identically |
| CMS31 MODULE | Returned 42 on CMS 20 under z/VM 4.4, using temporary 256 MiB guest storage; transferred bytes read back identically |
| TSO24, TSO31 and both TSO64 simple members | Restored and returned 42 on z/OS 1.5 |
| TSO31 and both TSO64 file members | Sequential write/read, PDS member read and missing-DD handling passed on z/OS 1.5 |

The corrected macOS package reproduced the guest-tested MODULE and TSO
transport bytes. Linux passed its own source build and installed consumer;
this record does not claim a separate Linux-output guest campaign. The
[3 October checkpoint](updates/2026-10-03-sdk-source-candidate.md) preserves
the intermediate failures, repairs, hashes and final comparisons. Its early
“not yet released” statements describe the time of those experiments.

## Known limits

| Area | 0.1.0 scope or outstanding issue |
| --- | --- |
| TSO24 files | Dataset I/O faults in above-the-line SWA lookup and is excluded. The simple entry/return path passed. |
| TSO31/64 files | The qualified subset is sequential write/read and PDS read. The selected service bypasses inherited NOTE/TRKCALC positioning; FBS extend/positioning is excluded. |
| Other native services | VSAM, IDCAMS and supervisor-mode switching are rejected by the selected source contract. Those rejection paths, dynamic allocation, command/prefix services and wider service behavior still need separate guest checks. |
| Control-block dependencies | Inherited PDPCLIB uses a `TCBFA` test that is not a designated IBM programming interface. The recorded z/OS 1.5 paths do not establish portability to other systems. |
| TSO64 code placement | Both shipped entry variants keep the image below 2 GiB. The separate application RMODE64/high-launcher route is excluded; modern z/OS high-code execution remains unverified. |
| C and runtime coverage | Selected C/newlib, math and software-arithmetic support; no complete C99, libc, floating-point or POSIX conformance claim. |
| CMS and kernel coverage | Modern z/CMS remains unqualified. `vmkernel` provides a component/export path, not a complete CMS/CP kernel or a 31/64-bit kernel ABI. |
| Host platforms | Released packages are macOS arm64 and Linux x64. An ARM Linux source-tool CI job is not an ARM Linux SDK release. |
| Descriptive metadata | Some profile JSON and manifest descriptions still reflect the bootstrap period; see [SDK-005](../sdk/doc/BACKLOG.md#sdk-005-profile-and-manifest-description-drift). |
| File example result | A PDS failure branch shares the final success code. Require all three success messages and RC 42; see [SDK-006](../sdk/doc/BACKLOG.md#sdk-006-file-example-success-and-failure-share-a-return-code). The recorded guest runs include those messages. |

These limits are part of the release description. Application experiments,
older bootstrap objects and broader z/PDOS source support do not silently
expand the released subset.

## Requirements for subsequent releases

Keep one maintained source tree per component and preserve the component
notices. Build the advertised profiles from checked source, record all input
identities, and qualify an unpacked package on each advertised host. Changes
to native bytes or service contracts require the affected guest checks;
unchanged results can retain their existing evidence.

For **Windows 0.1.1**, build native Windows executables from the same
maintained inputs, package the SDK, run the installed consumer and failure
controls on Windows, and document its dependencies before publication.

Wider file services, modern-system qualification and a generic high-code
launcher are separate work. Follow the [component backlogs](README.md#contributing)
for their observations and acceptance criteria. A source commit, a passing
host build, guest execution and a published release remain distinct milestones.
