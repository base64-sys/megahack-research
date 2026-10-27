# Goals

## Mission

Recover a faithful, source-level understanding of **Mega Hack** v9.1.3 from its shipped
x86-64 binary, and rebuild it as original, compilable C++ that is behaviourally equivalent
to the release — documented well enough that a new maintainer can change a feature without
guessing.

The deliverable is understanding and original code. The binary is evidence, never the
product.

## Why this is not "just run Ghidra"

The mod is a single 8.7 MB PE with no debug info and only 30 exported C++ symbols, running
on an undocumented host (Geometry Dash 2.2081) behind a loader (Geode). Three things make
this more than a decompile-and-paste job:

1. **The host ABI is the real subject.** Most of Mega Hack's behaviour lives in calls into
   Geometry Dash and Geode. Recovering *those* signatures and object layouts is worth more
   than the mod's own logic, and it is reusable by every other GD mod.
2. **Correctness is behavioural, not syntactic.** Decompiler output is a starting point.
   A reconstruction is correct when it matches the release's observable behaviour, which
   means every claim needs an evidence trail back to an address and a disassembly.
3. **Scale.** Mega Hack ships 389 distinct feature wordings sharing one settings, menu and
   hook infrastructure. The infrastructure is the leverage; features are the bulk.

## Scope

**In scope**

- The Windows x86-64 binary `absolllute.hackmega_original.dll` (base `0x180000000`) — the
  primary target and the only ground truth.
- The Geometry Dash 2.2081 host surface Mega Hack calls into, including the host image's
  own function boundaries (`.pdata`), which is a legitimate second angle on hook targets.
- Mega Hack's own structures: settings registry, `nxUI` menu/interface layer, feature
  modules, save/load and lifecycle.
- Patching and instrumentation, provided it is ASLR-safe and reproducible from a script.
- Android ARM64 parity as a *re-application* of the x86-64 map (M5), not as new evidence
  about the release.

**Out of scope**

- The local persistence patch's sidecar format (`best_runs.bin`, magic `MHBRDB1`). It is
  this repo's own experiment, not Mega Hack's; its format stays in `samples/` and never
  becomes a project goal. Verified as a non-feature in
  [persistence.md](../notes/persistence.md).
- Cracking, licence checks or DRM circumvention. Mega Hack is a commercial product and this
  repository treats it as a black box supplied lawfully.
- Shipping or redistributing Absolllute's binaries or assets. `samples/` stays untracked.
- Any patch intended to make gameplay easier or to gain an advantage in ranked play. The
  reconstruction must reproduce behaviour, not improve it.
- Chasing new Mega Hack releases. A version bump is a deliberate re-baseline.

## Non-negotiables

These constrain every decision below and are not tradeable against velocity. How each is
enforced day to day lives in [AGENTS.md](../AGENTS.md); this is the *what*.

1. **Evidence over inference.** No struct offset, signature or behaviour claim enters
   `notes/` or `docs/` without a file, an address and the tool that produced it.
2. **Plausible is not verified.** Unproven work is labelled `UNVERIFIED: <what would close
   it>` and is never silently promoted later.
3. **Original work only.** No Absolllute source exists and none is reconstructed by
   guessing at intent. Describe behaviour; write your own implementation.
4. **Reproducibility.** A derived binary exists only if a tracked script regenerates it
   byte-for-byte and a validator passes. Nothing tracked may require `samples/`.
5. **ASLR safety.** No absolute virtual addresses in injected code; reach image addresses
   RIP-relative or by same-image rel32. The absolute-base `.data` variant crashed and must
   not return.
6. **Baseline discipline.** v9.1.3 / GD 2.2081 / Geode 5.6.1 is the static reference. The
   runtime environment is Geode 5.11.2 in an isolated profile, and a result from one is
   never quoted as the other.

## Success criteria

The project is complete when:

- Mega Hack's core infrastructure — entry, settings registry, menu layer, hook dispatch,
  save lifecycle — is reconstructed as C++ that compiles and links against recovered host
  declarations.
- Every feature in the changelog universe is accounted for, and every mapped feature names
  the code that implements it, its settings tag, its default and its interactions.
- The Geometry Dash object layouts Mega Hack depends on are documented well enough that a
  third party could write a Geode mod against them without the binary.
- Each sampled cluster is validated against the release's observable behaviour with a
  trace comparison that passes the comparability gate.

## Definition of done

A unit of work (a function, a structure, a feature module, a format) is done when **all**
of these hold:

- [ ] Every address, offset and size carries its source: file, VA, RVA and tool.
- [ ] Confirmed by two independent angles — for example a string xref *and* a call-graph
      path, or a registration argument *and* the host image's function boundaries.
- [ ] Written up in `notes/` with the method, so the next reader can repeat it.
- [ ] Named consistently with the rest of the map; unknown parts say `UNVERIFIED`.
- [ ] If it produces code, the code is original and tracked, and any binary it emits is
      regenerable from it.
- [ ] If it touches the binary, geometry or state, the patch validators pass.
- [ ] Anything it contradicts is corrected in place, not appended as a second truth.

## Anti-goals

- Coverage percentage as a goal. Mapping functions you cannot explain is not progress.
- A pretty tree nobody can verify. A small, correct, evidence-backed core beats a large
  uncertain one.
- Notes that restate a conclusion without the method that produced it.
- Tools written for one address that never get reused. Investigate in scratch space;
  promote to `scripts/` only when a second investigation reuses them.
- Long prose in the tracker. The roadmap holds criteria and status; the evidence lives in
  `notes/`.

## Where to look

| Question | File |
| --- | --- |
| What is left to do, and how is "done" judged? | [ROADMAP.md](ROADMAP.md) |
| What are the working rules for an agent or a human? | [AGENTS.md](../AGENTS.md) |
| What do we already know, and where is the evidence? | [README.md](../README.md#findings-worth-knowing) and `notes/` |
| How is the release validated against the reconstruction? | [runtime-validation.md](../notes/runtime-validation.md) |
