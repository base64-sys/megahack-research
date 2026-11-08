#include "geode/entry.hpp"
#include <iostream>
#include <thread>
#include <mutex>
#include <functional>
#include <sstream>

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

// ============================================================================
// CoreDirector implementation (DAT_1808004d0)
// ============================================================================

namespace {
CoreDirector* g_coreDirector = nullptr;
} // namespace

CoreDirector* CoreDirector::instance() {
    if (!g_coreDirector) {
        g_coreDirector = new CoreDirector();
    }
    return g_coreDirector;
}

void CoreDirector::initialize() {
    // CoreDirector initialization (FUN_180002200 at 0x180002200)
    // In binary: sets up internal state for hook management
}

// ============================================================================
// TLS Guard for CoreDirector initialization (DAT_1808008d8)
// Matches binary TLS initialization pattern at 0x1808008D8
// ============================================================================

namespace detail {

// TLS initialization state (matches DAT_1808008d8 at 0x1808008D8)
static thread_local int g_tlsInitState = 0;
static std::mutex g_tlsInitMutex;
static bool g_coreDirectorInitialized = false;

int getTlsInitState() {
    return g_tlsInitState;
}

void setTlsInitState(int state) {
    g_tlsInitState = state;
}

bool isCoreDirectorInitialized() {
    return g_coreDirectorInitialized;
}

void setCoreDirectorInitialized(bool initialized) {
    g_coreDirectorInitialized = initialized;
}

// FUN_1803a5140 - TLS initialization helper
void tlsInitHelper(int* tlsGuard) {
    // In binary: initializes TLS guard via CRT _Init_thread_header/_Init_thread_footer
    // For reconstruction: simple atomic initialization
    std::lock_guard<std::mutex> lock(g_tlsInitMutex);
    if (*tlsGuard == 0) {
        *tlsGuard = 1; // Initializing
    }
}

// FUN_1800010d0 - CoreDirector constructor/initialization
void initializeCoreDirector(void* coreDirectorPtr) {
    // In binary: initializes CoreDirector object at DAT_1808004d0
    // For reconstruction: call CoreDirector::initialize()
    if (coreDirectorPtr) {
        static_cast<CoreDirector*>(coreDirectorPtr)->initialize();
    }
}

// FUN_180001370 - Shutdown handler (registered with atexit)
void coreDirectorShutdown() {
    // In binary: resets CoreDirector vtables, frees allocations
    // For reconstruction: cleanup
    g_coreDirectorInitialized = false;
    g_tlsInitState = 0;
}

// FUN_1800bbd90 - Check if early-load should run
bool shouldRunEarlyLoad() {
    // In binary: checks some condition (likely mod state)
    // Returns true to proceed with early-load
    return true;
}

// FUN_1800bbec0 - Another early-load condition check
bool checkEarlyLoadCondition() {
    // In binary: additional condition check
    return true;
}

// FUN_1800baf30 - Register early-load callback with CoreDirector
void registerEarlyLoadCallback(CoreDirector* director, std::function<void()> callback) {
    // In binary: registers the std::function<void()> with CoreDirector
    // For reconstruction: store and invoke when ready
    if (director && callback) {
        callback();
    }
}

// ============================================================================
// Tab registration (FUN_180002320 at 0x180002320)
// Registers HOME and KEYBINDS tabs from gzip-compressed JSON
// ============================================================================

// Minimal stub implementations for tab registration (FUN_1800bea70, FUN_1800bed70, FUN_1800bee20, FUN_180039360, FUN_18003d220)
void* getTabStreamHandle() {
    // FUN_1800bea70 at 0x1800BEA70 - returns handle for tab streams
    static int dummy = 0;
    return &dummy;
}

void buildHomeTabStream(void* handle, std::istream** outStream) {
    // FUN_1800bed70 at 0x1800BED70 - builds HOME tab gzip stream
    // In binary: decompresses DAT_1806FE004 (7,199 bytes uncompressed JSON)
    // For reconstruction: create a dummy stream with HOME tab JSON
    static std::string homeJson = R"({"type":"tab","id":"HOME","elements":[]})";
    static std::istringstream stream(homeJson);
    *outStream = &stream;
}

void buildKeybindsTabStream(void* handle, std::istream** outStream) {
    // FUN_1800bee20 at 0x1800BEE20 - builds KEYBINDS tab gzip stream
    // In binary: decompresses DAT_1806FE2D9 (442 bytes uncompressed JSON)
    // For reconstruction: create a dummy stream with KEYBINDS tab JSON
    static std::string keybindsJson = R"({"type":"tab","id":"KEYBINDS","elements":[]})";
    static std::istringstream stream(keybindsJson);
    *outStream = &stream;
}

void* getHomeDelegate() {
    // FUN_180039360 at 0x180039360 - returns HomeWin delegate singleton
    // In binary: vtable at 0x180800AD8, mega::extensions::Home -> core::HomeWin
    static int dummy = 0;
    return &dummy;
}

void* getKeybindsDelegate() {
    // FUN_18003d220 at 0x18003D220 - returns Keybinds delegate singleton
    // In binary: vtable at 0x180800C20, core::Keybinds
    static int dummy = 0;
    return &dummy;
}

bool registerTab(std::istream& stream, void* delegate) {
    // FUN_1800A60B0 at 0x1800A60B0 (via registerTab export at 0x180072E50)
    // In binary: parses JSON stream and associates delegate
    (void)stream;
    (void)delegate;
    return true;
}

void runTabRegistration() {
    // FUN_180002320 at 0x180002320
    // 1. Register HOME tab (stream from FUN_1800bed70, delegate from FUN_180039360)
    // 2. Register KEYBINDS tab (stream from FUN_1800bee20, delegate from FUN_18003d220)
    // 3. TLS initialization for DAT_1808008d8
    // 4. CORE readiness probe (FUN_1800a2fb0)
    // 5. Follow-up initialization if probe passes
    
    // Tab 1: HOME
    void* handle1 = getTabStreamHandle();
    std::istream* stream1 = nullptr;
    buildHomeTabStream(handle1, &stream1);
    void* delegate1 = getHomeDelegate();
    if (stream1 && delegate1) {
        registerTab(*stream1, delegate1);
    }
    
    // Tab 2: KEYBINDS
    void* handle2 = getTabStreamHandle();
    std::istream* stream2 = nullptr;
    buildKeybindsTabStream(handle2, &stream2);
    void* delegate2 = getKeybindsDelegate();
    if (stream2 && delegate2) {
        registerTab(*stream2, delegate2);
    }
    
    // TLS initialization (matches binary pattern at 0x180002377-0x1800023A4)
    // Checks DAT_1808008d8 TLS guard, initializes if needed
    
    // CORE readiness probe (FUN_1800a2fb0 at 0x1800A2FB0)
    // Builds "CORE" string, checks against global at 0x180801410
    // If passes, runs follow-up initialization
}

} // namespace detail

// ============================================================================
// FUN_180001010 at 0x180001010 - Get/initialize CoreDirector singleton
// Returns &DAT_1808004d0 (CoreDirector instance)
// ============================================================================
extern "C" CoreDirector* getCoreDirectorInstance() {
    // TLS guard check (matches binary pattern at 0x180001022-0x180001041)
    int tlsState = detail::getTlsInitState();
    if (tlsState < 1) { // Not initialized
        detail::tlsInitHelper(&tlsState);
        if (tlsState == -1) { // First-time initialization
            detail::initializeCoreDirector(hackpro::CoreDirector::instance());
            std::atexit(detail::coreDirectorShutdown);
            detail::setTlsInitState(2); // Initialized
            detail::setCoreDirectorInitialized(true);
        }
    }
    return hackpro::CoreDirector::instance();
}

// ============================================================================
// FUN_180002240 at 0x180002240 - Early-load trigger
// Called via std::function bound to CoreDirector
// ============================================================================
extern "C" void runEarlyLoadTrigger(void* coreDirectorPtr) {
    // FUN_180002240 at 0x180002240
    // 1. Calls FUN_1800d96d0(param_1, 0) - some initialization
    // 2. Calls FUN_1800bbd90() and FUN_1800bbec0() - condition checks
    // 3. If conditions pass, calls FUN_180002320() - tab registration
    // 4. Registers "FIRST_LOAD" string via FUN_1800ddd00
    
    auto* director = static_cast<CoreDirector*>(coreDirectorPtr);
    
    // FUN_1800d96d0 - initialization with param
    (void)director; // Used in binary
    
    // Condition checks
    if (detail::shouldRunEarlyLoad()) {
        if (detail::checkEarlyLoadCondition()) {
            // Trigger tab registration
            detail::runTabRegistration();
        }
    }
    
    // Register "FIRST_LOAD" marker (FUN_1800ddd00 at 0x1800DDD00)
    // In binary: stores "FIRST_LOAD" string (0x4f4c5f5453524946 = "FIRST_LO")
    // This is a marker that early-load has completed
}

// ============================================================================
// FUN_180072AA0 at 0x180072AA0 - Init-table entry
// Runs during CRT _initterm, registers early-load callback with CoreDirector
// ============================================================================
extern "C" void initTableEarlyLoadEntry() {
    // FUN_180072AA0 at 0x180072AA0 (init-table entry #11 at 0x18063FA28)
    // 1. Calls FUN_180001010() -> getCoreDirectorInstance()
    // 2. Calls FUN_180002200() -> CoreDirector::initialize()
    // 3. Creates std::function<void()> bound to FUN_180002240 with CoreDirector
    // 4. Calls FUN_1800baf30() to register the callback
    
    auto* director = getCoreDirectorInstance();
    director->initialize(); // FUN_180002200
    
    // Create std::function binding FUN_180002240 to CoreDirector
    // In binary: uses std::_Func_impl_no_alloc with std::_Binder
    auto callback = [director]() {
        runEarlyLoadTrigger(director);
    };
    
    // Register with CoreDirector (FUN_1800baf30 at 0x1800BAF30)
    detail::registerEarlyLoadCallback(director, callback);
}

// ============================================================================
// Early-load callback implementation (FUN_1800fdce0 at 0x1800FDCE0)
// Matches binary layout at 0x180801708 (DAT_180801708)
// ============================================================================

namespace detail {

// Vtable for early-load function object (PTR_FUN_180542488 at 0x180542488)
void earlyLoadDtor(void* obj) {
    // Destructor: cleanup function object
}

void earlyLoadInvoke(void* obj, CoreDirector* director) {
    // Invoke: called when CoreDirector is ready
    auto* callback = static_cast<EarlyLoadCallback*>(obj);
    if (callback && callback->director) {
        // Execute the bound early-load logic
        // In binary: this triggers FUN_180002320 via CoreDirector
    }
}

void earlyLoadCleanup(void* obj) {
    // Cleanup: called during shutdown
}

const EarlyLoadVTable g_earlyLoadVTable = {
    .dtor = earlyLoadDtor,
    .invoke = earlyLoadInvoke,
    .cleanup = earlyLoadCleanup
};

EarlyLoadCallback g_earlyLoadCallback = {
    .vtable = &g_earlyLoadVTable,
    .director = nullptr
};

void initializeEarlyLoadCallback(EarlyLoadCallback* callback) {
    // FUN_1800fdce0 at 0x1800FDCE0
    // In binary: manages function object lifecycle with vtable PTR_FUN_180542488
    // If callback is not the global g_earlyLoadCallback, does init/cleanup dance
    // Then stores callback's vtable and calls cleanup if present
    
    if (callback != &g_earlyLoadCallback) {
        const EarlyLoadVTable* srcVtable = callback->vtable;
        const EarlyLoadVTable* dstVtable = g_earlyLoadCallback.vtable;
        
        if (!dstVtable) {
            dstVtable = &g_earlyLoadVTable;
        }
        if (!srcVtable) {
            srcVtable = &g_earlyLoadVTable;
        }
        
        // Binary logic: if src has invoke, call it with global and src
        // else if dst has cleanup, save global state, call src invoke, then dst cleanup
        // else if dst has invoke, save global state to local, call dst invoke, then src invoke, then dst cleanup
        
        // For reconstruction: bind the callback to CoreDirector
        callback->director = CoreDirector::instance();
        callback->vtable = &g_earlyLoadVTable;
        
        // Copy to global
        g_earlyLoadCallback = *callback;
    }
    
    // Call cleanup on the active vtable
    const EarlyLoadVTable* activeVtable = g_earlyLoadCallback.vtable;
    if (activeVtable && activeVtable->cleanup) {
        activeVtable->cleanup(&g_earlyLoadCallback);
    }
}

} // namespace detail

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

    // 3. Initialize early-load callback (FUN_1800fdce0 with PTR_FUN_180542488)
    // In binary: creates local function object with vtable PTR_FUN_180542488,
    // passes to FUN_1800fdce0 for registration
    hackpro::detail::EarlyLoadCallback localCallback;
    localCallback.vtable = &hackpro::detail::g_earlyLoadVTable;
    localCallback.director = nullptr;
    hackpro::detail::initializeEarlyLoadCallback(&localCallback);

    // 4. Register [arc] logger handler (original binary continues to FUN_180072c00)
}

}