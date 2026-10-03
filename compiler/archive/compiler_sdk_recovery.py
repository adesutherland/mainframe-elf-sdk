#!/usr/bin/env python3
"""Generate the next GCC recovery patch from one committed SDK source topic.

Run in the standalone public source repository after committing only edits to
toolchain/gcc16/source/. The initial nineteen patches are a checked baseline;
new topic identities come from this repository's own Git history.
"""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

from compiler_sdk import ROOT, refresh_manifest, sha

SOURCE_PREFIX = "toolchain/gcc16/source/"
PATCH_DIR = Path("patches/gcc16/maintained")
TOPICS = Path("toolchain/gcc16/recovery-topics.json")


def git(*args: str, binary: bool = False) -> str | bytes:
    result = subprocess.run(["git", *args], cwd=ROOT, check=True,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    return result.stdout if binary else result.stdout.decode()


def checked_topic() -> tuple[str, str, list[str]]:
    if Path(git("rev-parse", "--show-toplevel").strip()).resolve() != ROOT:
        raise ValueError("run in the standalone SDK Git repository")
    if git("status", "--porcelain", "--untracked-files=all").strip():
        raise ValueError("commit the focused source topic and start from a clean tree")
    head = git("rev-parse", "HEAD").strip()
    parent = git("rev-parse", "HEAD^").strip()
    names = git("diff", "--name-only", parent, head).splitlines()
    if not names or any(not name.startswith(SOURCE_PREFIX) for name in names):
        raise ValueError("HEAD must be one focused GCC selected-source topic")
    if any(re.search(r"\s", name) for name in names):
        raise ValueError("source topic paths must not contain whitespace")
    subprocess.run(["git", "diff", "--check", parent, head], cwd=ROOT, check=True)
    return parent, head, names


def check_previous_source(parent: str) -> None:
    """Confirm the checked recovery series names the committed parent bytes."""
    for line in (ROOT / "toolchain/gcc16/source.sha256").read_text().splitlines():
        digest, name = line.split(None, 1)
        path = SOURCE_PREFIX + name.strip()
        previous = git("show", f"{parent}:{path}", binary=True)
        if hashlib.sha256(previous).hexdigest() != digest:
            raise ValueError(f"parent source differs from recovery checkpoint: {path}")
    patch_dir = ROOT / PATCH_DIR
    series = (patch_dir / "series").read_text().splitlines()
    sums = {name.strip(): digest for digest, name in
            (line.split(None, 1) for line in (patch_dir / "patches.sha256").read_text().splitlines())}
    if set(sums) != {name.strip() for name in series}:
        raise ValueError("patch hash inventory differs from series")
    for name in series:
        if sha(patch_dir / name) != sums[name]:
            raise ValueError(f"recovery patch differs: {name}")


def source_diff(parent: str, head: str) -> bytes:
    raw = git("diff", "--no-ext-diff", "--no-renames", "--binary", "--full-index",
              parent, head, "--", SOURCE_PREFIX, binary=True)
    if b"GIT binary patch" in raw:
        raise ValueError("binary GCC source changes need a separate migration")
    prefix = SOURCE_PREFIX.encode()
    output = []
    for line in raw.splitlines(keepends=True):
        if line.startswith(b"diff --git "):
            match = re.fullmatch(rb"diff --git a/" + prefix + rb"([^ ]+) b/" +
                                 prefix + rb"([^\n]+)\n", line)
            if not match:
                raise ValueError(f"unexpected Git diff path: {line!r}")
            line = b"diff --git a/" + match[1] + b" b/" + match[2] + b"\n"
        elif line.startswith((b"--- a/", b"+++ b/")):
            marker = line[:6]
            if not line[6:].startswith(prefix):
                raise ValueError(f"unexpected Git diff file: {line!r}")
            line = marker + line[6 + len(prefix):]
        elif line.startswith((b"--- ", b"+++ ")) and line not in (
                b"--- /dev/null\n", b"+++ /dev/null\n"):
            raise ValueError(f"unexpected Git diff file: {line!r}")
        output.append(line)
    patch = b"".join(output)
    if not patch.startswith(b"diff --git ") or b"--- " not in patch:
        raise ValueError("source topic generated no text patch")
    return patch


def export(topic: str) -> None:
    if not re.fullmatch(r"[a-z][a-z0-9-]{1,47}", topic):
        raise ValueError("topic must be a short lowercase hyphenated name")
    parent, head, changed = checked_topic()
    check_previous_source(parent)
    patch = source_diff(parent, head)
    patch_dir = ROOT / PATCH_DIR
    series = (patch_dir / "series").read_text().splitlines()
    number = len(series) + 1
    name = f"{number:04d}-{topic}.patch"
    destination = patch_dir / name
    if destination.exists():
        raise ValueError(f"recovery patch already exists: {destination}")
    ledger_path = ROOT / TOPICS
    ledger = json.loads(ledger_path.read_text())
    if ledger.get("format") != "mainframe-compiler-sdk-recovery-topics-v1":
        raise ValueError("unrecognized recovery topic ledger")
    paths = sorted(p for p in (ROOT / SOURCE_PREFIX).rglob("*") if p.is_file())
    selected = {SOURCE_PREFIX + str(path.relative_to(ROOT / SOURCE_PREFIX)) for path in paths}
    tracked = set(git("ls-files", "--", SOURCE_PREFIX).splitlines())
    if selected != tracked:
        raise ValueError("selected GCC source files must all be committed")
    inventory = ROOT / "sdk/source-files.txt"
    current = [line for line in inventory.read_text().splitlines()
               if line and not line.startswith("#")]
    current = set(current) - {name for name in current if name.startswith(SOURCE_PREFIX)}
    current.update(selected)
    current.add(str(PATCH_DIR / name))
    ledger["topics"].append({"base_commit": parent, "source_commit": head,
                              "patch": str(PATCH_DIR / name),
                              "source_paths": changed})
    destination.write_bytes(patch)
    series.append(name)
    (patch_dir / "series").write_text("\n".join(series) + "\n")
    (patch_dir / "patches.sha256").write_text(
        "".join(f"{sha(patch_dir / item)}  {item}\n" for item in series))
    (ROOT / "toolchain/gcc16/source.sha256").write_text(
        "".join(f"{sha(path)}  {path.relative_to(ROOT / SOURCE_PREFIX)}\n"
                for path in paths))
    inventory.write_text("# Reviewed source-only compiler SDK extraction; paths relative to repository.\n"
                         + "\n".join(sorted(current)) + "\n")
    ledger_path.write_text(json.dumps(ledger, indent=2, sort_keys=True) + "\n")
    refresh_manifest()
    print(f"Generated {destination.relative_to(ROOT)} from {head}; run a fresh prepare replay")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--topic", required=True)
    args = parser.parse_args()
    export(args.topic)


if __name__ == "__main__":
    main()
