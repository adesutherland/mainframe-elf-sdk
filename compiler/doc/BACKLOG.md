# Mainframe ELF C backlog

This is the live compiler queue. Record observations, affected profiles and
acceptance evidence here; dated reports own exact results.

## ELFC-001: Direct-source producer qualification

- Type: qualification
- Status: In progress
- Observation: The producer previously replayed nineteen recovery patches.
- Target: GCC 16.2.0 host cross-compiler for all active SDK profiles.
- Evidence: On 3 October 2026 a fresh direct-source preparation matched the prior patch-prepared GCC tree byte for byte, the Apple Silicon host build passed, and the historical profile object smoke passed. Linux host and broader profile matrix checks remain open.
- Acceptance: A fresh producer copies only checked maintained files from
  `src/gcc16/`, reproduces the old prepared GCC files exactly, builds the
  compiler, and passes the affected profile matrix on macOS and Linux.
