# Goals

## Mission

Recover a faithful, source-level understanding of **Mega Hack** v9.1.3 from its shipped x86-64 binary, and rebuild it as a **1:1 Geode mod** — original, compilable C++ that is behaviourally identical to the release in every observable way: same menu, same 160+ feature toggles, same license flow, same settings persistence, same hook surface, same safe_mode gating. Documented well enough that a new maintainer can change a feature without guessing.

The deliverable is a working Geode mod (`reconstruction/`) that a user cannot distinguish from the real Mega Hack v9.1.3 in-game. The binary is evidence, never the product.

## Why this is not "just run Ghidra"

The mod is a single ~8.7 MB PE with no symbols, no debug info and a stripped export table, running on an undocumented host (Geometry Dash 2.2081) behind a loader (Geode 5.6.1). Three things make this more than a decompile-and-paste job:

1. **The host ABI is the real subject.** Most of Mega Hack's behaviour lives in calls into Geometry Dash and Geode. Recovering *those* signatures and object layouts is worth more than the mod's own logic, and it is reusable by every other GD mod.
2. **Correctness is behavioural, not syntactic.** Decompiler output is a starting point. A reconstruction is correct when it matches the release's observable behaviour, which means every claim needs an evidence trail back to a VA and a disassembly.
3. **Scale.** Mega Hack ships 160+ feature toggles sharing a settings, menu and hook infrastructure. The infrastructure is the leverage; features are the bulk. The M3 mapping (8 clusters, 338 hooks, 215 GD methods, 202 setting keys) is the specification.

## Scope

**In scope**

- The Windows x86-64 binary `absolllute.hackmega_original.dll` (preferred base `0x180000000`) — the primary target.
- The Geometry Dash 2.2081 and Geode 5.6.1 host surface Mega Hack calls into.
- Mega Hack's own structures: settings registry, `nxUI` menu/interface layer, feature modules, save/load and lifecycle.
- **The license/entitlement system:** license file format, key validation, serial number logic, activation flow — reconstructed with **hardcoded constants in `src/license_constants.hpp`** (user-editable, no live service connections).
- **All 160+ feature toggles from `changelog.md`:** Input hooks (auto-clicker, frame stepper, startpos switcher, keybinds), Render hooks (hitboxes, trail, layout, trajectory, particles), Gameplay rewrites (noclip, no collision, auto deafen/kill, hitbox multiplier, jumpscare, respawn, random seed, speedhack), UI add-ons (status bar, replay, screenshot, custom BG, transitions), Save-data (accurate save, best run, cheat safety), Cosmetic/UI (accurate %, icon randomiser, pulse, trail, new best, pause button), Bypass/unlock (anti-kick, unlock icons, copy hack, editor extension, trigger bypass), Audio (music sync, speedhack audio, volume, pitch).
- **Two tabs exactly:** `HOME` (7,199 byte gzip-JSON) and `KEYBINDS` (442 byte gzip-JSON), registered at load via `registerTab` with gzip-compressed JSON declarative UI.
- **`mega::utils::safe_mode`**: `areCheatsEnabled()`, `disableAllCheats()`, `hasCheatedInAttempt()` — exported, shipped safety mechanism reproduced faithfully.
- Patching and instrumentation, provided it is ASLR-safe and reproducible from a script.
- Assets extracted from the original `.geode` (sprites, audio) referenced directly in `reconstruction/resources/`.

**Out of scope**

- Android `.so` parity (M6). Deferred until the x86-64 1:1 target is complete; ARM is a re-application of knowledge, not new knowledge.
- Connecting to Absolllute's live license/entitlement services. The license flow is reconstructed locally with user-controlled constants.
- Shipping or redistributing Absolllute's binaries. `samples/` stays untracked. The built `.geode` is for local testing only.
- Any patch intended to make gameplay easier or to gain an advantage in ranked play **beyond what Mega Hack itself provides**. The reconstruction reproduces Mega Hack's behaviour including its safe_mode gating; it does not improve on it.
- Reproducing Mega Hack's branding, artwork or shipped assets **as new creations** — extracted assets are used as-is.
- Geode 6.x migration (M7) — separate phase after 1:1 is complete on 5.6.1/2.2081.

## Non-negotiables

These constrain every decision below and are not tradeable against velocity.

1. **Evidence over inference.** No struct offset, function signature or behaviour claim enters `notes/` or `docs/` without a file, a VA and the tool that produced it.
2. **Plausible is not verified.** Unproven reconstructions are labelled `UNVERIFIED` and never silently promoted.
3. **Original work only.** No Absolllute source is available and none is reconstructed by guessing at their intent. Describe behaviour; write your own implementation.
4. **Reproducibility.** A derived binary exists only if a tracked script regenerates it byte-for-byte, and a validator passes.
5. **ASLR safety is non-negotiable.** No absolute virtual addresses in injected code; all image addresses are reached RIP-relative or via same-image rel32. The absolute-base `.data` variant crashed under ASLR and must not return.
6. **Baseline discipline.** v9.1.3 / GD 2.2081 / Geode 5.6.1. A version bump is a deliberate re-baselining with a diffed offset map, never a silent mix.
7. **Behavioural equivalence is the only metric.** The reconstruction passes when a user cannot distinguish it from the real Mega Hack v9.1.3 in-game.
8. **License constants are user-controlled.** All license parameters (public key, endpoints, serial format, validation thresholds) live in `src/license_constants.hpp` — one file, user-editable, no hardcoded secrets in the reconstruction logic.

## Success criteria

The project is complete when:

- Mega Hack's core infrastructure — entry, settings registry, menu layer, save lifecycle, license flow, safe_mode — is reconstructed as C++ that compiles and links against recovered host declarations.
- **All 160+ feature toggles** are mapped to the code that implements them, with their defaults, persistence behaviour and interactions with other toggles.
- The Geometry Dash object layouts Mega Hack depends on are documented well enough that a third party could write a Geode mod against them without the binary.
- Each reconstructed module is validated against the release's observable behaviour (side-by-side in-game comparison), and the validator suite passes.
- The built `.geode` loads under Geode 5.6.1 on GD 2.2081, registers two tabs (`HOME`, `KEYBINDS`), installs all 338 hooks, persists settings to `hackpro.json`, validates license from local file, and gates cheats via `safe_mode`.

## Definition of done

A unit of work (a function, a structure, a feature module, a format) is done when **all** of these hold:

- [ ] Every address, offset and size carries its source: file, VA, RVA, and tool.
- [ ] Confirmed by two independent angles (for example a string xref *and* a call-graph path), not one.
- [ ] Written up in `notes/` with the reasoning, not just the conclusion.
- [ ] Named consistently with the rest of the map; unknown parts are `UNVERIFIED`, not invented.
- [ ] If it produces code, that code is original, lives in `reconstruction/src/` or `scripts/`, and any binary it emits is regenerable from it.
- [ ] If it touches the binary, `validate_persistence_patch.py` and `test_best_runs_db.py` pass.
- [ ] Anything it contradicts is corrected, not appended to.
- [ ] **Behavioural validation:** the feature works identically to the release in-game (side-by-side comparison or automated test).

## Anti-goals

- Coverage percentage as a goal. Mapping 100% of functions you cannot explain is not progress.
- A pretty tree nobody can verify. A small, correct, evidence-backed core beats a large uncertain one.
- Chasing upstream. New Mega Hack releases are a re-baseline decision, not a treadmill.
- Tools written for one address that never get reused. Investigate in scratch space; promote to `scripts/` only when a second investigation reuses them.
- **Partial reconstruction.** The goal is 1:1 — menu, all features, license, everything. A "core only" reconstruction is not the mission.

## How to read the tracker

[ROADMAP.md](ROADMAP.md) holds the milestone table and exit criteria. Each milestone's status is `not started` / `in progress` / `done`, and a milestone is `done` only when its exit criteria are met — never on schedule pressure.