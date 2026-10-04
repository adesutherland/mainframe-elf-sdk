# Mainframe ELF C backlog

This is the live compiler queue. Record observations, affected profiles and
acceptance evidence here; dated reports own exact results.

## ELFC-001: Direct-source producer qualification

- Type: qualification
- Status: Done for 0.1.0
- Observation: The producer previously replayed nineteen recovery patches.
- Target: GCC 16.2.0 host cross-compiler for all active SDK profiles.
- Evidence: On 3 October 2026 fresh direct-source preparation matched the prior patch-prepared GCC tree byte for byte. The released macOS arm64 and Linux x64 packages passed independent installed consumers for all six profiles. The [release record](../../docs/RELEASE-PLAN.md) links the final source revision and Linux run. This closes the producer/profile-build criterion, not complete language or runtime qualification.
- Acceptance: A fresh producer copies only checked maintained files from
  `src/gcc16/`, reproduces the old prepared GCC files exactly, builds the
  compiler, and passes the affected profile matrix on macOS and Linux.
