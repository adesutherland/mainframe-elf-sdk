# Mainframe ELF C upstream

The compiler is based on the official GCC 16.2.0 source archive. Its SHA-256
is `e6738e29597f733270731aa90600f37ffdc045079dfc27ec7e8192cc81085c3e`.
That archive supplies unmodified GCC build infrastructure and source. The
maintained S/370 and S/390 files live in `src/gcc16/`; this directory is their
single editable home. `upstream-files.sha256` checks selected pristine inputs
before preparation, and `source.sha256` checks the maintained files copied
over them. The complete prepared tree is generated under ignored `build/`.

The current maintained files were reconciled from the initial nineteen-topic
Mainframe Lab recovery series. That historical series and its topic record are
frozen under `archive/recovery/`. Normal builds and required tests do not read
it. Later changes are made directly in `src/gcc16/` and recorded in Git history;
there is no generated or maintained patch stack. Preserve GCC file notices,
the GPLv3 terms in `LICENSE`, and the GCC runtime exception in the upstream
archive.
