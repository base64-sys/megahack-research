# Runtime validation handoff (2026-10-27)

- **Objective** - after a full repository rescan in a new chat, resume the Mega Hack
  runtime-validation work from [runtime-validation.md](runtime-validation.md). Keep the
  static reference (Mega Hack v9.1.3 / GD 2.2081 / Geode 5.6.1) and the runtime
  re-baseline (Geode 5.11.2, profile `runtime511`). Do not modify the original 80-mod
  profile. Build the `megahack.trace` observer, runner and comparator, then validate a
  pilot cluster.

## Current state

- Step 1 is complete: the trace contract, scenario manifest, comparison rules and M3
  evidence checklist are in [runtime-validation.md](runtime-validation.md).
- The runtime profile `runtime511` exists at
  `C:\Users\Robert_\Documents\GD-Runtime-511`. It is a copy of the installed game with
  the Geode 5.11.2 loader and only `geode.node-ids` installed.
- The original default profile `a` is untouched: 80 `.geode` packages and the same
  `Geode.dll` SHA-256 (`0C34F50454C102637038CAE48C8C7DD685DA3582FE474358024B1EB6833684D3`).
- The copied `GeometryDash.exe` SHA-256 is
  `FC5A16C292278BC2E8E078FB1D5023C2BD658322DD72712767EA70C2DD9EC6D0`.
- At the time of this handoff no harness, runner or comparator existed, no trace had been
  captured, and `runtime511` had not been launched. **All three are now false** - see
  "Superseded later on 2026-10-27" below.
- No production code, reconstruction code or original DLL was modified. `runtime-traces/`
  is gitignored; no trace files exist.

**Superseded later on 2026-10-27.** The observer (`megahack.trace` 0.2.0), the runner
tools in `harness/tools/` and the comparator exist and have been run: four bounded
no-input captures (two per variant) compare `pass` with no unstable and no failing row,
and the reconstruction loaded in Geode and installed the same six hooks the observer
watches. The claim is still `surface` because no run reaches a site only the target can
produce, so all five clusters stay structural-only - what those two bullets call missing
tooling is now a missing *scenario*. Current record:
[deterministic-capture.md](deterministic-capture.md), "Result"; observer detail:
[trace-observer.md](trace-observer.md).

## Read first in the new chat

1. `AGENTS.md`, `docs/GOALS.md`, `docs/ROADMAP.md`.
2. `notes/runtime-validation.md` (the contract and baseline decision).
3. `notes/m3-validation.md` (all five clusters remain structural-only).
4. `notes/hook-dispatch.md`, `notes/hook-registrations.md`,
   `notes/feature-inventory.md`.
5. `notes/persistence.md`, `notes/save-lifecycle.md` (no file I/O at the hook boundary).
6. Current `git status` and `git diff`; do not overwrite unrelated work.

## Environment facts

- Repository: `C:\Users\Robert_\Documents\megahack-research-new-codebase`, branch
  `migration/curated-codebase`.
- Python: `C:\Users\Robert_\AppData\Local\Python\bin\python3.exe` (3.14.6).
- `GEODE_SDK=C:\Users\Robert_\Documents\Geode`; that checkout is v5.11.1.
- Installed runtime loader: Geode 5.11.2 (both profiles).
- `geode profile list` shows `a` as default and `runtime511` as the isolated profile.
- REA/Ghidra 12.1.2 available; Frida not usable in this setup.

## Next sequence

1. Rescan the repository and reconcile findings with the contract. Record drift; do not
   silently rewrite evidence.
2. Implement the host-testable trace core (value model, JSONL writer, call pairing).
3. Implement `megahack.trace` against the matching 5.11.x SDK and install it only into
   `runtime511` with one target and `geode.node-ids`.
4. Implement the runner (hashes, run config, one-target enforcement, trace sidecars) and
   the comparator with synthetic tests for null vs unavailable, missing/extra, crashes,
   ordering and tolerances.
5. Pilot an input/gameplay scenario first. Treat `safe-mode-smoke` as draft until the
   `savePercentage` trigger and external file observation are established.
6. Promote a cluster in [m3-validation.md](m3-validation.md) only on a `pass` with the
   required trace hashes. Do not close the M3 criterion on a partial or blocked result.

## Open questions

- Exact multi-hook ordering/tie-break and whether the observer can read the target's hook
  priority.
- Frame counter, main-thread identity and synthetic-input APIs in 5.11.2.
- A deterministic local `.gmd` fixture and the trigger for `savePercentage`.
- Resolver strategy for the 30 name-resolved hooks over 26 methods.
- Whether target callback internals are observable without reimplementing the release's
  wrapper.
