# Maintained GCC target source

This repository carries the selected editable changes to GCC 16.2.0 under
`toolchain/gcc16/source/`. The complete compiler comes from the [official GCC
16.2.0 archive](https://ftp.gnu.org/gnu/gcc/gcc-16.2.0/gcc-16.2.0.tar.xz),
SHA-256 `e6738e29597f733270731aa90600f37ffdc045079dfc27ec7e8192cc81085c3e`.
That release archive, not a guessed upstream Git commit, is the source base.
`inputs.sha256` locks its selected pristine files; `source.sha256` locks the
current editable files. The GCC notices and GPLv3 terms remain in force.

The nineteen ordered patches in `patches/gcc16/maintained/` reconstruct the
initial extracted SDK source. They are a checked baseline imported with this
repository, not commits that need to exist in its new Git history. New changes
begin as focused source commits in **this repository**. The recovery patch is
generated from that commit; never edit a recovery patch by hand.

1. Start from a passing source tree and a clean Git worktree. Edit only
   `toolchain/gcc16/source/` for one focused topic, review it, and commit it.
2. Run `python3 tools/compiler_sdk_recovery.py --topic short-name`. It requires
   that HEAD is the one source commit and its parent matches the previous
   `source.sha256`. It generates the next numbered patch from the Git diff,
   refreshes `series`, `patches.sha256`, `source.sha256`, the explicit source
   allowlist and `SOURCE-MANIFEST.json`, and records the source commit in
   `recovery-topics.json`.
3. Review the generated diff and inventories, then replay in a **new** work
   directory: `python3 tools/compiler_sdk.py prepare --cache /path/to/checked-archives
   --work /path/to/new-replay`. This applies every patch with zero fuzz and
   compares each recovered file with the editable source. Commit the generated
   recovery files after that check. A compiler build and profile QA are needed
   when the actual compiler behavior changes; the replay is a source check.

The helper refuses a dirty worktree, non-source topic, changed prior checkpoint,
binary patch and untracked selected source. A failed run leaves reviewable
generated files; repair the source topic or restore only the generated files
before retrying. The standalone `prepare` command remains the authority for
patch application and source identity.

Only the files needed for these changes are imported. Other machine
descriptions, compiler internals, configure and build infrastructure come from
the locked archive. This is not an upstream GCC release, a new target triplet,
or a claim that CMS can host a GCC build.
