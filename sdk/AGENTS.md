# SDK host and package agent guide

Read the root `AGENTS.md`, this component's README, UPSTREAM.md, LICENSE and
doc/BACKLOG.md before editing. Keep host implementation and profile contracts
in `src/`, and maintained orchestration in `scripts/`. The optional `archive/`
is frozen reference material; do not update it or use it in a source-only
release route. Preserve the old bootstrap packager's limits in its output.

Use Git history for changes to current source. Record release and installed
consumer gaps in doc/BACKLOG.md. A host helper build, installed consumer, guest
run and published SDK are distinct results. Do not commit, push, tag or publish
without explicit session authorisation.
