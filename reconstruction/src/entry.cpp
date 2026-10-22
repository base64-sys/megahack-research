#include "geode/entry.hpp"

#include <iostream>

namespace {
geode::Mod* g_mod = nullptr;
geode::async::Runtime* g_runtime = nullptr;

bool g_gameReady = false;
bool g_pendingLoad = false;
bool g_asyncTaskRan = false;
int g_tabRegistrations = 0;

hackpro::CoreDirector g_coreDirector;

// Stand-in for the unknown FUN_1800FFF60(runtime) async task (UNVERIFIED identity).
void runAsyncTask(geode::async::Runtime* runtime) {
    (void)runtime;
    g_asyncTaskRan = true;
}

// Stand-in for the descriptor installed by FUN_1800FDCE0. Its first field is
// FUN_180072C00 in the release; that identity is UNVERIFIED.
struct Descriptor {
    void (*first)() = nullptr;
};
Descriptor g_descriptorSlot{};
void installDescriptor(Descriptor* descriptor) {
    if (descriptor != nullptr) {
        g_descriptorSlot = *descriptor;
    }
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

CoreDirector& coreDirector() {
    return g_coreDirector;
}

bool gameReady() {
    return g_gameReady;
}

void setGameReady(bool ready) {
    g_gameReady = ready;
}

void registerTabs() {
    // FUN_180002320: HOME + KEYBINDS tabs, TLS init, CORE probe (see tab-registration.md).
    ++g_tabRegistrations;
}

void realiseLoad(CoreDirector& director) {
    // Deferred realisation of FUN_180006070 (FUN_180002620): init, set loaded, register tabs.
    director.loaded = true;
    registerTabs();
    g_pendingLoad = false;
}

void loadOnce(CoreDirector& director) {
    // FUN_180006070: guarded "load once" path.
    if (director.loaded) {
        return;
    }
    if (gameReady()) {
        director.loaded = true;
        registerTabs();
    } else {
        // Defer to realiseLoad until the game reports ready.
        g_pendingLoad = true;
    }
}

void onReady(CoreDirector& director) {
    // FUN_180002240: registers tabs only when the game reports ready.
    if (gameReady()) {
        registerTabs();
    }
    (void)director;
}

void setup() {
    // FUN_180072AA0: drives onReady through a std::function over CoreDirector*.
    CoreDirector& director = coreDirector();
    onReady(director);
}

void flushPendingLoads() {
    if (g_pendingLoad && gameReady()) {
        realiseLoad(g_coreDirector);
    }
}

int tabRegistrationCount() {
    return g_tabRegistrations;
}

bool asyncTaskRan() {
    return g_asyncTaskRan;
}

bool hasPendingLoad() {
    return g_pendingLoad;
}

void resetEarlyLoadState() {
    g_gameReady = false;
    g_pendingLoad = false;
    g_asyncTaskRan = false;
    g_tabRegistrations = 0;
    g_coreDirector = CoreDirector{};
    g_mod = nullptr;
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
    (void)severity;
    (void)mod;
    (void)format;
    (void)args;
}
} // namespace log

} // namespace geode
#endif

extern "C" {

HACKPRO_API void geodeImplicitEntry() {
    // 0x180072B60: geodeImplicitEntry
    auto* mod = hackpro::getMod();

    if (mod && !mod->isInternal()) {
        g_runtime = geode::async::runtime();
        runAsyncTask(g_runtime);
    }

    Descriptor descriptor{};
    // The release installs FUN_180072C00 here; identity UNVERIFIED.
    descriptor.first = nullptr;
    installDescriptor(&descriptor);
}

} // extern "C"
