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

// Forward declarations for CoreDirector and early-load path
namespace hackpro {

class CoreDirector {
public:
    static CoreDirector* instance();
    void initialize();
};

namespace detail {

// Function object vtable for early-load callback (PTR_FUN_180542488 at 0x180542488)
struct EarlyLoadVTable {
    void (*dtor)(void*);
    void (*invoke)(void*, CoreDirector*);
    void (*cleanup)(void*);
};

extern const EarlyLoadVTable g_earlyLoadVTable;

// Function object for early-load callback (matches binary layout at 0x180801708)
struct EarlyLoadCallback {
    const EarlyLoadVTable* vtable;
    CoreDirector* director;
};

extern EarlyLoadCallback g_earlyLoadCallback;

// Initialize early-load callback (FUN_1800fdce0 at 0x1800FDCE0)
void initializeEarlyLoadCallback(EarlyLoadCallback* callback);

} // namespace detail

} // namespace hackpro

extern "C" {

HACKPRO_API void geodeImplicitEntry();

}

namespace hackpro {
HACKPRO_API geode::Mod* getMod();
}