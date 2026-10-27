// Observer mod: proves the trace pipeline inside the game.
//
// This skeleton owns no hooks yet. On load it writes one meta record and one end record
// through the portable core (`harness/src/trace_writer.cpp`), so that "the writer works in
// the runtime" is an observation rather than an assumption. Everything the observer cannot
// know at load time - the target's hash, the scenario, the level, the settings snapshot -
// is written as null and named in `capture.missing`, which is exactly what makes such a
// trace ineligible as evidence: the runner fills those fields, not this mod.

#include <Geode/Geode.hpp>

#include <mh/contract.hpp>
#include <mh/json.hpp>
#include <mh/trace_writer.hpp>

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <random>
#include <string>

using namespace geode::prelude;

namespace {

constexpr char kUnknownDigest[] =
    "0000000000000000000000000000000000000000000000000000000000000000";

std::string uuid_v4() {
    std::random_device device;
    std::mt19937_64 generator(device());
    std::uint64_t high = generator();
    std::uint64_t low = generator();

    // Version 4, variant RFC 4122.
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
                  utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday, utc.tm_hour,
                  utc.tm_min, utc.tm_sec);
    return std::string(buffer);
}

std::int64_t steady_ns() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

mh::json::Json object() { return mh::json::Json::object(); }

mh::json::Json unknown_field(std::string key) {
    mh::json::Json field = object();
    field.set(std::move(key), mh::json::Json::null());
    return field;
}

// The fields the runner owns. Null here, named in capture.missing, never guessed.
void mark_unobserved(mh::Capture& capture, std::string path, std::string reason) {
    capture.missing.push_back(path);
    capture.reasons.emplace_back(std::move(path), std::move(reason));
}

mh::json::Json build_meta_body(std::string_view geode_version, bool target_loaded) {
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
    geode.set("version", mh::json::Json::string(std::string(geode_version)));
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
    if (target_loaded) {
        mh::json::Json target_mod = object();
        target_mod.set("id", mh::json::Json::string("absolllute.hackmega"));
        target_mod.set("role", mh::json::Json::string("target"));
        target_mod.set("version", mh::json::Json::null());
        target_mod.set("sha256", mh::json::Json::null());
        mods.push(std::move(target_mod));
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

    return body;
}

bool target_is_loaded() {
    for (Mod* mod : Loader::get()->getAllMods()) {
        if (mod && mod->getID() == "absolllute.hackmega") {
            return true;
        }
    }
    return false;
}

} // namespace

$on_mod(Loaded) {
    const std::int64_t started_ns = steady_ns();
    const std::string run_id = uuid_v4();

    std::filesystem::path directory = Mod::get()->getSaveDir() / "traces";
    std::error_code ec;
    std::filesystem::create_directories(directory, ec);
    if (ec) {
        log::error("cannot create trace directory: {}", ec.message());
        return;
    }

    std::filesystem::path path = directory / (run_id + ".jsonl");
    std::ofstream sink = mh::open_trace_file(path.string());
    if (!sink) {
        log::error("cannot open trace file: {}", path.string());
        return;
    }

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

    const std::string geode_version = Loader::get()->getVersion().toNonVString();
    mh::TraceWriter writer(sink, {run_id, kUnknownDigest});
    writer.write_meta(build_meta_body(geode_version, target_is_loaded()), capture);

    mh::EndInfo end;
    end.status = mh::EndStatus::stopped;
    end.exit_code = 0;
    end.frames = 0;
    end.t_ns = steady_ns() - started_ns;
    end.duration_ns = end.t_ns;
    end.unclosed = 0;
    end.capture.status = mh::CaptureStatus::partial;
    end.capture.missing = {"artifacts.stdout_sha256"};
    end.capture.reasons.emplace_back("artifacts.stdout_sha256", "the observer does not capture output");
    writer.write_end(end);

    sink.flush();
    log::info("trace written to {} (run {})", path.string(), run_id);
}
