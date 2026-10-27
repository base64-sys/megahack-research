// Observer mod: the runtime half of the trace harness.
//
// O3a/O3b (harness/OBSERVER-PROGRESS.md): the hooks the pilot scenario declares as required.
// Each one writes a `hook` enter record before the original runs and a `hook` exit after it,
// with a call id, a depth and a parent taken from an explicit stack, so the call tree in the
// trace comes from real nesting rather than from an assertion.
//
// Contract source: notes/runtime-validation.md "hook event", "call pairing", "thread.role".
// Hook targets (file, address, tool), all four confirmed against the Geode 2.2081 bindings:
//
//   0x3B8EB0  PlayLayer::resetLevel      notes/address-map.md:50, gameplay-hooks.md
//   0x3B4BA0  PlayLayer::postUpdate      notes/audio-sync-feature.md:103
//   0x397F40  PlayerObject::pushButton   notes/gameplay-hooks.md:1590
//   0x3981D0  PlayerObject::releaseButton notes/gameplay-hooks.md:1591
//
// The bindings ship the same addresses (`GEODE_APPLY_MODIFY_FOR_FUNCTION(base::get() +
// 0x397f40, ...)` in the generated Geode/modify/PlayerObject.hpp), which is the second angle.

#include <Geode/loader/Dirs.hpp>
#include <Geode/modify/GJGameLevel.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>

#include <mh/contract.hpp>
#include <mh/context.hpp>
#include <mh/json.hpp>
#include <mh/records.hpp>
#include <mh/sha256.hpp>
#include <mh/trace_writer.hpp>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>

using namespace geode::prelude;

namespace {

struct HookSpec {
    const char* key;    // contract spelling: <module>:rva:<lowercase hex>
    const char* method; // human label for the same address
    std::uint32_t rva;
};

constexpr HookSpec kResetLevel{"GeometryDash.exe:rva:0x3b8eb0", "PlayLayer::resetLevel", 0x3B8EB0};
constexpr HookSpec kPostUpdate{"GeometryDash.exe:rva:0x3b4ba0", "PlayLayer::postUpdate", 0x3B4BA0};
constexpr HookSpec kPushButton{"GeometryDash.exe:rva:0x397f40", "PlayerObject::pushButton", 0x397F40};
constexpr HookSpec kSavePercentage{"GeometryDash.exe:rva:0x16c8b0", "GJGameLevel::savePercentage",
                                   0x16C8B0};
constexpr HookSpec kReleaseButton{"GeometryDash.exe:rva:0x3981d0",
                                  "PlayerObject::releaseButton", 0x3981D0};

// Every site the observer depends on, for the startup check that each one really is hooked.
constexpr std::array<HookSpec, 5> kRequiredSites{kResetLevel, kPostUpdate, kPushButton,
                                                 kReleaseButton, kSavePercentage};

// PlayLayer::init (GeometryDash.exe RVA 0x3A5B00) is not one of the manifest's required hook
// keys: it is observed as the `level_started` milestone, so no hook key is invented for it.
// RVA sources: hook-registrations.md:117 and the generated Geode/modify/PlayLayer.hpp note.

constexpr char kUnknownDigest[] =
    "0000000000000000000000000000000000000000000000000000000000000000";

struct Session {
    std::ofstream sink;
    std::unique_ptr<mh::TraceWriter> writer;
    std::thread::id load_thread;
    std::vector<std::string> call_stack;
    std::int64_t next_call = 1;
    std::int64_t started_ns = 0;
    // Writing is buffered and flushed every few events: flushing a JSONL line per hook event
    // cost a syscall per frame, which is why a ten-minute session produced 54k records slowly.
    // The trade is that a crash can lose the tail, which the missing `end` record already
    // signals - a run without `end` is not eligible anyway.
    int pending_flush = 0;
    bool overflow_reported = false;
    bool multi_target_reported = false;
};

void emit_error_locked(Session& session, const char* code, const char* message, bool fatal,
                       const char* hook_key);

// A trace that grows without bound is a trace nobody can read, and an unbounded run can fill a
// disk. The cap is reported as the contract's own `capture_overflow` error rather than silently
// truncating, and the run then stops recording (so `end.counts` stays truthful).
constexpr std::int64_t kMaxEvents = 200000;
constexpr int kFlushEvery = 64;

// One in-flight hook call. `active` is false when no trace is open, in which case the
// detour behaves like the original with a few cheap checks.
struct HookScope {
    bool active = false;
    std::string call_id;
    std::string parent;
    std::int64_t depth = 0;
    std::int64_t started_ns = 0;
};

std::mutex g_mutex;
std::unique_ptr<Session> g_session;

std::string uuid_v4() {
    std::random_device device;
    std::mt19937_64 generator(device());
    std::uint64_t high = generator();
    std::uint64_t low = generator();
    high = (high & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
    low = (low & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;

    char buffer[37];
    std::snprintf(buffer, sizeof(buffer), "%08llx-%04llx-%04llx-%04llx-%012llx",
                  static_cast<unsigned long long>(high >> 32) & 0xFFFFFFFFULL,
                  static_cast<unsigned long long>(high >> 16) & 0xFFFFULL,
                  static_cast<unsigned long long>(high) & 0xFFFFULL,
                  static_cast<unsigned long long>(low >> 48) & 0xFFFFULL,
                  static_cast<unsigned long long>(low) & 0xFFFFFFFFFFFFULL);
    return std::string(buffer);
}

std::string utc_now() {
    std::time_t now = std::time(nullptr);
    std::tm utc{};
#if defined(_MSC_VER)
    gmtime_s(&utc, &now);
#else
    gmtime_r(&now, &utc);
#endif
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02dT%02d:%02d:%02dZ",
                  utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday, utc.tm_hour, utc.tm_min,
                  utc.tm_sec);
    return std::string(buffer);
}

std::int64_t steady_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

mh::json::Json object() { return mh::json::Json::object(); }

void mark_unobserved(mh::Capture& capture, std::string path, std::string reason) {
    capture.missing.push_back(path);
    capture.reasons.emplace_back(std::move(path), std::move(reason));
}

bool has_member(const mh::json::Json& object, std::string_view key) {
    for (const auto& entry : object.members()) {
        if (entry.first == key) {
            return true;
        }
    }
    return false;
}

const mh::json::Json* find_member(const mh::json::Json& object, std::string_view key) {
    for (const auto& entry : object.members()) {
        if (entry.first == key) {
            return &entry.second;
        }
    }
    return nullptr;
}

std::string read_text_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    std::string text((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    return text;
}

// The runner's half of the run: `MH_TRACE_CONTEXT` points at a file holding the meta body
// (target, scenario, environment, mods, settings, level) plus repetition_index. The observer
// splices it instead of inventing nulls, computes the fingerprint from those members, and
// renames the file to `*.consumed` so a stale context cannot apply to a second run.
bool load_run_context(const std::string& path, mh::json::Json& body, std::string& fingerprint,
                      mh::Capture& capture) {
    const std::string text = read_text_file(path);
    if (text.empty()) {
        log::error("run context is empty or unreadable: {}", path);
        return false;
    }
    const mh::ContextResult result = mh::read_run_context(text);
    if (!result.ok()) {
        log::error("run context rejected: {}", result.error);
        return false;
    }

    body = result.context->body;
    fingerprint = mh::run_fingerprint(result.context->fingerprint_input());
    capture.status = mh::CaptureStatus::complete;
    capture.missing.clear();
    capture.reasons.clear();

    std::error_code ec;
    std::filesystem::rename(path, path + ".consumed", ec);
    if (ec) {
        log::warn("run context could not be marked consumed: {}", ec.message());
    }
    return true;
}

// The mods that are actually loaded, which only the runtime knows. Used for both the context
// path and the fallback: the runner cannot know what happened to load.
mh::json::Json build_mods_list() {
    mh::json::Json observer = object();
    observer.set("id", mh::json::Json::string(std::string(mh::contract::kHarnessId)));
    observer.set("role", mh::json::Json::string("observer"));
    observer.set("version", mh::json::Json::string(std::string(mh::contract::kHarnessVersion)));
    observer.set("sha256", mh::json::Json::null());

    mh::json::Json mods = mh::json::Json::array();
    mods.push(std::move(observer));
    for (Mod* mod : Loader::get()->getAllMods()) {
        if (mod && mod->getID() == "absolllute.hackmega") {
            mh::json::Json target_mod = object();
            target_mod.set("id", mh::json::Json::string("absolllute.hackmega"));
            target_mod.set("role", mh::json::Json::string("target"));
            target_mod.set("version", mh::json::Json::string(mod->getVersion().toNonVString()));
            target_mod.set("sha256", mh::json::Json::null());
            mods.push(std::move(target_mod));
            break;
        }
    }
    return mods;
}

mh::ThreadInfo current_thread(const Session& session) {
    mh::ThreadInfo info;
    if (std::this_thread::get_id() == session.load_thread) {
        // UNVERIFIED until a captured run compares this with the thread that runs the
        // frame: the mod load event is documented as running during startup, and this
        // records identity, not an inference from a thread name.
        info.id = "main";
        info.role = mh::ThreadRole::main;
    } else {
        char buffer[48];
        std::snprintf(buffer, sizeof(buffer), "thread:%llu",
                      static_cast<unsigned long long>(
                          std::hash<std::thread::id>{}(std::this_thread::get_id())));
        info.id = buffer;
        info.role = mh::ThreadRole::unknown;
    }
    return info;
}

std::uintptr_t image_base() {
    return reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
}

// The contract's `frame` field: the engine's own frame counter, not a restart-relative one.
std::optional<std::int64_t> current_frame() {
    if (auto* director = cocos2d::CCDirector::get()) {
        return static_cast<std::int64_t>(director->getTotalFrames());
    }
    return std::nullopt;
}

// The target's own hook on the same GD function, resolved once per address from the live hook
// list. This is what makes a trace say *which implementation* was running: the release and a
// reconstruction both hook the same addresses, and only the owning module differs. The contract
// spells `hook.target.module` as a module image name (`absolllute.hackmega_original.dll`), so
// that is what goes there, with the mod id kept alongside for the settings cross-reference.
struct TargetHook {
    std::string module;
    std::int64_t priority = 0;
};

// A cached lookup plus how many distinct mods claim the same address. More than one owner means
// `hook.target` cannot name a single implementation, so the record says nothing rather than
// picking one, and the run is told about the ambiguity.
struct TargetLookup {
    std::optional<TargetHook> hook;
    int owners = 0;
};

const TargetLookup& target_hook_for(std::uintptr_t address) {
    static std::map<std::uintptr_t, TargetLookup> cache;
    auto found = cache.find(address);
    if (found != cache.end()) {
        return found->second;
    }

    TargetLookup result;
    for (Mod* mod : Loader::get()->getAllMods()) {
        if (!mod || mod == Mod::get()) {
            continue;
        }
        for (Hook* hook : mod->getHooks()) {
            if (hook && hook->getAddress() == address) {
                std::filesystem::path binary = mod->getBinaryPath();
                std::string module = binary.filename().string();
                if (module.empty()) {
                    module = std::string(mod->getID());
                }
                if (result.owners == 0) {
                    result.hook = TargetHook{std::move(module),
                                             static_cast<std::int64_t>(hook->getPriority())};
                }
                ++result.owners;
                break;
            }
        }
    }
    if (result.owners > 1) {
        result.hook.reset();
    }
    return cache.emplace(address, std::move(result)).first->second;
}

// `hook.target`: the contract's fields are all present; the ones Geode 5.11.2 does not expose
// (a detour address, the target's own registration VA and tag) stay null rather than invented.
mh::json::Json target_hook_object(std::uintptr_t address) {
    const TargetLookup& lookup = target_hook_for(address);
    const std::optional<TargetHook>& target = lookup.hook;
    if (!target) {
        return mh::json::Json::null();
    }
    return mh::records::hook_target(target->module, target->priority);
}

// The `hook` object's shape lives in the portable core (`mh/records.hpp`), so the observer
// and any second implementation cannot drift apart on a key name. This only supplies the
// resolved address and the target it found.
mh::records::HookIdentity identity_for(const HookSpec& spec) {
    return mh::records::HookIdentity{spec.key, spec.method, "GeometryDash.exe", spec.rva,
                                     image_base() + spec.rva};
}

// Every record goes through here: it enforces the event cap and the flush batching, so no
// emit site can forget either.
void write_event(Session& session, const mh::json::Json& body, const mh::EventContext& context) {
    if (!session.writer) {
        return;
    }
    if (session.writer->counts().attempted >= kMaxEvents) {
        if (!session.overflow_reported) {
            session.overflow_reported = true;
            mh::json::Json overflow = mh::records::error_event(
                "capture_overflow", "event cap reached; recording stopped here", true,
                std::nullopt);
            session.writer->write_event(overflow, context);
            session.sink.flush();
        }
        return;
    }
    session.writer->write_event(body, context);
    if (++session.pending_flush >= kFlushEvery) {
        session.sink.flush();
        session.pending_flush = 0;
    }
}

// Emits one record. Caller holds g_mutex.
void emit_hook_locked(Session& session, const char* phase, const HookSpec& spec,
                      const HookScope& scope, void* self, std::int64_t duration_ns) {
    const std::optional<std::string_view> parent =
        scope.parent.empty() ? std::nullopt
                             : std::optional<std::string_view>(scope.parent);
    mh::json::Json call = mh::records::call_object(
        scope.call_id, scope.depth, parent,
        static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(self)),
        duration_ns >= 0 ? std::optional<std::int64_t>(duration_ns) : std::nullopt);

    mh::json::Json body = mh::records::hook_event(
        phase, identity_for(spec), target_hook_object(image_base() + spec.rva),
        std::move(call));

    // Two mods hooking the same function makes `hook.target` a guess, so it is left null and the
    // ambiguity is reported once instead of being resolved by iteration order.
    if (!session.multi_target_reported &&
        target_hook_for(image_base() + spec.rva).owners > 1) {
        session.multi_target_reported = true;
        emit_error_locked(session, "multiple_targets_loaded",
                          "more than one mod hooks this function; hook.target is null", true,
                          spec.key);
    }

    mh::EventContext context;
    context.t_ns = steady_ns();
    context.frame = current_frame();
    context.thread = current_thread(session);
    context.capture.status = mh::CaptureStatus::complete;

    write_event(session, body, context);
}

// notes/runtime-validation.md "milestone": kind, name, and data as a value object. The pilot
// manifest's required milestones are level_started and attempt_started.
void emit_milestone_locked(Session& session, const char* name) {
    mh::json::Json body = mh::records::milestone_event(name);

    mh::EventContext context;
    context.t_ns = steady_ns();
    context.frame = current_frame();
    context.thread = current_thread(session);
    context.capture.status = mh::CaptureStatus::complete;

    write_event(session, body, context);
}

// `save` is "an observed file state or write; not inferred from a hook" (notes/runtime-validation.md),
// so this records what the file system actually holds at that moment: a snapshot before the
// target's veto would run, and one after. Equal digests mean the write did not happen; a changed
// digest means it did. That difference is the only thing in a trace that depends on the target's
// behaviour rather than on the observer's own hooks.
mh::json::Json save_snapshot_object(const char* phase) {
    const std::filesystem::path save_dir = geode::dirs::getSaveDir();
    const std::filesystem::path path = save_dir / "CCLocalLevels.dat";

    std::error_code error;
    const bool exists = std::filesystem::exists(path, error) && !error;
    if (!exists) {
        return mh::records::save_event(phase, false, std::nullopt, std::nullopt, path.string());
    }

    const std::string bytes = read_text_file(path);
    return mh::records::save_event(phase, true, static_cast<std::int64_t>(bytes.size()),
                                   mh::sha256_hex(bytes), path.string());
}

void emit_save_locked(Session& session, const char* phase) {
    mh::EventContext context;
    context.t_ns = steady_ns();
    context.frame = current_frame();
    context.thread = current_thread(session);
    context.capture.status = mh::CaptureStatus::complete;

    write_event(session, save_snapshot_object(phase), context);
}

void emit_save(const char* phase) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_session || !g_session->writer) {
        return;
    }
    emit_save_locked(*g_session, phase);
}

// One `error` record, per the contract's error kind: code, message, fatal, hook_key.
void emit_error_locked(Session& session, const char* code, const char* message, bool fatal,
                       const char* hook_key) {
    mh::json::Json body = mh::records::error_event(
        code, message, fatal,
        hook_key ? std::optional<std::string_view>(hook_key) : std::nullopt);

    mh::EventContext context;
    context.t_ns = steady_ns();
    context.frame = current_frame();
    context.thread = current_thread(session);
    context.capture.status = mh::CaptureStatus::complete;

    write_event(session, body, context);
    session.sink.flush();
}

// Checks that can only fail silently, so they are asserted at startup: the runtime must be the
// one the run context names, every site the observer relies on must actually be hooked, and no
// second mod may be hooking the same functions (that would make `hook.target` ambiguous).
void verify_session(Session& session, const mh::json::Json& body) {
    const std::string game_version = Loader::get()->getGameVersion();
    const std::string geode_version = Loader::get()->getVersion().toNonVString();

    if (const mh::json::Json* environment = find_member(body, "environment")) {
        if (const mh::json::Json* gd = find_member(*environment, "gd")) {
            if (const mh::json::Json* declared = find_member(*gd, "version")) {
                if (declared->string_value() != game_version) {
                    emit_error_locked(session, "scenario_mismatch",
                                      "the running GD build is not the one the run names", true,
                                      nullptr);
                }
            }
        }
        if (const mh::json::Json* geode = find_member(*environment, "geode")) {
            if (const mh::json::Json* declared = find_member(*geode, "version")) {
                if (declared->string_value() != geode_version) {
                    emit_error_locked(session, "scenario_mismatch",
                                      "the running Geode build is not the one the run names", true,
                                      nullptr);
                }
            }
        }
    }

    std::vector<std::uintptr_t> installed;
    for (Hook* hook : Mod::get()->getHooks()) {
        if (hook) {
            installed.push_back(hook->getAddress());
        }
    }
    for (const HookSpec& spec : kRequiredSites) {
        const std::uintptr_t address = image_base() + spec.rva;
        const bool present = std::find(installed.begin(), installed.end(), address) != installed.end();
        if (!present) {
            emit_error_locked(session, "hook_install_failed",
                              "the hook is not present in this mod's hook list", true, spec.key);
        }
    }
}

void emit_milestone(const char* name) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_session || !g_session->writer) {
        return;
    }
    emit_milestone_locked(*g_session, name);
}

HookScope begin_hook(const HookSpec& spec, void* self) {
    HookScope scope;
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_session || !g_session->writer) {
        return scope;
    }
    Session& session = *g_session;
    scope.active = true;
    scope.call_id = "c" + std::to_string(session.next_call++);
    scope.depth = static_cast<std::int64_t>(session.call_stack.size()) + 1;
    if (!session.call_stack.empty()) {
        scope.parent = session.call_stack.back();
    }
    session.call_stack.push_back(scope.call_id);
    scope.started_ns = steady_ns();
    emit_hook_locked(session, "enter", spec, scope, self, -1);
    return scope;
}

void end_hook(HookScope& scope, const HookSpec& spec, void* self) {
    if (!scope.active) {
        return;
    }
    const std::int64_t duration = steady_ns() - scope.started_ns;
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_session || !g_session->writer) {
        return;
    }
    Session& session = *g_session;
    if (!session.call_stack.empty() && session.call_stack.back() == scope.call_id) {
        session.call_stack.pop_back();
    } else {
        auto position = std::find(session.call_stack.begin(), session.call_stack.end(),
                                  scope.call_id);
        if (position != session.call_stack.end()) {
            session.call_stack.erase(position);
        }
    }
    emit_hook_locked(session, "exit", spec, scope, self, duration);
}

} // namespace

class $modify(TracePlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        // Emitted before the original runs: PlayLayer::init sets the level up and may call
        // resetLevel itself, and the manifest's order group requires level_started first.
        // A failed init therefore still records the attempt, which is the honest reading.
        emit_milestone("level_started");
        return PlayLayer::init(level, useReplay, dontCreateObjects);
    }

    void resetLevel() {
        emit_milestone("attempt_started");
        HookScope scope = begin_hook(kResetLevel, this);
        PlayLayer::resetLevel();
        end_hook(scope, kResetLevel, this);
    }

    void postUpdate(float dt) {
        HookScope scope = begin_hook(kPostUpdate, this);
        PlayLayer::postUpdate(dt);
        end_hook(scope, kPostUpdate, this);
    }
};

class $modify(TracePlayerObject, PlayerObject) {
    bool pushButton(PlayerButton button) {
        HookScope scope = begin_hook(kPushButton, this);
        const bool result = PlayerObject::pushButton(button);
        end_hook(scope, kPushButton, this);
        return result;
    }

    bool releaseButton(PlayerButton button) {
        HookScope scope = begin_hook(kReleaseButton, this);
        const bool result = PlayerObject::releaseButton(button);
        end_hook(scope, kReleaseButton, this);
        return result;
    }
};

// The manifest lists `0x16c8b0` as an optional hook and `save_requested` as an optional
// milestone, so this site can be absent without invalidating a run.
class $modify(TraceGJGameLevel, GJGameLevel) {
    void savePercentage(int percent, bool isPracticeMode, int clicks, int attempts,
                        bool isChkValid) {
        emit_milestone("save_requested");
        emit_save("snapshot_before");
        HookScope scope = begin_hook(kSavePercentage, this);
        GJGameLevel::savePercentage(percent, isPracticeMode, clicks, attempts, isChkValid);
        end_hook(scope, kSavePercentage, this);
        emit_save("snapshot_after");
    }
};

$on_mod(Loaded) {
    auto session = std::make_unique<Session>();
    session->started_ns = steady_ns();
    session->load_thread = std::this_thread::get_id();

    const std::string run_id = uuid_v4();
    std::filesystem::path directory = Mod::get()->getSaveDir() / "traces";
    std::error_code ec;
    std::filesystem::create_directories(directory, ec);
    if (ec) {
        log::error("cannot create trace directory: {}", ec.message());
        return;
    }

    const std::filesystem::path path = directory / (run_id + ".jsonl");
    session->sink = mh::open_trace_file(path.string());
    if (!session->sink) {
        log::error("cannot open trace file: {}", path.string());
        return;
    }
    mh::Capture capture;
    mh::json::Json body = object();
    std::string fingerprint = kUnknownDigest;

    // Two ways to hand over the run context: `MH_TRACE_CONTEXT` names a file explicitly, and a
    // `pending-context.json` in the mod's own data directory covers every launch that does not
    // inherit the environment. A path that fails to load must not disable the other one - a
    // stale variable naming a consumed file silently cost a run once.
    std::vector<std::string> candidates;
    if (const char* from_environment = std::getenv("MH_TRACE_CONTEXT"); from_environment && *from_environment) {
        candidates.emplace_back(from_environment);
    }
    {
        std::error_code error;
        const std::filesystem::path pending =
            Mod::get()->getSaveDir() / "pending-context.json";
        if (std::filesystem::exists(pending, error)) {
            const std::string path = pending.string();
            if (candidates.empty() || candidates.front() != path) {
                candidates.push_back(path);
            }
        }
    }

    bool loaded = false;
    for (const std::string& candidate : candidates) {
        if (load_run_context(candidate, body, fingerprint, capture)) {
            loaded = true;
            break;
        }
        body = object();
        capture = mh::Capture();
    }
    if (!loaded && !candidates.empty()) {
        capture.status = mh::CaptureStatus::partial;
        mark_unobserved(capture, "run_context",
                        "no run context loaded; the log names each path that was tried");
    }

    if (body.members().empty()) {
    capture.status = mh::CaptureStatus::partial;
    mark_unobserved(capture, "target.artifact", "the runner resolves the target artifact");
    mark_unobserved(capture, "target.sha256", "the runner hashes the target artifact");
    mark_unobserved(capture, "target.size", "the runner hashes the target artifact");
    mark_unobserved(capture, "target.image_base", "the runner reads the target header");
    mark_unobserved(capture, "scenario.id", "the scenario manifest drives a run, not a load");
    mark_unobserved(capture, "scenario.sha256", "the scenario manifest drives a run, not a load");
    mark_unobserved(capture, "settings.tags", "settings are observed at a hook, not at load");
    mark_unobserved(capture, "settings.snapshot_sha256", "settings are observed at a hook");
    mark_unobserved(capture, "level.fixture", "no level is loaded when this mod loads");
    mark_unobserved(capture, "level.sha256", "no level is loaded when this mod loads");
    mark_unobserved(capture, "environment.gd.version", "the observer does not read the GD version yet");
    mark_unobserved(capture, "environment.gd.sha256", "the runner hashes the GD image");
    mark_unobserved(capture, "environment.geode.sha256", "the runner hashes the Geode image");
    mark_unobserved(capture, "environment.harness.sha256", "the runner hashes the observer module");

    body.set("created_utc", mh::json::Json::string(utc_now()));

    mh::json::Json target = object();
    target.set("role", mh::json::Json::string("release"));
    target.set("artifact", mh::json::Json::null());
    target.set("sha256", mh::json::Json::null());
    target.set("size", mh::json::Json::null());
    target.set("image_base", mh::json::Json::null());
    body.set("target", std::move(target));

    mh::json::Json scenario = object();
    scenario.set("id", mh::json::Json::null());
    scenario.set("sha256", mh::json::Json::null());
    body.set("scenario", std::move(scenario));

    mh::json::Json gd = object();
    gd.set("version", mh::json::Json::null());
    gd.set("sha256", mh::json::Json::null());

    mh::json::Json geode = object();
    geode.set("version", mh::json::Json::string(Loader::get()->getVersion().toNonVString()));
    geode.set("sha256", mh::json::Json::null());

    mh::json::Json harness = object();
    harness.set("id", mh::json::Json::string(std::string(mh::contract::kHarnessId)));
    harness.set("version", mh::json::Json::string(std::string(mh::contract::kHarnessVersion)));
    harness.set("sha256", mh::json::Json::null());

    mh::json::Json environment = object();
    environment.set("os", mh::json::Json::string("Windows"));
    environment.set("arch", mh::json::Json::string("x86_64"));
    environment.set("gd", std::move(gd));
    environment.set("geode", std::move(geode));
    environment.set("harness", std::move(harness));
    body.set("environment", std::move(environment));

    mh::json::Json observer = object();
    body.set("mods", build_mods_list());

    mh::json::Json settings = object();
    settings.set("reset", mh::json::Json::string("defaults"));
    settings.set("tags", mh::json::Json::array());
    settings.set("snapshot_sha256", mh::json::Json::null());
    body.set("settings", std::move(settings));

    mh::json::Json level = object();
    level.set("fixture", mh::json::Json::null());
    level.set("sha256", mh::json::Json::null());
    level.set("source", mh::json::Json::null());
    body.set("level", std::move(level));

    mh::json::Json clock = object();
    clock.set("source", mh::json::Json::string("steady_clock"));
    clock.set("unit", mh::json::Json::string("ns"));
    body.set("clock", std::move(clock));
    } // end unobserved fallback

    if (!has_member(body, "created_utc")) {
        body.set("created_utc", mh::json::Json::string(utc_now()));
    }
    // Loaded mods are a runtime fact, so the observer states them even when the runner supplied
    // the rest of the meta.
    if (!has_member(body, "mods")) {
        body.set("mods", build_mods_list());
    }

    session->writer = std::make_unique<mh::TraceWriter>(
        session->sink, mh::TraceHeader{run_id, fingerprint});

    session->writer->write_meta(body, capture);
    session->sink.flush();

    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_session = std::move(session);
    }

    // Two checks that would otherwise fail silently: a build that is not the one the run was
    // planned against, and a hook Geode did not install. Both are reported as the contract's
    // `error` records rather than left to be inferred from missing events.
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (g_session && g_session->writer) {
            verify_session(*g_session, body);
        }
    }

    log::info("trace opened at {} (run {})", path.string(), run_id);
}

// The SDK documents GameEventType::Exiting as the safe place for global cleanup. A crash
// never reaches it, so the trace stays without an end record - incomplete, not truncated.
$on_game(Exiting) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_session || !g_session->writer) {
        return;
    }
    mh::EndInfo end;
    end.status = mh::EndStatus::stopped;
    end.t_ns = steady_ns() - g_session->started_ns;
    end.duration_ns = end.t_ns;
    end.unclosed = static_cast<std::int64_t>(g_session->call_stack.size());
    end.capture.status = mh::CaptureStatus::partial;
    end.capture.missing = {"artifacts.stdout_sha256"};
    end.capture.reasons.emplace_back("artifacts.stdout_sha256",
                                     "the observer does not capture output");
    g_session->writer->write_end(end);
    g_session->sink.flush();
    g_session.reset();
}
