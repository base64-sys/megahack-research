# Goals

## Mission

Recover a faithful, source-level understanding of **Mega Hack** v9.1.3 from its shipped
x86-64 binary, and rebuild it as original, compilable C++ that is behaviourally equivalent
to the release — documented well enough that a new maintainer can change a feature without
guessing.

The deliverable is understanding and original code. The binary is evidence, never the
product.

## Why this is not "just run Ghidra"

The mod is a single ~8.7 MB PE with no symbols, no debug info and a stripped export table,
running on an undocumented host (Geometry Dash 2.2081) behind a loader (Geode 5.6.1). Three
things make this more than a decompile-and-paste job:

1. **The host ABI is the real subject.** Most of Mega Hack's behaviour lives in calls into
   Geometry Dash and Geode. Recovering *those* signatures and object layouts is worth more
   than the mod's own logic, and it is reusable by every other GD mod.
2. **Correctness is behavioural, not syntactic.** Decompiler output is a starting point.
   A reconstruction is correct when it matches the release's observable behaviour, which
   means every claim needs an evidence trail back to a VA and a disassembly.
3. **Scale.** Mega Hack ships 160+ feature toggles sharing a settings, menu and hook
   infrastructure. The infrastructure is the leverage; features are the bulk.

## Scope

**In scope**

- The Windows x86-64 binary `absolllute.hackmega_original.dll` (preferred base
  `0x180000000`) — the primary target.
- The Geometry Dash 2.2081 and Geode 5.6.1 host surface Mega Hack calls into.
- Mega Hack's own structures: settings registry, `nxUI` menu/interface layer, feature
  modules, save/load and lifecycle.
- The sidecar persistence format (`best_runs.bin`, magic `MHBRDB1\0`) and its importer.
- Patching and instrumentation, provided it is ASLR-safe and reproducible from a script.

**Out of scope**

- Android `.so` parity. Deferred until the x86-64 target is understood; ARM is a
  re-application of knowledge, not new knowledge.
- Cracking, licence checks or DRM circumvention. Mega Hack is a commercial product and this
  repository treats it as a black box supplied lawfully.
- Shipping or redistributing Absolllute's binaries. `samples/` stays untracked.
- Any patch intended to make gameplay easier or to gain an advantage in ranked play. The
  reconstruction must reproduce behaviour, not improve it.
- Reproducing Mega Hack's branding, artwork or shipped assets.

## Non-negotiables

These constrain every decision below and are not tradeable against velocity.

1. **Evidence over inference.** No struct offset, function signature or behaviour claim
   enters `notes/` or `docs/` without a file, a VA and the tool that produced it.
2. **Plausible is not verified.** Unproven reconstructions are labelled `UNVERIFIED` and
   never silently promoted.
3. **Original work only.** No Absolllute source is available and none is reconstructed by
   guessing at their intent. Describe behaviour; write your own implementation.
4. **Reproducibility.** A derived binary exists only if a tracked script regenerates it
   byte-for-byte, and a validator passes.
5. **ASLR safety is non-negotiable.** No absolute virtual addresses in injected code; all
   image addresses are reached RIP-relative or via same-image rel32. The absolute-base
   `.data` variant crashed under ASLR and must not return.
6. **Baseline discipline.** v9.1.3 / GD 2.2081 / Geode 5.6.1. A version bump is a
   deliberate re-baselining with a diffed offset map, never a silent mix.

## Success criteria

The project is complete when:

- Mega Hack's core infrastructure — entry, settings registry, menu layer, save lifecycle —
  is reconstructed as C++ that compiles and links against recovered host declarations.
- Every feature toggle is mapped to the code that implements it, with its default, its
  persistence behaviour and its interaction with other toggles.
- The Geometry Dash object layouts Mega Hack depends on are documented well enough that a
  third party could write a Geode mod against them without the binary.
- Each reconstructed module is validated against the release's observable behaviour, and
  the validator suite passes on the reconstructed sidecar.

## Definition of done

A unit of work (a function, a structure, a feature module, a format) is done when **all** of
these hold:

- [ ] Every address, offset and size carries its source: file, VA, RVA, and tool.
- [ ] Confirmed by two independent angles (for example a string xref *and* a call-graph
      path), not one.
- [ ] Written up in `notes/` with the reasoning, not just the conclusion.
- [ ] Named consistently with the rest of the map; unknown parts are `UNVERIFIED`, not
      invented.
- [ ] If it produces code, that code is original, lives in `scripts/`, and any binary it
      emits is regenerable from it.
- [ ] If it touches the binary, `validate_persistence_patch.py` and
      `test_best_runs_db.py` pass.
- [ ] Anything it contradicts is corrected, not appended to.

## Anti-goals

- Coverage percentage as a goal. Mapping 100% of functions you cannot explain is not
  progress.
- A pretty tree nobody can verify. A small, correct, evidence-backed core beats a large
  uncertain one.
- Chasing upstream. New Mega Hack releases are a re-baseline decision, not a treadmill.
- Tools written for one address that never get reused. Investigate in scratch space;
  promote to `scripts/` only when a second investigation reuses them.

## How to read the tracker

[ROADMAP.md](ROADMAP.md) holds the milestone table and exit criteria. Each milestone's
status is `not started` / `in progress` / `done`, and a milestone is `done` only when its
exit criteria are met — never on schedule pressure.