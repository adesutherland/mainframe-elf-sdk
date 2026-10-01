# Source publication and producer smoke correction

Mainframe ELF SDK source was first published on 1 October 2026 at
`deb7772d098c171331f557541293c334f1e07a0d`. The 179-file inventory and fresh
extraction matched; ten heap-default cases, two CMS console/text host checks
and all 32 selected GCC source/recovery hashes passed. This publishes source,
not native bootstrap objects or a qualified binary release.

The first [hosted producer run](https://github.com/adesutherland/mainframe-elf-sdk/actions/runs/36823262883)
reconstructed the pinned source and built GCC and binutils on Linux arm64.
Its final historical-profile smoke failed during assembly annotation because
the workflow omitted the ordinary profile's unwind flags. GCC emitted
`.eh_frame` outside the checker's accepted historical assembly surface.

The same probe reproduced that failure with the retained host compiler.
The smoke also omitted the assembler's `-L` option, which retains the local
labels used to classify instruction and data ranges. The SDK consumer already
uses `-fno-asynchronous-unwind-tables -fno-unwind-tables` and `-L`; the workflow
now follows those conventions. GCC source, ISA checks and runtime contracts
are unchanged. The exact corrected workflow smoke passed locally using the
retained host compiler and the current checker: compile, annotation, assembly
and encoded object checks, covering four instructions, two inline data bytes
and two padding bytes. The corrected hosted run is separate qualification;
the first run's queued macOS job was cancelled as obsolete.
