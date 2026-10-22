#pragma once

#include "hackpro/types.hpp"

namespace geode {

class Mod {
public:
    virtual ~Mod() = default;
    virtual bool isInternal() const { return false; }
};

class Loader {
public:
    static Loader* get();
    Mod* takeNextMod();
};

namespace async {
class Runtime {};
Runtime* runtime();
} // namespace async

namespace log {
void vlogImpl(int severity, Mod* mod, const char** format, void* args);
} // namespace log

} // namespace geode

extern "C" {
HACKPRO_API void geodeImplicitEntry();
}

namespace hackpro {

HACKPRO_API geode::Mod* getMod();

// Early-load path (see notes/entry-path.md). These are original-work
// reconstructions of the named binary functions, not verbatim decompiler output.
struct CoreDirector {
    bool loaded = false;          // binary field +0x381
    void* levelHandle = nullptr;  // binary field +0x390
};

HACKPRO_API CoreDirector& coreDirector();
HACKPRO_API bool gameReady();
HACKPRO_API void setGameReady(bool ready);

HACKPRO_API void registerTabs();     // FUN_180002320
HACKPRO_API void realiseLoad(CoreDirector& director); // FUN_180002620
HACKPRO_API void loadOnce(CoreDirector& director);    // FUN_180006070
HACKPRO_API void onReady(CoreDirector& director);     // FUN_180002240
HACKPRO_API void setup();            // FUN_180072AA0 driver
HACKPRO_API void flushPendingLoads();
HACKPRO_API int tabRegistrationCount();
HACKPRO_API bool asyncTaskRan();
HACKPRO_API bool hasPendingLoad();
HACKPRO_API void resetEarlyLoadState();

} // namespace hackpro
