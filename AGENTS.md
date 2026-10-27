# AGENTS.md

Working rules for this repository. The *what* is in [docs/GOALS.md](docs/GOALS.md); this
file is the *how*, and it is deliberately about the rules that get broken.

## Ground truth

- **`samples/absolllute.hackmega/absolllute.hackmega_original.dll` is the only ground
  truth.** 8,697,056 bytes, SHA-256 `1635c339…e1107a92`, image base `0x180000000`. Check the
  digest before a baseline-sensitive claim; it is cheap and it has caught swaps before.
- `_pre_persistence`, `_working_aslr` and any generated `.dll` are **experiments**. Quoting
  them as evidence about the release is a bug, even when they agree.
- Host-side claims may use a **GD 2.2081 image** (`GeometryDash.exe`, base `0x140000000`,
  SHA-256 `FC5A16C2…C6D0`) read-only, for function boundaries and structure layout. A claim
  from the host image is evidence about GD, not about Mega Hack, and the reverse holds too.
- Baseline: Mega Hack v9.1.3 / GD 2.2081 / Geode 5.6.1 static reference. The runtime
  environment is Geode 5.11.2 in the isolated `runtime511` profile; never mix the two in one
  claim. A version bump is a re-baseline with a diffed offset map, not a silent update.

## Evidence

- **Evidence or it does not exist.** Every address, offset, size and field carries its
  source: file, VA, RVA, and the tool that produced it.
- **Verify twice.** Two independent angles — a string xref *and* a call-graph path, a
  registration argument *and* the host `.pdata` boundary, disassembly *and* runtime
  observation. One angle is a lead, not a fact.
- **Label uncertainty, never launder it.** Write `UNVERIFIED: <what would close it>`. Do not
  present a plausible reconstruction as established, and do not quietly promote it later.
- **Do not trust the decompiler's** types, names, struct sizes or function extents. Confirm
  widths and displacements against instruction bytes, and state the base. Two "different
  fields" in this repo's history were the same field read from two bases.
- **Never convert an RVA to a file offset by arithmetic.** Use `scripts/pe_rva.py`, which
  distinguishes backed, zero-fill, gap and unmapped. `pefile.get_offset_from_rva` has the
  same bug: past `SizeOfRawData` it returns an offset inside a *later* section.
- **Absence needs a boundary.** "Not found" is a result only with the search that bounds it.
  Record the query and the scope, or do not record the negative.

## Legal and ethical lines

- Do not reverse, bypass or work around licence checks, DRM or entitlement checks.
- Do not add patches that improve gameplay or help in ranked or online play. Reproduce
  behaviour; do not improve it.
- Do not paste Absolllute's assets, artwork, branding or shipped text into new code.
- Do not produce a redistributable Mega Hack, and never commit a binary, asset or user data.
- If a request crosses one of these lines, say so plainly and offer the
  interoperability/preservation work you *can* do instead.

## Environment

- Python: `C:\Users\Robert_\AppData\Local\Python\bin\python3.exe` (3.14.6). `python` on PATH
  is a Store stub. Run `scripts/env_check.py` for a one-pass readiness report.
- Tracked tools (`scripts/`) run from the repository root and must not need `samples/` or
  `recovered/`. Sample tools (`samples/.../tools/`) do `sys.path.insert(0, ".analysis_tools")`
  and therefore run from `samples/absolllute.hackmega/`.
- `samples/` is evidence: read it, never edit it. Scratch work goes elsewhere; quote what
  you need into a note.
- Ghidra 12.1.2 headless drives `scripts/decompile_all.py`; the decompiled tree in
  `recovered/functions/` is gitignored and reusable — read it before re-running analysis.
- **REA MCP** (Ghidra/Rizin/provider bridge) is the second analysis engine. One import costs
  ~6 minutes, so: ask whole-program questions first (`list_procedures`, `list_strings`),
  batch per-address work in one session, `export_evidence_bundle` before you finish, and
  `close_binary(snapshot_path=recovered/…)` so the next session skips the import. Treat
  Evidence IDs from a previous session as dead: verify with `get_evidence_bundle`, do not
  cite a result you cannot re-open.
- `scripts/run_supervised.ps1` wraps anything that may exceed ~60 s and reports a stall.
  Never pipe long output through `Select-Object -Last`: it hides progress and turns a slow
  run into an indistinguishable hang.

## Route the question

| Question | First tool | Then |
| --- | --- | --- |
| Which code mentions this string or feature? | `scripts/build_xref_index.py --grep` | `notes/finding-code.md` |
| Which hooks does the release install? | `scripts/hook_sites.py` | `_hook_sites.tsv`, [hook-dispatch.md](notes/hook-dispatch.md) |
| Is this hook target real? | `.pdata` boundary check ([hook-target-verification.md](notes/hook-target-verification.md)) | `_wrappers.tsv`, REA procedure list |
| Which tag drives this callback? | `scripts/tag_callback_map.py` | `scripts/callback_hooks.py`, [hook-registrations.md](notes/hook-registrations.md) |
| Which settings tags does a function read? | `scripts/settings_index.py` | [settings-registry.md](notes/settings-registry.md) |
| Who calls this function, and how big is it? | `scripts/build_callgraph.py --callers-of`, `scripts/func_bounds.py` | REA `procedure_callers` |
| Raw instruction truth at an address | `tools/pe_disasm.py` | REA `read_function_instructions` |
| Which host method is behind this RVA? | `scripts/host_methods.py` | [host-methods.md](notes/host-methods.md) |
| What is stored, and when? | [persistence.md](notes/persistence.md) | [save-lifecycle.md](notes/save-lifecycle.md) |
| Which changelog rows are accounted for? | [feature-inventory.md](notes/feature-inventory.md) | [m3-validation.md](notes/m3-validation.md) |

`notes/address-map.md` is the index of indexes. Search the notes before starting a new
investigation; there is probably already a subsystem note.

## Writing notes

- `notes/<subsystem>.md` is durable output: **question, method, evidence, conclusion, open
  questions**. A conclusion without a method is not a note.
- Prefer tables of addresses over prose when the content is a list.
- Record what failed and why; a ruled-out approach saves the next investigation an hour.
- `references/` is for external material (Geode, GD, format specs), cited and dated.
- Promote a finding out of `samples/.../RE*_NOTES*.md` into `notes/` once it is
  independently re-derived from `_original.dll`. Sample notes describe a patch's history;
  they are not the project record.
- Correct contradictions in place. Two versions of the truth is worse than one wrong one.

## Writing code

- Original code only. Decompiled output is a reference; it is never committed verbatim.
- Every derived binary must be regenerable byte-for-byte from a tracked script, and the
  builder must be idempotent: running it twice gives the same bytes.
- No absolute VAs in injected code. Mutable state goes in the documented writable block at
  `0x18081BFC0`, never in `.text` and never at an absolute address.
- Hook sites replace whole instructions only; never split an instruction.
- Win64: reserve shadow space and 16-byte align RSP before every call. A JMP-entered
  wrapper inherits the original's stack alignment and must reproduce its exact epilogue.
- The reconstruction lives in `reconstruction/` and is host-testable; the runtime observer
  lives in `harness/` and is a third mod that reuses neither mod id.

## Verification gates

Run these before you claim work is done or push. Everything here is tracked and needs no
sample set unless marked.

| When | Command |
| --- | --- |
| Any change | `python scripts/check_doc_links.py` |
| Any change to the tracker | `python scripts/make_progress_svg.py` then `--check` |
| Any change to `scripts/` | the matching `scripts/test_*.py` (all of them is ~20 s) |
| Any claim about the release | `python scripts/check_inventory_evidence.py` |
| Touching binary, geometry or state | `validate_persistence_patch.py` and `test_best_runs_db.py` from `samples/absolllute.hackmega/` |
| Environment doubt | `python scripts/env_check.py` |

A red gate is a stop, not a warning. If a gate cannot run because `samples/` is absent, say
so; do not report it as passing.

## Long-running commands

- `scripts/run_supervised.ps1` wraps anything that may exceed ~60 s and reports a stall.
- Never pipe long output through `Select-Object -Last`/`-First`: it buffers everything until
  the pipeline ends, which makes a hang indistinguishable from a slow run.
- If the wrapper says `KILLED`, do not rerun. Read the log tails, say which step stalled, and
  diagnose before retrying. A killed wrapper does not always kill the work it started.
- A tool that legitimately runs silently for minutes needs `-MaxSilentSeconds` raised, not a
  rerun.
- The REA provider's own timeout is `REA_GHIDRA_STARTUP_TIMEOUT_MS=540000` in the OpenCode
  config; a cold import legitimately takes ~6 minutes.

## Notifications

- Arm the watchdog **before** substantial work, because a turn can die mid-flight and
  anything sent at the end never arrives:
  `powershell -File scripts\watchdog.ps1 -Action Start -Label "<short task>"`, then
  `scripts/notify.ps1 -Kind Done …` and `-Action Stop` before the final response.
- All messages go through `scripts/notify.ps1` (never raw `curl`) so every message carries a
  monotonic `#run.seq`. Kinds: `Armed`, `Heartbeat`, `Done`, `Question`, `Note`.
- The topic is public. Send a status word or one-line outcome. Never send paths, hashes,
  command output, credentials, or anything from `samples/`.
- Ask a `Question` only when the answer changes what to do next and only the user knows it.
  Ask once; keep working on the branch that does not depend on it.
- Do not leave a watchdog armed when going idle.
- `scripts/keepalive.ps1` is the older idle re-prompt; the `opencode-loop-plugin`
  (`/loop <interval> <instruction>`) supersedes it for long autonomous stretches.

## Before you commit

- `git status`: nothing from `samples/` or `recovered/`, no binaries, no scratch files.
- Does every new claim have an address and a tool? Does anything contradict a note — and did
  you correct the note rather than add a second version?
- Commit subjects are Conventional Commits. One commit per coherent change; do not sweep
  unrelated dirty files into it.
- Update [docs/ROADMAP.md](docs/ROADMAP.md) only when a milestone's state actually changes,
  and only against its exit criteria.
