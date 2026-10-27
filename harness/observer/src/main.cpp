// Observer mod: the runtime half of the trace harness.
//
// O3a (harness/OBSERVER-PROGRESS.md): one hook, end to end - PlayLayer::resetLevel. It emits
// a `hook` enter record before the original runs and `hook` exit after it, through the
// portable core's TraceWriter, so the call stack in the trace is produced by real nesting
// rather than asserted.
//
// Contract sources: notes/runtime-validation.md "hook event" (fields), "call pairing"
// (call_id unique per run, depth from 1, parent is the enclosing observed call),
// "thread.role" (main only when proven).
//
// The hook target is GeometryDash.exe RVA 0x3B8EB0: notes/address-map.md row
// `PlayLayer::resetLevel`, and the contract's own example trace names the same key.

#include <Geode/modify/PlayLayer.hpp>

#include <mh/contract.hpp>
#include <mh/json.hpp>
#include <mh/json_parse.hpp>
#include <mh/trace_writer.hpp>
#include <mh/value.hpp>

#include <chrono>
#include <cstdint>
#include <cstdio>
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

// notes/address-map.md: PlayLayer::resetLevel, RVA 0x3B8EB0 (image base 0x140000000).
constexpr std::uint32_t kPlayLayerResetLevelRva = 0x3B8EB0;
constexpr char kPlayLayerResetLevelKey[] = "GeometryDash.exe:rva:0x3b8eb0";

constexpr char kUnknownDigest[] =
    "0000000000000000000000000000000000000000000000000000000000000000";

struct Session {
    std::ofstream sink;
    std::unique_ptr<mh::TraceWriter> writer;
    std::thread::id load_thread;
    std::vector<std::string> call_stack;
    std::int64_t next_call = 1;
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

// The value model owns its serialisation; parsing it back keeps one spelling of a value
// object in the repository instead of a second, hand-written one here.
mh::json::Json value_json(const mh::Value& value) {
    auto parsed = mh::json::parse(value.to_json());
    if (parsed.value) {
        return *parsed.value;
    }
    return mh::json::Json::null();
}

void mark_unobserved(mh::Capture& capture, std::string path, std::string reason) {
    capture.missing.push_back(path);
    capture.reasons.emplace_back(std::move(path), std::move(reason));
}

mh::ThreadInfo current_thread(const Session& session) {
    mh::ThreadInfo info;
    if (std::this_thread::get_id() == session.load_thread) {
        // UNVERIFIED until a captured run compares this with the frame thread: Geode loads
        // mods on the thread that starts the game, and this records identity, not a guess
        // from a thread name.
        info.id = "main";
        info.role = mh::ThreadRole::main;
    } else {
        char buffer[32];
        std::snprintf(buffer, sizeof(buffer), "thread:%llu",
                      static_cast<unsigned long long>(
                          std::hash<std::thread::id>{}(std::this_thread::get_id())));
        info.id = buffer;
        info.role = mh::ThreadRole::unknown;
    }
    return info;
}

mh::json::Json build_hook_object(std::uintptr_t image_base) {
    mh::json::Json hook = object();
    hook.set("observer", mh::json::Json::string("gd_method"));
    hook.set("key", mh::json::Json::string(kPlayLayerResetLevelKey));
    hook.set("module", mh::json::Json::string("GeometryDash.exe"));
    hook.set("method", mh::json::Json::string("PlayLayer::resetLevel"));
    hook.set("resolution", mh::json::Json::string("rva"));
    hook.set("rva", mh::json::Json::string("0x3B8EB0"));
    hook.set("va", mh::json::Json::string(
                       [&] {
                           char buffer[32];
                           std::snprintf(buffer, sizeof(buffer), "0x%llX",
                                         static_cast<unsigned long long>(
                                             image_base + kPlayLayerResetLevelRva));
                           return std::string(buffer);
                       }()));
    hook.set("target", mh::json::Json::null());
    hook.set("convention", mh::json::Json::string("unknown"));
    return hook;
}

std::uintptr_t image_base() {
    return reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
}

void emit_hook(const char* phase, const std::string& call_id, std::int64_t depth,
               const std::string& parent_call_id, void* self, bool has_parent,
               std::int64_t duration_ns) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_session || !g_session->writer) {
        return;
    }
    Session& session = *g_session;

    mh::json::Json call = object();
    call.set("call_id", mh::json::Json::string(call_id));
    call.set("depth", mh::json::Json::integer(depth));
    call.set("parent_call_id",
             has_parent ? mh::json::Json::string(parent_call_id) : mh::json::Json::null());
    call.set("this", value_json(mh::Value::observed_ptr(
                       static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(self)))));
    call.set("args", value_json(
                         mh::Value::unavailable("arguments are not captured by this hook")));
    call.set("return", value_json(mh::Value::null_value()));
    if (duration_ns >= 0) {
        call.set("duration_ns", mh::json::Json::integer(duration_ns));
    }

    mh::json::Json body = object();
    body.set("kind", mh::json::Json::string("hook"));
    body.set("phase", mh::json::Json::string(phase));
    body.set("hook", build_hook_object(image_base()));
    body.set("call", std::move(call));

    mh::EventContext context;
    context.t_ns = steady_ns();
    context.thread = current_thread(session);
    context.capture.status = mh::CaptureStatus::complete;

    session.writer->write_event(body, context);
    session.sink.flush();
}

} // namespace

class $modify(TracePlayLayer, PlayLayer) {
    void resetLevel() {
        std::string call_id;
        std::string parent;
        std::int64_t depth = 0;
        std::int64_t started = 0;
        bool tracked = false;

        {
            std::lock_guard<std::mutex> lock(g_mutex);
            if (g_session && g_session->writer) {
                Session& session = *g_session;
                call_id = "c" + std::to_string(session.next_call++);
                depth = static_cast<std::int64_t>(session.call_stack.size()) + 1;
                const bool has_parent = !session.call_stack.empty();
                if (has_parent) {
                    parent = session.call_stack.back();
                }
                session.call_stack.push_back(call_id);
                tracked = true;

                emit_hook("enter", call_id, depth, parent, this, has_parent, -1);
                started = steady_ns();
            }
        }

        PlayLayer::resetLevel();

        if (tracked) {
            const std::int64_t duration = steady_ns() - started;
            {
                std::lock_guard<std::mutex> lock(g_mutex);
                if (g_session && !g_session->call_stack.empty()) {
                    g_session->call_stack.pop_back();
                }
            }
            emit_hook("exit", call_id, depth, parent, this, !parent.empty(), duration);
        }
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
    session->writer = std::make_unique<mh::TraceWriter>(
        session->sink, mh::TraceHeader{run_id, kUnknownDigest});

    mh::Capture capture;
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

    mh::json::Json body = object();
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
    geode.set("version", mh::json::Json::string(
                             Loader::get()->getVersion().toNonVString()));
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
            target_mod.set("version", mh::json::Json::null());
            target_mod.set("sha256", mh::json::Json::null());
            mods.push(std::move(target_mod));
            break;
        }
    }
    body.set("mods", std::move(mods));

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

    session->writer->write_meta(body, capture);
    session->sink.flush();

    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_session = std::move(session);
    }
    log::info("trace opened at {} (run {})", path.string(), run_id);
}

// The game's own exit event: "This can be used as a safe place for cleanup of global state"
// (Geode loader/include/Geode/loader/GameEvent.hpp). A crash never reaches it, which is what
// the contract wants - no end record means the run is incomplete, not truncated.
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
