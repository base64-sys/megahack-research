#include "geode/entry.hpp"
#include <iostream>

namespace {
geode::Mod* g_mod = nullptr;
geode::async::Runtime* g_runtime = nullptr;

// Custom log callback routing to geode::log::vlogImpl with tag "[arc] {}"
void arcLogCallback(int severity, const std::string& message) {
    if (!g_mod) {
        g_mod = hackpro::getMod();
    }
    const char* fmt = "[arc] {}";
    // Severity mapping: severity 2 -> Geode info (3), severity 1 -> Geode debug (2)
    int geodeSev = (severity == 2) ? 3 : 2;
    void* args = const_cast<void*>(static_cast<const void*>(&message));
    geode::log::vlogImpl(geodeSev, g_mod, &fmt, args);
}
} // namespace

namespace hackpro {

geode::Mod* getMod() {
    if (!g_mod) {
        auto* loader = geode::Loader::get();
        if (loader) {
            g_mod = loader->takeNextMod();
        }
    }
    return g_mod;
}

} // namespace hackpro

// Dummy stubs for standalone testing / compilation
#ifndef GEODE_IS_LINKED
namespace geode {

static Loader s_dummyLoader;
static async::Runtime s_dummyRuntime;

Loader* Loader::get() {
    return &s_dummyLoader;
}

Mod* Loader::takeNextMod() {
    static Mod s_dummyMod;
    return &s_dummyMod;
}

namespace async {
Runtime* runtime() {
    return &s_dummyRuntime;
}
} // namespace async

namespace log {
void vlogImpl(int severity, Mod* mod, const char** format, void* args) {
    // In game runtime, writes to Geode log console
}
} // namespace log

} // namespace geode
#endif

extern "C" {

HACKPRO_API void geodeImplicitEntry() {
    // 0x180072b60: geodeImplicitEntry
    // 1. Check/obtain cached Mod* pointer from Loader::get()->takeNextMod()
    auto* mod = hackpro::getMod();

    // 2. If mod is external, obtain and store geode::async::runtime()
    if (mod && !mod->isInternal()) {
        g_runtime = geode::async::runtime();
    }

    // 3. Register [arc] logger handler
    // In binary: FUN_1800fdce0 with PTR_FUN_180542488 -> FUN_180072c00
}

}
