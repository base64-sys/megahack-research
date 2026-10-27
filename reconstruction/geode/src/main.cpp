// Reconstruction target: the second variant of the runtime-validation comparison.
//
// This mod installs the *hook surface* the release installs and nothing else. It is not an
// emitter: `megahack.trace` writes the trace for both variants, because the comparability gate
// requires one harness identity on both sides and two writers would fail it by construction.
// What this mod changes in a run is therefore exactly one thing, and that is the point: the
// observer's `hook.target` names this module instead of `absolllute.hackmega.dll` at every site
// the two share.
//
// Addresses (GeometryDash.exe RVAs, image base 0x140000000). Each is the same number the
// release hooks (`_hook_sites.tsv`) and the same one the Geode 2.2081 bindings generate:
//
//   0x3A5B00  PlayLayer::init          notes/hook-registrations.md:117
//   0x3B8EB0  PlayLayer::resetLevel    notes/address-map.md:50
//   0x3B4BA0  PlayLayer::postUpdate    notes/audio-sync-feature.md:103
//   0x397F40  PlayerObject::pushButton      notes/gameplay-hooks.md:1590
//   0x3981D0  PlayerObject::releaseButton  notes/gameplay-hooks.md:1591
//   0x16C8B0  GJGameLevel::savePercentage  notes/address-map.md:52 (optional in the manifest)
//
// `$modify` resolves each address through the loader, so no RVA appears in executed code and
// no detour calls an absolute address. Every detour forwards to the original and returns its
// result unchanged: a reconstruction that altered gameplay would be a different variant, not
// this one.

#include <Geode/modify/GJGameLevel.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>

using namespace geode::prelude;

class $modify(ReconstructionPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        return PlayLayer::init(level, useReplay, dontCreateObjects);
    }

    void resetLevel() {
        PlayLayer::resetLevel();
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
    }
};

class $modify(ReconstructionPlayerObject, PlayerObject) {
    bool pushButton(PlayerButton button) {
        return PlayerObject::pushButton(button);
    }

    bool releaseButton(PlayerButton button) {
        return PlayerObject::releaseButton(button);
    }
};

// Optional in the manifest (`hooks.optional`), present here because the release installs it:
// a surface the two variants do not share would be reported as a missing hook rather than as
// the behavioural difference the manifest would then have to declare.
class $modify(ReconstructionGJGameLevel, GJGameLevel) {
    void savePercentage(int percent, bool isPracticeMode, int clicks, int attempts,
                        bool isChkValid) {
        GJGameLevel::savePercentage(percent, isPracticeMode, clicks, attempts, isChkValid);
    }
};

$on_mod(Loaded) {
    // One line so a run's log names the variant that was installed. The hook installation
    // itself is proven by the loader's own "Enabled ... hook at <address> for <mod>" lines,
    // which are the evidence the runbook reads.
    log::info("megahack.reconstruction target loaded (v0.0.1): hook surface only, no trace");
}
