// Item 11 — synthetic corpus: one pair per result status, plus the contract's own example.
//
// The corpus is generated through `TraceWriter` and read back through `read_trace`, so each
// case exercises the writer, the reader, the gate and the comparison in one pass rather
// than testing them against hand-written expectations. The contract's seven-line example is
// also required to validate and to round-trip byte for byte (§Complete illustrative trace).

#include <cstdint>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "fixtures/example_trace.hpp"
#include "mh/compare.hpp"
#include "mh/comparison.hpp"
#include "mh/json.hpp"
#include "mh/json_parse.hpp"
#include "mh/scenario.hpp"
#include "mh/trace_reader.hpp"
#include "mh/trace_writer.hpp"
#include "test_runner.hpp"

namespace {

using mh::json::Json;

std::string sha(char digit) { return std::string(64, digit); }

void replace_all(std::string& text, std::string_view from, std::string_view to) {
  for (std::size_t at = text.find(from); at != std::string::npos;
       at = text.find(from, at + to.size())) {
    text.replace(at, from.size(), to);
  }
}

std::string manifest_text() {
  std::string text = R"JSON({
  "schema": "mh-scenario/1",
  "id": "corpus",
  "status": "approved",
  "description": "Synthetic corpus for the comparator.",
  "baseline": {"mega_hack": "v9.1.3", "gd": "2.2081", "geode": "5.6.1", "arch": "x86_64"},
  "runtime_baseline": {"geode": "5.11.2", "profile": "runtime511"},
  "targets": {"release": {"artifact": "release.dll", "sha256": "@RELEASE_SHA@"}, "reconstruction": {"artifact": "recon.dll", "sha256": "@RECON_SHA@"}},
  "harness": {"id": "megahack.trace", "version": "0.1.0", "sha256": "@HARNESS_SHA@"},
  "mods": {"required": ["absolllute.hackmega", "megahack.trace"], "dependency_versions": {}, "other_mods": "forbidden"},
  "level": {"fixture": "fixtures/corpus.gmd", "sha256": "@LEVEL_SHA@", "source": "generated"},
  "profile": {"isolation": "disposable", "notes": ""},
  "settings": {"reset": "defaults", "tags": [{"tag": "0x1E2", "value": true}]},
  "input": {"driver": "harness", "actions": [{"at_frame": 30, "action": "press", "key": "space"}]},
  "timing": {"max_frames": 240, "timeout_ms": 30000, "fps_cap": 60, "stop_on": ["level_completed"]},
  "milestones": {"required": ["level_started"], "optional": ["level_completed"]},
  "hooks": {"required": [{"key": "GeometryDash.exe:rva:0x3b8eb0", "phases": ["enter", "exit"], "min_count": 1}],
            "optional": [{"key": "GeometryDash.exe:rva:0x16c8b0", "phases": ["enter", "exit"], "min_count": 1}]},
  "repetitions": 2,
  "comparison": {"order_groups": [["level_started"]], "tolerances": {}, "volatile": []}
})JSON";
  replace_all(text, "@RELEASE_SHA@", sha('a'));
  replace_all(text, "@RECON_SHA@", sha('b'));
  replace_all(text, "@HARNESS_SHA@", sha('c'));
  replace_all(text, "@LEVEL_SHA@", sha('d'));
  return text;
}

struct TraceOptions {
  std::string role = "release";
  std::string target_sha256;
  std::string scenario_id = "corpus";
  std::string scenario_sha256;
  std::string geode_version = "5.11.2";
  bool emit_hook = true;
  bool hook_feature_enabled = true;
  bool emit_optional_hook = false;
  bool emit_unavailable_save = false;
  std::string extra_milestone;
};

mh::EventContext context(std::int64_t t_ns, std::int64_t frame) {
  mh::EventContext ctx;
  ctx.t_ns = t_ns;
  ctx.frame = frame;
  ctx.thread.id = "main";
  ctx.thread.role = mh::ThreadRole::main;
  return ctx;
}

Json milestone_body(std::string_view name) {
  Json body = Json::object();
  body.set("kind", Json::string("milestone"));
  body.set("name", Json::string(name));
  Json data = Json::object();
  data.set("state", Json::string("null"));
  body.set("data", data);
  return body;
}

Json hook_body(std::string_view phase, std::string_view call_id, bool feature_enabled,
               std::string_view hook_key = "GeometryDash.exe:rva:0x3b8eb0",
               std::string_view rva = "0x3B8EB0",
               std::string_view va = "0x7FF6A1B8EB0",
               std::string_view method = "PlayLayer::resetLevel") {
  Json body = Json::object();
  body.set("kind", Json::string("hook"));
  body.set("phase", Json::string(phase));

  Json hook = Json::object();
  hook.set("observer", Json::string("gd_method"));
  hook.set("key", Json::string(hook_key));
  hook.set("module", Json::string("GeometryDash.exe"));
  hook.set("method", Json::string(method));
  hook.set("resolution", Json::string("rva"));
  hook.set("rva", Json::string(rva));
  hook.set("va", Json::string(va));
  hook.set("target_name", Json::null());
  body.set("hook", hook);

  Json call = Json::object();
  call.set("call_id", Json::string(call_id));
  call.set("depth", Json::integer(1));
  call.set("parent_call_id", Json::null());
  Json pointer = Json::object();
  pointer.set("state", Json::string("null"));
  call.set("this", pointer);
  Json args = Json::object();
  args.set("state", Json::string("observed"));
  args.set("items", Json::array());
  call.set("args", args);
  Json result = Json::object();
  result.set("state", Json::string("null"));
  call.set("return", result);
  body.set("call", call);

  Json feature = Json::object();
  feature.set("tag", Json::string("0x1E2"));
  feature.set("label", Json::string("HACK_SAFE_MODE"));
  feature.set("key", Json::null());
  feature.set("enabled", Json::boolean(feature_enabled));
  feature.set("source", Json::string("settings"));
  Json features = Json::array();
  features.push(feature);
  body.set("feature", features);
  return body;
}

Json optional_hook_body() {
  return hook_body("enter", "c2", true, "GeometryDash.exe:rva:0x16c8b0", "0x16C8B0",
                   "0x7FF6A16C8B0", "PlayLayer::update");
}

Json save_body() {
  Json body = Json::object();
  body.set("kind", Json::string("save"));
  body.set("store", Json::string("hackpro"));
  body.set("phase", Json::string("snapshot_after"));
  body.set("source", Json::string("filesystem"));
  body.set("path_template", Json::string("<mod_config>/hackpro.json"));
  body.set("exists", Json::boolean(true));
  body.set("size", Json::null());
  body.set("sha256", Json::null());
  body.set("keys", Json::null());
  return body;
}

Json meta_body(const TraceOptions& options) {
  Json meta = Json::object();

  Json target = Json::object();
  target.set("role", Json::string(options.role));
  target.set("artifact", Json::string(options.role == "release" ? "release.dll" : "recon.dll"));
  target.set("sha256", Json::string(options.target_sha256));
  target.set("size", Json::integer(1048576));
  target.set("image_base", Json::string("0x180000000"));
  meta.set("target", target);

  Json scenario = Json::object();
  scenario.set("id", Json::string(options.scenario_id));
  scenario.set("sha256", Json::string(options.scenario_sha256));
  meta.set("scenario", scenario);

  Json gd = Json::object();
  gd.set("version", Json::string("2.2081"));
  gd.set("sha256", Json::string(sha('e')));
  Json geode = Json::object();
  geode.set("version", Json::string(options.geode_version));
  geode.set("sha256", Json::string(sha('f')));
  Json harness = Json::object();
  harness.set("id", Json::string("megahack.trace"));
  harness.set("version", Json::string("0.1.0"));
  harness.set("sha256", Json::string(sha('c')));
  Json environment = Json::object();
  environment.set("os", Json::string("Windows 10 22H2"));
  environment.set("arch", Json::string("x86_64"));
  environment.set("gd", gd);
  environment.set("geode", geode);
  environment.set("harness", harness);
  meta.set("environment", environment);

  Json tag = Json::object();
  tag.set("tag", Json::string("0x1E2"));
  tag.set("value", Json::boolean(true));
  Json tags = Json::array();
  tags.push(tag);
  Json settings = Json::object();
  settings.set("reset", Json::string("defaults"));
  settings.set("tags", tags);
  settings.set("snapshot_sha256", Json::string(sha('1')));
  meta.set("settings", settings);

  Json level = Json::object();
  level.set("fixture", Json::string("fixtures/corpus.gmd"));
  level.set("sha256", Json::string(sha('d')));
  level.set("source", Json::string("generated"));
  meta.set("level", level);

  return meta;
}

std::string build_trace(const TraceOptions& options) {
  const mh::ScenarioDocument manifest = mh::read_scenario(manifest_text());
  TraceOptions resolved = options;
  if (resolved.target_sha256.empty()) {
    resolved.target_sha256 = sha(options.role == "release" ? 'a' : 'b');
  }
  if (resolved.scenario_sha256.empty()) {
    resolved.scenario_sha256 = manifest.sha256;
  }

  std::ostringstream out;
  mh::TraceWriter writer(out, mh::TraceHeader{"00000000-0000-4000-8000-000000000001",
                                              sha('0')});
  writer.write_meta(meta_body(resolved), mh::Capture{});

  writer.write_event(milestone_body("level_started"), context(100, 0));
  if (resolved.emit_hook) {
    writer.write_event(hook_body("enter", "c1", resolved.hook_feature_enabled),
                       context(150, 1));
    writer.write_event(hook_body("exit", "c1", resolved.hook_feature_enabled),
                       context(200, 1));
  }
  if (resolved.emit_optional_hook) {
    writer.write_event(optional_hook_body(), context(250, 1));
    writer.write_event(hook_body("exit", "c2", true, "GeometryDash.exe:rva:0x16c8b0",
                                 "0x16C8B0", "0x7FF6A16C8B0", "PlayLayer::update"),
                       context(275, 1));
  }
  if (resolved.emit_unavailable_save) {
    mh::EventContext save_context = context(300, 2);
    save_context.capture.status = mh::CaptureStatus::unavailable;
    save_context.capture.missing = {"size", "sha256", "keys"};
    save_context.capture.reasons = {
        {"size", "file locked"}, {"sha256", "file locked"}, {"keys", "file locked"}};
    writer.write_event(save_body(), save_context);
  }
  if (!resolved.extra_milestone.empty()) {
    writer.write_event(milestone_body(resolved.extra_milestone), context(350, 3));
  }

  mh::EndInfo end;
  end.t_ns = 400;
  end.frames = 4;
  end.duration_ns = 400;
  writer.write_end(end);
  return out.str();
}

TraceOptions reconstruction_default() {
  TraceOptions options;
  options.role = "reconstruction";
  return options;
}

std::vector<mh::TraceDocument> two_runs(const std::string& first, const std::string& second) {
  return {mh::read_trace(first), mh::read_trace(second)};
}

mh::ComparisonReport compare(const std::vector<mh::TraceDocument>& release,
                             const std::vector<mh::TraceDocument>& reconstruction) {
  return mh::compare_traces(mh::read_scenario(manifest_text()), release, reconstruction);
}

} // namespace

MH_TEST(corpus_pass_matches_identical_variants) {
  const std::string release = build_trace(TraceOptions{});
  const std::string reconstruction = build_trace(reconstruction_default());

  const mh::ComparisonReport report = compare(two_runs(release, release),
                                              two_runs(reconstruction, reconstruction));

  MH_CHECK_EQ(report.gate, mh::GateStatus::pass);
  MH_CHECK_EQ(report.status, mh::ComparisonStatus::pass);
  MH_CHECK(report.findings.empty());
}

MH_TEST(corpus_pass_with_warnings_for_an_optional_gap) {
  // The release calls the optional hook; the reconstruction does not. §6: "Optional event |
  // Warning".
  TraceOptions release_options;
  release_options.emit_optional_hook = true;
  const std::string release = build_trace(release_options);
  const std::string reconstruction = build_trace(reconstruction_default());

  const mh::ComparisonReport report = compare(two_runs(release, release),
                                              two_runs(reconstruction, reconstruction));

  MH_CHECK_EQ(report.status, mh::ComparisonStatus::pass_with_warnings);
  MH_CHECK(!report.findings.empty());
}

MH_TEST(corpus_fail_on_a_changed_required_payload) {
  // Same events, same keys, different observed feature state: a changed event on a required
  // key is a failure even though the shape matches.
  const std::string release = build_trace(TraceOptions{});
  TraceOptions reconstruction_options;
  reconstruction_options.role = "reconstruction";
  reconstruction_options.hook_feature_enabled = false;
  const std::string reconstruction = build_trace(reconstruction_options);

  const mh::ComparisonReport report = compare(two_runs(release, release),
                                              two_runs(reconstruction, reconstruction));

  MH_CHECK_EQ(report.status, mh::ComparisonStatus::fail);
  bool changed = false;
  for (const mh::Finding& finding : report.findings) {
    if (finding.check == "alignment" && finding.detail.find("changed") != std::string::npos) {
      changed = true;
    }
  }
  MH_CHECK(changed);
}

MH_TEST(corpus_blocked_on_an_uncaptured_required_field) {
  // The save observation could not read the file: the gate's Completeness row fails, and
  // §9 calls that `blocked`, not `fail`.
  TraceOptions options;
  options.emit_unavailable_save = true;
  const std::string release = build_trace(options);
  TraceOptions reconstruction_options = options;
  reconstruction_options.role = "reconstruction";
  const std::string reconstruction = build_trace(reconstruction_options);

  const mh::ComparisonReport report = compare(two_runs(release, release),
                                              two_runs(reconstruction, reconstruction));

  MH_CHECK_EQ(report.gate, mh::GateStatus::blocked);
  MH_CHECK_EQ(report.status, mh::ComparisonStatus::blocked);
}

MH_TEST(corpus_unstable_when_a_variant_disagrees_with_itself) {
  const std::string release = build_trace(TraceOptions{});
  TraceOptions drifted;
  drifted.extra_milestone = "level_completed";
  const std::string release_drifted = build_trace(drifted);
  const std::string reconstruction = build_trace(reconstruction_default());

  const mh::ComparisonReport report =
      compare(two_runs(release, release_drifted), two_runs(reconstruction, reconstruction));

  MH_CHECK(report.unstable);
  MH_CHECK_EQ(report.status, mh::ComparisonStatus::unstable);
}

MH_TEST(corpus_invalid_on_a_target_hash_mismatch) {
  const std::string release = build_trace(TraceOptions{});
  TraceOptions reconstruction_options;
  reconstruction_options.role = "reconstruction";
  reconstruction_options.target_sha256 = sha('9');  // not the manifest's reconstruction
  const std::string reconstruction = build_trace(reconstruction_options);

  const mh::ComparisonReport report = compare(two_runs(release, release),
                                              two_runs(reconstruction, reconstruction));

  MH_CHECK_EQ(report.gate, mh::GateStatus::invalid);
  MH_CHECK_EQ(report.status, mh::ComparisonStatus::invalid);
}

MH_TEST(corpus_example_trace_validates) {
  const mh::TraceDocument document = mh::read_trace(std::string{mh_test_fixtures::kExampleTrace});

  MH_CHECK(document.structurally_valid());
  MH_CHECK(!document.has_errors());
  MH_CHECK_EQ(document.events.size(), std::size_t{5});
}

MH_TEST(corpus_example_trace_round_trips_byte_for_byte) {
  // Parse each line and write it back in document order: the round trip must not reorder
  // keys, reformat numbers or re-escape strings.
  std::string text{mh_test_fixtures::kExampleTrace};
  std::size_t start = 0;
  std::size_t lines = 0;
  while (start < text.size()) {
    const std::size_t newline = text.find('\n', start);
    const std::string line =
        newline == std::string::npos ? text.substr(start) : text.substr(start, newline - start);
    start = newline == std::string::npos ? text.size() : newline + 1;
    if (line.empty()) {
      continue;
    }

    const mh::json::ParseResult parsed = mh::json::parse(line);
    MH_CHECK(parsed.ok());
    if (parsed.ok()) {
      MH_CHECK_EQ(parsed.value->to_json(), line);
    }
    ++lines;
  }
  MH_CHECK_EQ(lines, std::size_t{7});
}
