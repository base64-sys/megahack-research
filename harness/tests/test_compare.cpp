// What a pass proves (§9's statuses do not say, so the manifest does).
//
// The rule under test: `behavioural` only when the manifest *requires* an observation the
// target alone can produce, and both variants actually contain it. Everything else is
// `surface`, including a manifest that asks for such a site and a run that never triggered it.

#include <cstdint>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "mh/compare.hpp"
#include "mh/comparison.hpp"
#include "mh/json.hpp"
#include "mh/records.hpp"
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

// `save_site_required` moves 0x16c8b0 between `hooks.required` and `hooks.optional`; nothing
// else in the manifest changes, so the two cases differ by exactly the decision under test.
// `comparison.volatile` is the contract's escape hatch for a field that has been *observed*
// to vary between runs of one build; `volatile_entry` is the JSON text of one such path.
std::string manifest_text(bool save_site_required, std::string volatile_entry = "") {
  std::string text = R"JSON({
  "schema": "mh-scenario/1",
  "id": "claim",
  "status": "draft",
  "description": "Claim-class derivation.",
  "baseline": {"mega_hack": "v9.1.3", "gd": "2.2081", "geode": "5.6.1", "arch": "x86_64"},
  "runtime_baseline": {"geode": "5.11.2", "profile": "runtime511"},
  "targets": {"release": {"artifact": "release.dll", "sha256": "@RELEASE_SHA@"}, "reconstruction": {"artifact": "recon.dll", "sha256": "@RECON_SHA@"}},
  "harness": {"id": "megahack.trace", "version": "0.1.0", "sha256": "@HARNESS_SHA@"},
  "mods": {"required": ["megahack.trace"], "dependency_versions": {}, "other_mods": "forbidden"},
  "level": {"fixture": "fixtures/claim.gmd", "sha256": "@LEVEL_SHA@", "source": "generated"},
  "profile": {"isolation": "disposable", "notes": ""},
  "settings": {"reset": "defaults", "tags": [{"tag": "0x1E2", "value": true}]},
  "input": {"driver": "harness", "actions": [{"at_frame": 30, "action": "press", "key": "space"}]},
  "timing": {"max_frames": 240, "timeout_ms": 30000, "fps_cap": 60, "stop_on": ["level_completed"]},
  "milestones": {"required": ["level_started"], "optional": []},
  "hooks": {"required": [{"key": "GeometryDash.exe:rva:0x3b8eb0", "phases": ["enter", "exit"], "min_count": 1}@SAVE_REQUIRED@],
            "optional": [{"key": "GeometryDash.exe:rva:0x16c8b0", "phases": ["enter", "exit"], "min_count": 1}]},
  "repetitions": 2,
  "comparison": {"order_groups": [["level_started"]], "tolerances": {}, "volatile": [@VOLATILE@]}
})JSON";
  replace_all(text, "@SAVE_REQUIRED@",
              save_site_required
                  ? R"(, {"key": "GeometryDash.exe:rva:0x16c8b0", "phases": ["enter", "exit"], "min_count": 1})"
                  : "");
  replace_all(text, "@VOLATILE@", volatile_entry);
  replace_all(text, "@RELEASE_SHA@", sha('a'));
  replace_all(text, "@RECON_SHA@", sha('b'));
  replace_all(text, "@HARNESS_SHA@", sha('c'));
  replace_all(text, "@LEVEL_SHA@", sha('d'));
  return text;
}

struct TraceOptions {
  std::string role = "release";
  bool emit_save_site = false;  // the 0x16c8b0 hook pair and its snapshots
  // `frame` is `CCDirector::getTotalFrames()`, counted since the process started, so a later
  // launch of the same build starts its window at a different absolute frame. `frame_base`
  // moves the whole document; `hook_frame_shift` moves only the hook events, so a real
  // one-frame difference can be told apart from a shifted clock.
  std::int64_t frame_base = 0;
  std::int64_t hook_frame_shift = 0;
  // The observer's per-invocation counter, not behaviour: two runs of one build allocate
  // these in whatever order their hooks happen to run.
  std::string call_id = "c1";
  // The JSON text of the `comparison.volatile` entries this trace's own manifest carries,
  // so the trace's `scenario.sha256` matches the manifest it is compared against.
  std::string manifest_volatile;
};

mh::EventContext context(std::int64_t t_ns, std::int64_t frame) {
  mh::EventContext ctx;
  ctx.t_ns = t_ns;
  ctx.frame = frame;
  ctx.thread.id = "main";
  ctx.thread.role = mh::ThreadRole::main;
  return ctx;
}

mh::records::HookIdentity reset_level() {
  return mh::records::HookIdentity{"GeometryDash.exe:rva:0x3b8eb0", "PlayLayer::resetLevel",
                                   "GeometryDash.exe", 0x3B8EB0, 0x7FF6A1B8EB0};
}

mh::records::HookIdentity save_percentage() {
  return mh::records::HookIdentity{"GeometryDash.exe:rva:0x16c8b0",
                                   "GJGameLevel::savePercentage", "GeometryDash.exe", 0x16C8B0,
                                   0x7FF6A16C8B0};
}

Json meta_body(const TraceOptions& options, const mh::ScenarioDocument& manifest) {
  Json meta = Json::object();

  Json target = Json::object();
  target.set("role", Json::string(options.role));
  target.set("artifact", Json::string(options.role == "release" ? "release.dll" : "recon.dll"));
  target.set("sha256", Json::string(options.role == "release" ? sha('a') : sha('b')));
  target.set("size", Json::integer(1048576));
  target.set("image_base", Json::string("0x180000000"));
  meta.set("target", target);

  Json scenario = Json::object();
  scenario.set("id", Json::string(manifest.scenario->id));
  scenario.set("sha256", Json::string(manifest.sha256));
  meta.set("scenario", scenario);

  Json gd = Json::object();
  gd.set("version", Json::string("2.2081"));
  gd.set("sha256", Json::string(sha('e')));
  Json geode = Json::object();
  geode.set("version", Json::string("5.11.2"));
  geode.set("sha256", Json::string(sha('f')));
  Json harness = Json::object();
  harness.set("id", Json::string("megahack.trace"));
  harness.set("version", Json::string("0.1.0"));
  harness.set("sha256", Json::string(sha('c')));
  Json environment = Json::object();
  environment.set("os", Json::string("Windows"));
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
  level.set("fixture", Json::string("fixtures/claim.gmd"));
  level.set("sha256", Json::string(sha('d')));
  level.set("source", Json::string("generated"));
  meta.set("level", level);

  return meta;
}

std::string build_trace(const TraceOptions& options, bool save_site_required) {
  const mh::ScenarioDocument manifest =
      mh::read_scenario(manifest_text(save_site_required, options.manifest_volatile));

  std::ostringstream out;
  mh::TraceWriter writer(out, mh::TraceHeader{"00000000-0000-4000-8000-000000000002",
                                              sha('0')});
  writer.write_meta(meta_body(options, manifest), mh::Capture{});

  const std::int64_t anchor_frame = options.frame_base;
  const std::int64_t hook_frame = options.frame_base + 1 + options.hook_frame_shift;

  writer.write_event(
      mh::records::milestone_event("level_started",
                                   mh::Value::observed_str(sha('d'))),
      context(100, anchor_frame));
  writer.write_event(
      mh::records::hook_event("enter", reset_level(), Json::null(),
                              mh::records::call_object(options.call_id, 1, std::nullopt, 0)),
      context(150, hook_frame));
  writer.write_event(
      mh::records::hook_event("exit", reset_level(), Json::null(),
                              mh::records::call_object(options.call_id, 1, std::nullopt, 0, 900)),
      context(160, hook_frame));
  if (options.emit_save_site) {
    writer.write_event(
        mh::records::hook_event("enter", save_percentage(), Json::null(),
                                mh::records::call_object("c2", 1, std::nullopt, 0)),
        context(170, hook_frame));
    writer.write_event(
        mh::records::save_event("snapshot_before", false, std::nullopt, std::nullopt, "saves/l.dat"),
        context(171, hook_frame));
    writer.write_event(
        mh::records::save_event("snapshot_after", true, 12, std::string("beef"), "saves/l.dat"),
        context(172, hook_frame));
    writer.write_event(
        mh::records::hook_event("exit", save_percentage(), Json::null(),
                                mh::records::call_object("c2", 1, std::nullopt, 0, 300)),
        context(173, hook_frame));
  }

  mh::EndInfo end;
  end.t_ns = 400;
  end.frames = 4;
  end.duration_ns = 400;
  writer.write_end(end);
  return out.str();
}

std::vector<mh::TraceDocument> two_runs(const std::string& text) {
  return {mh::read_trace(text), mh::read_trace(text)};
}

mh::ComparisonReport compare(const std::vector<mh::TraceDocument>& release,
                             const std::vector<mh::TraceDocument>& reconstruction,
                             bool save_site_required, std::string volatile_entry = "") {
  return mh::compare_traces(mh::read_scenario(manifest_text(save_site_required, volatile_entry)),
                            release, reconstruction);
}

}  // namespace

MH_TEST(claim_is_surface_when_only_lifecycle_hooks_are_required) {
  MH_CHECK(mh::required_target_dependent_kinds(*mh::read_scenario(manifest_text(false)).scenario)
               .empty());

  TraceOptions release_options;
  release_options.emit_save_site = true;
  TraceOptions reconstruction_options;
  reconstruction_options.role = "reconstruction";
  reconstruction_options.emit_save_site = true;

  const mh::ComparisonReport report =
      compare(two_runs(build_trace(release_options, false)),
              two_runs(build_trace(reconstruction_options, false)), false);

  // Observed `save` records do not make a claim on their own: only a *required* site does.
  MH_CHECK_EQ(report.claim, mh::ClaimClass::surface);
}

MH_TEST(claim_is_behavioural_when_a_required_save_site_was_observed_on_both_sides) {
  const mh::ScenarioDocument manifest = mh::read_scenario(manifest_text(true));
  const std::vector<std::string> kinds = mh::required_target_dependent_kinds(*manifest.scenario);
  MH_CHECK_EQ(kinds.size(), std::size_t{1});
  MH_CHECK_EQ(kinds.front(), std::string("save"));

  TraceOptions release_options;
  release_options.emit_save_site = true;
  TraceOptions reconstruction_options;
  reconstruction_options.role = "reconstruction";
  reconstruction_options.emit_save_site = true;

  const mh::ComparisonReport report =
      compare(two_runs(build_trace(release_options, true)),
              two_runs(build_trace(reconstruction_options, true)), true);

  MH_CHECK_EQ(report.status, mh::ComparisonStatus::pass);
  MH_CHECK_EQ(report.claim, mh::ClaimClass::behavioural);
}

MH_TEST(claim_stays_surface_when_the_required_site_was_never_observed) {
  // Both variants run the site's *hook* but produce no snapshot: the manifest asked for
  // behavioural evidence and the run did not deliver it, so the claim stays where the
  // evidence is.
  TraceOptions release_options;
  TraceOptions reconstruction_options;
  reconstruction_options.role = "reconstruction";

  const mh::ComparisonReport report =
      compare(two_runs(build_trace(release_options, true)),
              two_runs(build_trace(reconstruction_options, true)), true);

  MH_CHECK_EQ(report.claim, mh::ClaimClass::surface);
}

MH_TEST(claim_is_surface_when_the_save_site_is_optional) {
  // The `safe-mode-smoke` shape: 0x16c8b0 is listed, but as optional.
  const mh::ScenarioDocument manifest = mh::read_scenario(manifest_text(false));
  MH_CHECK(mh::derive_claim(manifest, {}, {}) == mh::ClaimClass::surface);
}

MH_TEST(report_carries_the_derived_claim_even_when_the_gate_stops_it) {
  // One run per variant is not enough evidence, but the claim is still a fact about the
  // manifest and travels with the report instead of disappearing with the status.
  const std::string release = build_trace(TraceOptions{}, true);
  TraceOptions reconstruction_options;
  reconstruction_options.role = "reconstruction";
  const std::string reconstruction = build_trace(reconstruction_options, true);

  const mh::ComparisonReport report = compare({mh::read_trace(release)},
                                              {mh::read_trace(reconstruction)}, true);

  MH_CHECK_EQ(report.gate, mh::GateStatus::blocked);
  MH_CHECK_EQ(report.claim, mh::ClaimClass::surface);
}

MH_TEST(a_later_launch_of_the_same_behaviour_is_comparable) {
  // `frame` is `CCDirector::getTotalFrames()`, an absolute count since the process started
  // (the first four captures opened at frames 12507 and 37963 of the same level). Two runs
  // of one build therefore differ in every `frame` until it is rebased on the document's own
  // `level_started`; the behaviour-carrying clock is "frames since the level started".
  TraceOptions release_first;
  TraceOptions release_second;
  release_second.frame_base = 32460;
  TraceOptions reconstruction_first;
  reconstruction_first.role = "reconstruction";
  reconstruction_first.frame_base = 512;
  TraceOptions reconstruction_second;
  reconstruction_second.role = "reconstruction";
  reconstruction_second.frame_base = 37963;

  const mh::ComparisonReport report =
      compare({mh::read_trace(build_trace(release_first, false)),
               mh::read_trace(build_trace(release_second, false))},
              {mh::read_trace(build_trace(reconstruction_first, false)),
               mh::read_trace(build_trace(reconstruction_second, false))},
              false);

  MH_CHECK_EQ(report.unstable, false);
  MH_CHECK_EQ(report.status, mh::ComparisonStatus::pass);
}

MH_TEST(a_hook_that_fires_one_frame_later_is_still_unstable) {
  // The guard against the cheap fix: making `frame` volatile would also hide a real
  // one-frame difference in when a hook fired, which is the signal the bounded window exists
  // to produce. Rebasing must keep it visible.
  TraceOptions release_first;
  TraceOptions release_second;
  release_second.hook_frame_shift = 1;  // same behaviour, one frame later

  const std::string first_text = build_trace(release_first, false);
  const std::string second_text = build_trace(release_second, false);

  TraceOptions reconstruction_options;
  reconstruction_options.role = "reconstruction";
  const mh::ComparisonReport report =
      compare({mh::read_trace(first_text), mh::read_trace(second_text)},
              two_runs(build_trace(reconstruction_options, false)), false);

  MH_CHECK_EQ(report.unstable, true);
}

MH_TEST(a_volatile_path_the_manifest_declares_is_not_compared) {
  // The contract: "Record the varying field in `comparison.volatile` only after it has been
  // observed to vary, with the observed run ids." A declared path has to actually leave the
  // payload; otherwise the field is decoration and every run whose identifiers differ is
  // `unstable` for a reason no reader can act on.
  const std::string declared_entry = R"("call.call_id")";
  auto build_pair = [&](const std::string& volatile_entry) {
    TraceOptions release_first;
    release_first.manifest_volatile = volatile_entry;
    TraceOptions release_second = release_first;
    release_second.call_id = "c9";
    TraceOptions reconstruction_first;
    reconstruction_first.role = "reconstruction";
    reconstruction_first.manifest_volatile = volatile_entry;
    TraceOptions reconstruction_second = reconstruction_first;
    reconstruction_second.call_id = "c7";
    return std::pair<std::vector<mh::TraceDocument>, std::vector<mh::TraceDocument>>{
        {mh::read_trace(build_trace(release_first, false)),
         mh::read_trace(build_trace(release_second, false))},
        {mh::read_trace(build_trace(reconstruction_first, false)),
         mh::read_trace(build_trace(reconstruction_second, false))}};
  };

  // Undeclared: the identifier is part of the payload, so the variant is unstable.
  const auto undeclared = build_pair("");
  MH_CHECK_EQ(compare(undeclared.first, undeclared.second, false).unstable, true);

  // Declared: it is not compared, and the runs agree.
  const auto declared = build_pair(declared_entry);
  const mh::ComparisonReport report =
      compare(declared.first, declared.second, false, declared_entry);
  MH_CHECK_EQ(report.unstable, false);
  MH_CHECK_EQ(report.status, mh::ComparisonStatus::pass);
}
