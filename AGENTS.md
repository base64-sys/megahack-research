# AGENTS.md

This repo reconstructs Mega Hack v9.1.3 from its shipped binary. Mission and scope are in
[docs/GOALS.md](docs/GOALS.md); the tracker is [docs/ROADMAP.md](docs/ROADMAP.md). The rules
below are the ones that get broken, not the ones already obvious.

## Ground rules

- **Evidence or it does not exist.** Every address, offset, size and struct field carries
  its source: file, VA, RVA, and the tool that produced it. A claim without that trail is a
  guess.
- **Verify twice.** Confirm from two independent angles — a string xref *and* a call-graph
  path, or disassembly *and* runtime observation. One angle is a lead, not a fact.
- **Label uncertainty, never launder it.** Write `UNVERIFIED: <what would confirm it>`.
  Never present a plausible reconstruction as established, and never quietly promote it
  later.
- **Ground truth is `_original.dll`.** `_pre_persistence`, `_working_aslr` and the
  generated `.dll` are experiments. Quoting them as evidence about the release is a bug,
  even when they agree.
- **Never commit binaries.** `samples/` is untracked by design. No `.dll`, `.so`, `.exe`,
  extracted asset, sprite, audio file or Geode package enters the index. Hashes are fine;
  the bytes are not.
- **Nothing tracked may require `samples/`.** Files under `docs/`, `notes/` and `scripts/`
  must stay useful on a fresh clone with no sample set present.

## Legal and ethical lines

- Do not reverse, bypass or work around licence checks, DRM, or entitlement checks.
- Do not add patches that improve gameplay or help gain an advantage in ranked or online
  play. The reconstruction reproduces behaviour; it does not improve it.
- Do not paste Absolllute's assets, artwork, branding or shipped text into new code.
- Do not produce a redistributable Mega Hack. This repo publishes understanding and original
  tooling.
- Treat a real game session, level data or player information as user data: do not commit it.
- If a request would cross one of these lines, say so plainly and offer the
  interoperability/preservation work you *can* do instead.

## Task completion notification

An agent turn can die abruptly — out of tokens, a dead tool call, an interrupted
session. Anything sent at the *end* of such a turn is simply never sent, so the signal
must not depend on the turn finishing.

**Arm the watchdog first, always, before any substantial work:**

```powershell
# 1. before starting  — this also sends the "Armed" ping, refreshes
#    docs/progress.svg from live repo state, and opens a new run index
powershell -File scripts\watchdog.ps1 -Action Start -Label "<short task>"

# 2. do the work

# 3. before the final response: send the real completion ping, then disarm
powershell -File scripts\notify.ps1 -Kind Done -Task "<task>" -Text "<one-line outcome>"
powershell -File scripts\watchdog.ps1 -Action Stop
```

`scripts\watchdog.ps1` launches a **detached** PowerShell process that pings every minute, so
it survives the agent's turn ending. `-Action Status` reports whether it is armed. Heartbeats
stop when you disarm, so **silence after a heartbeat means the agent stalled**.

Rules:

- **Send everything through `scripts\notify.ps1`, never raw curl.** It stamps every message
  with a monotonic `#run.seq` index and persists the counter, so the user can read the
  order of a session without checking the clock, and heartbeats and agent messages share
  one counter. Kinds: `Armed`, `Heartbeat`, `Done`, `Question`, `Note`.
- **The payload is public.** `ntfy.sh/ai_agent` is a topic on a public relay — anyone who
  subscribes to `ai_agent` can read what is posted. Send a status word or a one-line outcome
  only. Never send command output, file contents, credentials, tokens, binary paths, hashes,
  user data, or anything from `samples/`. If a summary would leak, send `"done"`.
- **`curl` is not `curl` on this machine.** The bare name is PowerShell's alias for
  `Invoke-WebRequest`, which rejects `-H` and `-d`. `notify.ps1` calls
  `C:\Windows\System32\curl.exe` by full path; if you ever need it directly, do the same.
- **Never block the task on it.** Keep `-m`, ignore the exit code, carry on. A failed
  notification is not a failed task.
- Dispatch silently just before the final response, and do not print the invocation in the
  reply. If it fails, say nothing unless the user asked about notifications.
- **A test ping is not a completion ping.** Verifying the transport does not tell the user
  the task finished. Send the real one at the end, or state plainly that you did not.
- Do not leave a watchdog armed when going idle. `-Action Stop` first, always.
- ntfy's delayed-message cancel was tried for this and is **not reliable** — a cancel
  returned success and the message still fired. That is why the watchdog stops by being
  stopped, rather than by cancelling a scheduled alarm.

### Asking a question by notification

The user reads these and comes back with an answer, so a question can unblock a long run
instead of waiting for the next message. Ask only when an answer changes what to do next and
only the user knows it.

```powershell
powershell -File scripts\notify.ps1 -Kind Question -Task "<task>" `
       -Text "One short question, plus the reason it blocks."
```

- One short question per message. This is read on a lock screen; it is not the place for
  context.
- Only for a real fork in the road: missing evidence, a permission question, a choice
  between valid approaches. Never ask what the code already answers.
- Ask once. If there is no answer by the next task, keep working on the branch that does not
  depend on it, and say in the reply which question is still open.
- Same payload rules as above: no paths, hashes, command output or `samples/` data.

## Long-running commands

- Anything that might run longer than ~60 s goes through `scripts\run_supervised.ps1`, which
  returns `EXITCODE`, `MONITOR` status, and log tails. Quick commands run directly.
- If `MONITOR` says `KILLED`: do **not** rerun. Read the log tails, say exactly which step
  stalled and your diagnosis, then propose a fix.
- Never pipe long-running output through `Select-Object -Last`/`-First`. It buffers
  everything until the pipeline ends, which makes a hang indistinguishable from a slow run
  and hides all progress. The wrapper returns tails itself.
- Before rerunning something that timed out, check whether the child process is still alive
  (`Get-Process`) and whether its output files are still growing. A killed wrapper does not
  always kill the work it started, and a second run can corrupt the first one's output.
- The wrapper treats "no output for `MaxSilentSeconds`" as a stall. A tool that legitimately
  works silently for minutes needs `-MaxSilentSeconds` raised, not a rerun.

## Investigating

- Run tools from the mod root: every script does `sys.path.insert(0, ".analysis_tools")`,
  which is relative to the cwd.
- `python` is a Store stub on this machine. Use
  `C:\Users\Robert_\AppData\Local\Python\bin\python3.exe` (3.14.6).
- Prefer the purpose-built tool over ad-hoc scripting: `pe_xrefs.py` for strings,
  `pe_disasm.py` for raw truth, `pe_caves.py` for space, `DumpDecompile.java` /
  `FindAsciiReferences.java` for structure.
- Never convert an RVA to a file offset by linear subtraction. Use `scripts/pe_rva.py`,
  which is tracked and dependency-free. `pefile.get_offset_from_rva` has the same bug: for an
  RVA past a section's `SizeOfRawData` it returns an offset inside a *later* section. Two
  wrong reads in this repo's history came from exactly that, so the rule is not optional.
- Do not edit anything under `samples/absolllute.hackmega/`. It is evidence. Keep scratch
  work somewhere else and quote what you need into a note.
- Do not trust the decompiler's types, variable names or structure sizes. Confirm sizes
  against `mov`/`lea` displacements and access patterns.
- When a field is inferred from one access, say so and list the alternative interpretation.

## Writing notes

- `notes/<subsystem>.md` is durable output: question, method, evidence, conclusion, open
  questions. Conclusions without method are not notes.
- `references/` is for external material — Geode and GD docs, format specifications,
  third-party research. Cite the source and date it was consulted.
- Promote a finding out of `samples/.../RE_NOTES_*.md` into `notes/` once it is
  independently re-derived from `_original.dll`. The RE notes describe one patch's history;
  they are not the project record.
- Record what you tried that failed, and why. A ruled-out approach saves the next
  investigation an hour.
- Prefer tables of addresses over prose when the content is a list.

## Writing code

- Original code only. No decompiler output is committed verbatim; use it as a reference and
  write the implementation yourself.
- Every derived binary must be regenerable byte-for-byte from a tracked script in
  `scripts/`. If you cannot regenerate it, you cannot ship it.
- The builder is idempotent. Running it twice must produce the same bytes.
- Do not hardcode absolute VAs in injected code. Reach image addresses RIP-relative or via
  same-image rel32. The absolute-preferred-base `.data` build crashed under ASLR and the
  mutable-`.text` build crashed at `DLL+0x28638E`; both failure modes are documented so they
  are not reintroduced.
- Hook sites replace whole instructions only. Never split an instruction.
- Win64 calling convention: reserve shadow space and align `RSP` before every call. A JMP
  entered wrapper inherits the original function's already-aligned stack and must reproduce
  its exact epilogue.
- Mutable state goes in the writable 48 bytes at `0x18081BFC0` (or a documented extension of
  that block), never in `.text`, never at an absolute address.
- Validate before pushing anything that changes geometry or state:
  `validate_persistence_patch.py` and `test_best_runs_db.py` both green.
- Scripts that only ever get used once do not go in `scripts/`. Investigate in scratch space
  and promote to `scripts/` when a second investigation reuses them.

## Working with others' work

- Baseline discipline: Mega Hack v9.1.3 / GD 2.2081 / Geode 5.6.1. A version bump is a
  deliberate re-baselining with a diffed offset map, not a silent mix of two layouts.
- Mega Hack is Absolllute's product, Geometry Dash is RobTop's, Geode is a separate project.
  Cite them as such; this repo claims no ownership and no licence over them.
- Absence of evidence is not permission to assume absence. If a feature is not found, record
  that the search was done and how.

## Before you commit

- `git status` — nothing from `samples/`, no binaries, no large generated output.
- Does every new claim have a VA and a tool?
- Does anything contradict an existing note? If so, correct the note rather than adding a
  second version of the truth.
- Commit subjects follow Conventional Commits.
- Update [docs/ROADMAP.md](docs/ROADMAP.md) if you changed a milestone's state, and only
  against its exit criteria.