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
}
