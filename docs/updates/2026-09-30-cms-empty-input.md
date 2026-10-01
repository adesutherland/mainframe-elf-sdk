# CMS20 empty console input

30 September 2026. The CMS20 adapter now explicitly requests `WAIT=YES` on
LINERD. A reduced native probe and the full cREXX four-input application
check have passed. This is a source repair and local guest proof, not a
published SDK package.

## Cause and change

The earlier adapter supplied flag bytes `ce 40`: mixed-case logical input,
blank padding, stack input and attention restart, with the default WAIT=NO.
At a visible input prompt, a null Enter changed RUNNING to VM READ without
completing the read; another Enter delivered the empty line. Waiting for
VM READ did not help because that status was not issued before input.

IBM's [LINERD reference](https://www.ibm.com/docs/en/zvm/7.3.0?topic=instructions-linerd)
defines WAIT=NO as RUNNING in line mode and WAIT=YES as VM READ. Its usage
notes distinguish the attention interrupt caused by Enter with WAIT=NO.
The [command-environment guide](https://www.ibm.com/docs/en/zvm/7.4.0?topic=introduction-using-commands)
also describes a null Enter under RUNNING as an environment request.

The running CMS20 DMSGPI LINERD macro confirms `OI 37(1),B'10000000'` for
WAIT=YES and `OI 37(1),B'01000000'` for ATTREST=YES. The adapter changes only
the second flag byte from `40` to `c0`. It retains the native parameter list,
stack handling, mixed case, logical input, blank padding and attention
restart. The historical WAITRD branch is unchanged. No extra Enter,
space trimming or terminal-setting workaround implements the fix.

## Checks

The actual production adapter passed a host service callback check of the
LINERD command, flags, fields and fence. A successful zero-character return
sets the caller's length to zero in both text-conversion states. Service
errors and invalid input length remain observable. An otherwise identical
old `40` source control failed the required WAIT flag check.

The Lab retained `qualification/build-cms-input.crexx` rebuilt this adapter
against the existing compiler/core, copied the retained CMS31 libcms archive
into a distinct candidate directory, and replaced only its adapter member.
It linked `tests/cms-newlib/plain-empty-input.c` as IOEMPTY. Native MODULE
verification and the one-member record-aware tape preparation passed. The
original frozen archive was not overwritten.

On the leased CMS20 account under z/VM 4.4, IOEMPTY showed its prompt and
VM READ before input. Exactly one empty Enter returned
`CMS EMPTY RESULT: RC=0 LENGTH=0`, then `CMS EMPTY INPUT PASS` and a successful
CMS return. The application touched no files or guest settings.

The Lab receipts are in the private `crexx-io-cms-fix-20260930` directory:
`cms31-macro-IOQUAL.screens`, `input-build.log` and
`cms31-probe-IOQUAL.screens`. The final rebuilt CMS31 RXVM uses a 64 MiB heap,
a 3 MiB entry stack and the repaired WAIT adapter. Its supplied qualification
bytecode returned 8 PASS, 0 FAIL and
3 contractual SKIP in automatic mode. The full terminal check observed
VM READ and an unlocked keyboard before each input, then issued one Enter
each for `Mixed café!`, the empty string, two spaces before and after
`padded`, and `é`. Their lengths were exactly 11, 0, 10 and 1. It returned
9 PASS, 0 FAIL and 2 contractual SKIP with native RC0. The empty string
required exactly one Enter. These receipts are
`final/cms31-auto-IOQUAL.screens` and
`final/cms31-interactive-IOQUAL.screens`.

The final compiler, default assembler and fresh guest-built IOGUE3 bytecode
also passed their native chain, with 8 PASS, 0 FAIL and 3 contractual SKIP.
That separate application/resource qualification is recorded in the Lab's
`qualification/cms31.md` and `final/cms31-chain-IOGUE3.screens`. General SDK archive
regeneration, historical CMS input and wider terminal modes were not tested
by this bounded repair.
