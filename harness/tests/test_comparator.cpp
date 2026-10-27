// Item 8 — comparator core: canonical event keys, volatile normalisation, comparability
// gate.
//
// Contract: notes/runtime-validation.md §Comparison rules 1 (canonical keys table and the
// volatility paragraph), §Comparability gate, §9 result statuses.

#include <string>
#include <string_view>
#include <vector>

#include "fixtures/example_scenario.hpp"
#include "fixtures/example_trace.hpp"
#include "mh/comparator.hpp"
#include "mh/json_parse.hpp"
#include "mh/scenario.hpp"
#include "test_runner.hpp"

namespace {

using mh::GateResult;
using mh::GateStatus;
using mh::VariantSummary;

mh::json::Json parse_one(const std::string& text) {
  mh::json::ParseResult result = mh::json::parse(text);
  if (!result.ok()) {
    return mh::json::Json::null();
  }
  return *result.value;
}

// The manifest is `draft` but structurally valid, which is what the gate consumes.
//
// The contract's example manifest ships placeholder (all-zero) digests, and the gate now calls
// a placeholder *missing evidence* rather than a mismatch. The identity rows under test here
// are about comparing hashes, so a placeholder is replaced with a stable real-looking digest;
// the placeholder rule itself has its own tests below.
mh::ScenarioDocument example_scenario() {
  mh::ScenarioDocument document =
      mh::read_scenario(std::string{mh_test_fixtures::kExampleScenario});
  if (document.scenario.has_value()) {
    const auto fill = [](std::string& digest, char digit) {
      if (mh::is_placeholder_digest(digest)) {
        digest = std::string(64, digit);
      }
    };
    fill(document.scenario->release_sha256, 'a');
    fill(document.scenario->reconstruction_sha256, 'b');
    fill(document.scenario->harness_sha256, 'c');
    fill(document.scenario->level_sha256, 'd');
  }
  return document;
}

VariantSummary variant(std::string role, std::string target_sha256,
                       const mh::ScenarioDocument& manifest) {
  const mh::Scenario& scenario = *manifest.scenario;
  VariantSummary summary;
  summary.role = std::move(role);
  summary.target_sha256 = std::move(target_sha256);
  summary.harness_id = scenario.harness_id;
  summary.harness_version = scenario.harness_version;
  summary.harness_sha256 = scenario.harness_sha256;
  summary.gd_version = "2.2081";
  summary.geode_version = "5.11.2";
  summary.level_sha256 = scenario.level_sha256;
  summary.level_source = scenario.level_source;
  summary.scenario_id = scenario.id;
  summary.scenario_sha256 = manifest.sha256;
  summary.settings_tags = scenario.settings_tags;
  summary.runs = 2;
  // A run that observed the manifest's fixture. The observation row has its own tests below;
  // this helper is the "everything matches" baseline the identity rows are tested against.
  summary.level_observed = {scenario.level_sha256, scenario.level_sha256};
  return summary;
}

bool has_check(const GateResult& result, std::string_view check) {
  for (const mh::GateIssue& issue : result.issues) {
    if (issue.check == check) {
      return true;
    }
  }
  return false;
}

} // namespace

// --- Canonical event keys (§Comparison rules 1, table) ------------------------------

MH_TEST(comparator_canonical_key_for_a_hook_is_observer_key_phase_depth) {
  const mh::json::Json event = parse_one(
      R"({"kind":"hook","phase":"enter",)"
      R"("hook":{"observer":"gd_method","key":"GeometryDash.exe:rva:0x3b8eb0"},)"
      R"("call":{"call_id":"c1","depth":1}})");

  MH_CHECK(mh::canonical_event_key(event).has_value());
  MH_CHECK_EQ(*mh::canonical_event_key(event),
              std::string{"gd_method|GeometryDash.exe:rva:0x3b8eb0|enter|1"});
}

MH_TEST(comparator_canonical_key_for_a_milestone_is_the_name) {
  const mh::json::Json event = parse_one(R"({"kind":"milestone","name":"level_started"})");
  MH_CHECK_EQ(*mh::canonical_event_key(event), std::string{"milestone|level_started"});
}

MH_TEST(comparator_canonical_key_for_a_save_is_store_phase_source) {
  const mh::json::Json event = parse_one(
      R"({"kind":"save","store":"hackpro","phase":"snapshot_after","source":"filesystem"})");
  MH_CHECK_EQ(*mh::canonical_event_key(event),
              std::string{"save|hackpro|snapshot_after|filesystem"});
}

MH_TEST(comparator_canonical_key_for_an_error_is_the_code) {
  const mh::json::Json event = parse_one(R"({"kind":"error","code":"timeout"})");
  MH_CHECK_EQ(*mh::canonical_event_key(event), std::string{"error|timeout"});
}

MH_TEST(comparator_canonical_key_is_null_when_the_payload_is_incomplete) {
  // A key must never be invented: "Do not use a runtime VA, timestamp, or thread id in a
  // key", and an incomplete payload has no identity at all.
  MH_CHECK(!mh::canonical_event_key(parse_one(R"({"kind":"hook"})")).has_value());
  MH_CHECK(!mh::canonical_event_key(parse_one(R"({"kind":"milestone"})")).has_value());
  MH_CHECK(!mh::canonical_event_key(parse_one(R"({"kind":"telemetry"})")).has_value());
  MH_CHECK(!mh::canonical_event_key(parse_one(R"({"kind":"save","store":"hackpro"})"))
                .has_value());
}

// --- Normalisation (§Comparison rules 1, first paragraph) ---------------------------

MH_TEST(comparator_normalisation_removes_the_documented_volatile_fields) {
  const mh::json::Json event = parse_one(
      R"({"schema":"mh-trace/1","type":"event","seq":4,"t_ns":250,"frame":1,)"
      R"("thread":{"id":"main","role":"main"},)"
      R"("kind":"hook","phase":"enter",)"
      R"("hook":{"observer":"gd_method","key":"GeometryDash.exe:rva:0x3b8eb0",)"
      R"("va":"0x7FF6A1B8EB0","module":"GeometryDash.exe","method":"PlayLayer::resetLevel",)"
      R"("resolution":"rva","rva":"0x3B8EB0","target_name":null,)"
      R"("target":{"module":"absolllute.hackmega_original.dll","callback_va":"0x180199E80"}},)"
      R"("call":{"call_id":"c1","depth":1,"this":{"state":"observed","type":"ptr","value":"0x1F4A2B00000"},)"
      R"("args":{"state":"observed","items":[]},"return":{"state":"null"}}})");

  const mh::json::Json normalized = mh::normalize_event(event);
  const std::string text = normalized.to_json();

  MH_CHECK(text.find("\"t_ns\"") == std::string::npos);
  MH_CHECK(text.find("\"va\"") == std::string::npos);
  MH_CHECK(text.find("\"callback_va\"") == std::string::npos);
  MH_CHECK(text.find("\"target\"") == std::string::npos);
  MH_CHECK(text.find("\"id\":\"main\"") == std::string::npos);
  MH_CHECK(text.find("0x1F4A2B00000") == std::string::npos);

  // What is behaviour survives: the observed method, the depth, the args and the thread
  // role (needed for ordered groups).
  MH_CHECK(text.find("GeometryDash.exe:rva:0x3b8eb0") != std::string::npos);
  MH_CHECK(text.find("\"depth\":1") != std::string::npos);
  MH_CHECK(text.find("\"role\":\"main\"") != std::string::npos);
}

MH_TEST(comparator_normalisation_removes_call_duration_and_save_local_path) {
  const mh::json::Json hook = parse_one(
      R"({"kind":"hook","phase":"exit","call":{"call_id":"c1","depth":1,"duration_ns":50,)"
      R"("return":{"state":"null"}}})");
  MH_CHECK(mh::normalize_event(hook).to_json().find("duration_ns") == std::string::npos);

  const mh::json::Json save = parse_one(
      R"({"kind":"save","store":"hackpro","phase":"read","source":"filesystem",)"
      R"("path_template":"<mod_config>/hackpro.json","local_path":"C:/Users/x/AppData/hackpro.json"})");
  const std::string text = mh::normalize_event(save).to_json();
  MH_CHECK(text.find("local_path") == std::string::npos);
  MH_CHECK(text.find("<mod_config>/hackpro.json") != std::string::npos);
}

MH_TEST(comparator_normalisation_applies_the_manifest_volatile_list) {
  const mh::json::Json event = parse_one(R"({"kind":"milestone","name":"run_end","note":"x"})");

  // §8: a manifest may add a field to `comparison.volatile` only after it was observed to
  // vary within a variant. The normaliser takes that list as input.
  const std::string text = mh::normalize_event(event, {"note"}).to_json();
  MH_CHECK(text.find("\"note\"") == std::string::npos);
  MH_CHECK(text.find("run_end") != std::string::npos);
}

MH_TEST(comparator_normalisation_ignores_paths_that_do_not_exist) {
  const mh::json::Json event = parse_one(R"({"kind":"milestone","name":"run_end"})");
  MH_CHECK_EQ(mh::normalize_event(event, {"no.such.path"}).to_json(), event.to_json());
}

// --- Comparability gate (§Comparability gate, §9) ----------------------------------

MH_TEST(comparator_gate_passes_when_every_row_matches) {
  const mh::ScenarioDocument manifest = example_scenario();
  const VariantSummary release = variant("release", manifest.scenario->release_sha256, manifest);
  const VariantSummary reconstruction =
      variant("reconstruction", manifest.scenario->reconstruction_sha256, manifest);

  const GateResult result = mh::check_comparability(manifest, release, reconstruction);
  MH_CHECK_EQ(result.status, GateStatus::pass);
  MH_CHECK(result.passed());
  MH_CHECK(result.issues.empty());
}

MH_TEST(comparator_gate_invalidates_a_different_scenario) {
  const mh::ScenarioDocument manifest = example_scenario();
  VariantSummary release = variant("release", manifest.scenario->release_sha256, manifest);
  VariantSummary reconstruction =
      variant("reconstruction", manifest.scenario->reconstruction_sha256, manifest);
  reconstruction.scenario_sha256 = std::string(64, '1');

  const GateResult result = mh::check_comparability(manifest, release, reconstruction);
  MH_CHECK_EQ(result.status, GateStatus::invalid);
  MH_CHECK(has_check(result, "scenario"));

  // Same scenario hash, different id: still not the same scenario.
  release.scenario_sha256 = manifest.sha256;
  VariantSummary other =
      variant("reconstruction", manifest.scenario->reconstruction_sha256, manifest);
  other.scenario_id = "some-other-scenario";
  MH_CHECK(has_check(mh::check_comparability(manifest, release, other), "scenario"));
}

MH_TEST(comparator_gate_invalidates_a_different_harness) {
  // "A different harness build is a different observer and invalidates the comparison even
  // if the target hash is unchanged."
  const mh::ScenarioDocument manifest = example_scenario();
  const VariantSummary release = variant("release", manifest.scenario->release_sha256, manifest);
  VariantSummary reconstruction =
      variant("reconstruction", manifest.scenario->reconstruction_sha256, manifest);
  reconstruction.harness_sha256 = std::string(64, '2');

  const GateResult result = mh::check_comparability(manifest, release, reconstruction);
  MH_CHECK_EQ(result.status, GateStatus::invalid);
  MH_CHECK(has_check(result, "harness"));
}

MH_TEST(comparator_gate_invalidates_a_different_runtime) {
  const mh::ScenarioDocument manifest = example_scenario();
  const VariantSummary release = variant("release", manifest.scenario->release_sha256, manifest);
  VariantSummary reconstruction =
      variant("reconstruction", manifest.scenario->reconstruction_sha256, manifest);
  reconstruction.geode_version = "5.6.1";

  const GateResult result = mh::check_comparability(manifest, release, reconstruction);
  MH_CHECK_EQ(result.status, GateStatus::invalid);
  MH_CHECK(has_check(result, "runtime"));
}

MH_TEST(comparator_gate_invalidates_a_different_level) {
  const mh::ScenarioDocument manifest = example_scenario();
  const VariantSummary release = variant("release", manifest.scenario->release_sha256, manifest);
  VariantSummary reconstruction =
      variant("reconstruction", manifest.scenario->reconstruction_sha256, manifest);
  reconstruction.level_sha256 = std::string(64, '3');

  const GateResult result = mh::check_comparability(manifest, release, reconstruction);
  MH_CHECK_EQ(result.status, GateStatus::invalid);
  MH_CHECK(has_check(result, "level"));
}

MH_TEST(comparator_gate_invalidates_a_level_observation_that_is_not_the_fixture) {
  // The row that turns "the manifest agrees with itself" into evidence: every run must have
  // started the level whose digest the manifest pins.
  const mh::ScenarioDocument manifest = example_scenario();
  const VariantSummary release = variant("release", manifest.scenario->release_sha256, manifest);
  VariantSummary reconstruction =
      variant("reconstruction", manifest.scenario->reconstruction_sha256, manifest);
  reconstruction.level_observed[0] = std::string(64, 'e');

  const GateResult result = mh::check_comparability(manifest, release, reconstruction);
  MH_CHECK_EQ(result.status, GateStatus::invalid);
  MH_CHECK(has_check(result, "level_observed"));
}

MH_TEST(comparator_gate_blocks_when_a_run_did_not_observe_the_level_digest) {
  // Absence of an observation is not agreement, so it is `blocked`, not `invalid`.
  const mh::ScenarioDocument manifest = example_scenario();
  const VariantSummary release = variant("release", manifest.scenario->release_sha256, manifest);
  VariantSummary reconstruction =
      variant("reconstruction", manifest.scenario->reconstruction_sha256, manifest);
  reconstruction.level_observed[1].clear();

  const GateResult result = mh::check_comparability(manifest, release, reconstruction);
  MH_CHECK_EQ(result.status, GateStatus::blocked);
  MH_CHECK(has_check(result, "level_observed"));

  // A variant with no runs at all is equally unobserved, not silently passing.
  VariantSummary empty =
      variant("reconstruction", manifest.scenario->reconstruction_sha256, manifest);
  empty.level_observed.clear();
  MH_CHECK(has_check(mh::check_comparability(manifest, release, empty), "level_observed"));
}

MH_TEST(comparator_gate_invalidates_different_settings_tags) {
  const mh::ScenarioDocument manifest = example_scenario();
  const VariantSummary release = variant("release", manifest.scenario->release_sha256, manifest);
  VariantSummary reconstruction =
      variant("reconstruction", manifest.scenario->reconstruction_sha256, manifest);

  mh::json::Json tags = mh::json::Json::array();
  mh::json::Json tag = mh::json::Json::object();
  tag.set("tag", mh::json::Json::string("0x1E2"));
  tag.set("value", mh::json::Json::boolean(false));
  tags.push(tag);
  reconstruction.settings_tags = tags;

  const GateResult result = mh::check_comparability(manifest, release, reconstruction);
  MH_CHECK_EQ(result.status, GateStatus::invalid);
  MH_CHECK(has_check(result, "settings"));
}

MH_TEST(comparator_gate_accepts_reordered_settings_tags) {
  // "Same semantic settings.tags list" — two files listing the same tags in a different
  // order describe the same settings.
  const mh::ScenarioDocument manifest = example_scenario();
  VariantSummary release = variant("release", manifest.scenario->release_sha256, manifest);
  VariantSummary reconstruction =
      variant("reconstruction", manifest.scenario->reconstruction_sha256, manifest);

  mh::json::Json first = mh::json::Json::object();
  first.set("tag", mh::json::Json::string("0x1E2"));
  first.set("value", mh::json::Json::boolean(true));
  mh::json::Json second = mh::json::Json::object();
  second.set("tag", mh::json::Json::string("0x405"));
  second.set("value", mh::json::Json::boolean(true));

  mh::json::Json forward = mh::json::Json::array();
  forward.push(first);
  forward.push(second);
  mh::json::Json reverse = mh::json::Json::array();
  reverse.push(second);
  reverse.push(first);

  release.settings_tags = forward;
  reconstruction.settings_tags = reverse;

  MH_CHECK_EQ(mh::check_comparability(manifest, release, reconstruction).status,
              GateStatus::pass);
}

MH_TEST(comparator_gate_blocks_a_single_run) {
  // One run per variant is missing evidence, not contradicting evidence: §9's `invalid` is for
  // a hash or version that disagrees, and `blocked` is for a required observation that is not
  // there. This used to report `invalid`, which read as "the run was wrong".
  const mh::ScenarioDocument manifest = example_scenario();
  VariantSummary release = variant("release", manifest.scenario->release_sha256, manifest);
  const VariantSummary reconstruction =
      variant("reconstruction", manifest.scenario->reconstruction_sha256, manifest);
  release.runs = 1;

  const GateResult result = mh::check_comparability(manifest, release, reconstruction);
  MH_CHECK_EQ(result.status, GateStatus::blocked);
  MH_CHECK(has_check(result, "runs"));
}

MH_TEST(comparator_gate_invalidates_a_crashed_run) {
  const mh::ScenarioDocument manifest = example_scenario();
  const VariantSummary release = variant("release", manifest.scenario->release_sha256, manifest);
  VariantSummary reconstruction =
      variant("reconstruction", manifest.scenario->reconstruction_sha256, manifest);
  reconstruction.crashed_runs = 1;

  const GateResult result = mh::check_comparability(manifest, release, reconstruction);
  MH_CHECK_EQ(result.status, GateStatus::invalid);
  MH_CHECK(has_check(result, "runs"));
}

MH_TEST(comparator_gate_blocks_when_a_variant_is_absent) {
  // A comparison with no reconstruction runs at all is blocked on evidence, not invalid: the
  // identity rows have nothing to compare, so they must stay silent instead of each reporting a
  // mismatch against a variant that does not exist.
  const mh::ScenarioDocument manifest = example_scenario();
  const VariantSummary release = variant("release", manifest.scenario->release_sha256, manifest);
  VariantSummary reconstruction =
      variant("reconstruction", manifest.scenario->reconstruction_sha256, manifest);
  reconstruction.runs = 0;

  const GateResult result = mh::check_comparability(manifest, release, reconstruction);
  MH_CHECK_EQ(result.status, GateStatus::blocked);
  MH_CHECK(has_check(result, "runs"));
  MH_CHECK(!has_check(result, "scenario"));
  MH_CHECK(!has_check(result, "harness"));
  MH_CHECK(!has_check(result, "target"));
  MH_CHECK_EQ(result.issues.size(), std::size_t{1});
}

MH_TEST(comparator_gate_blocks_on_dropped_events) {
  const mh::ScenarioDocument manifest = example_scenario();
  const VariantSummary release = variant("release", manifest.scenario->release_sha256, manifest);
  VariantSummary reconstruction =
      variant("reconstruction", manifest.scenario->reconstruction_sha256, manifest);
  reconstruction.dropped_events = 1;

  const GateResult result = mh::check_comparability(manifest, release, reconstruction);
  MH_CHECK_EQ(result.status, GateStatus::blocked);
  MH_CHECK(has_check(result, "completeness"));
}

MH_TEST(comparator_gate_blocks_on_an_unclosed_call) {
  const mh::ScenarioDocument manifest = example_scenario();
  VariantSummary release = variant("release", manifest.scenario->release_sha256, manifest);
  VariantSummary reconstruction =
      variant("reconstruction", manifest.scenario->reconstruction_sha256, manifest);
  release.unclosed_calls = 1;

  MH_CHECK_EQ(mh::check_comparability(manifest, release, reconstruction).status,
              GateStatus::blocked);
}

MH_TEST(comparator_gate_blocks_on_an_uncaptured_required_field) {
  const mh::ScenarioDocument manifest = example_scenario();
  const VariantSummary release = variant("release", manifest.scenario->release_sha256, manifest);
  VariantSummary reconstruction =
      variant("reconstruction", manifest.scenario->reconstruction_sha256, manifest);
  reconstruction.unavailable_required_fields = 2;

  MH_CHECK_EQ(mh::check_comparability(manifest, release, reconstruction).status,
              GateStatus::blocked);
}

// --- Placeholder digests: missing evidence, not a contradiction ---------------------

MH_TEST(comparator_placeholder_digest_is_64_zeros_and_nothing_else) {
  MH_CHECK(mh::is_placeholder_digest(std::string(64, '0')));
  MH_CHECK(!mh::is_placeholder_digest(std::string(64, 'a')));
  MH_CHECK(!mh::is_placeholder_digest(std::string(63, '0')));
  MH_CHECK(!mh::is_placeholder_digest(""));
  MH_CHECK(!mh::is_placeholder_digest("0000000000000000000000000000000000000000000000000000000000000001"));
}

MH_TEST(comparator_gate_blocks_a_placeholder_reconstruction_digest) {
  // The allocation the manifest ships: a real release hash, a zeroed reconstruction hash. The
  // run is not wrong, the manifest is simply not enrolled against the built artifact.
  mh::ScenarioDocument manifest = example_scenario();
  manifest.scenario->reconstruction_sha256 = std::string(64, '0');
  const VariantSummary release = variant("release", manifest.scenario->release_sha256, manifest);
  const VariantSummary reconstruction =
      variant("reconstruction", std::string(64, '0'), manifest);

  const GateResult result = mh::check_comparability(manifest, release, reconstruction);
  MH_CHECK_EQ(result.status, GateStatus::blocked);
  MH_CHECK(!has_check(result, "scenario"));
  bool named = false;
  for (const mh::GateIssue& issue : result.issues) {
    if (issue.check == "target" && issue.detail.find("placeholder") != std::string::npos) {
      named = true;
    }
  }
  MH_CHECK(named);
}

MH_TEST(comparator_gate_blocks_a_placeholder_harness_digest) {
  mh::ScenarioDocument manifest = example_scenario();
  manifest.scenario->harness_sha256 = std::string(64, '0');
  const VariantSummary release = variant("release", manifest.scenario->release_sha256, manifest);
  const VariantSummary reconstruction =
      variant("reconstruction", manifest.scenario->reconstruction_sha256, manifest);

  const GateResult result = mh::check_comparability(manifest, release, reconstruction);
  MH_CHECK_EQ(result.status, GateStatus::blocked);
  MH_CHECK(has_check(result, "harness"));
}

MH_TEST(comparator_gate_blocks_a_level_row_where_both_sides_are_placeholders) {
  // The one row an inequality cannot catch: zeros equal zeros, so without this rule the row
  // would "pass" having compared nothing at all.
  mh::ScenarioDocument manifest = example_scenario();
  manifest.scenario->level_sha256 = std::string(64, '0');
  VariantSummary release = variant("release", manifest.scenario->release_sha256, manifest);
  VariantSummary reconstruction =
      variant("reconstruction", manifest.scenario->reconstruction_sha256, manifest);
  release.level_sha256 = std::string(64, '0');
  reconstruction.level_sha256 = std::string(64, '0');

  const GateResult result = mh::check_comparability(manifest, release, reconstruction);
  MH_CHECK_EQ(result.status, GateStatus::blocked);
  MH_CHECK(has_check(result, "level"));
}

MH_TEST(comparator_gate_still_invalidates_two_real_digests_that_differ) {
  // The placeholder rule must not launder a genuine mismatch.
  const mh::ScenarioDocument manifest = example_scenario();
  const VariantSummary release = variant("release", manifest.scenario->release_sha256, manifest);
  VariantSummary reconstruction =
      variant("reconstruction", manifest.scenario->reconstruction_sha256, manifest);
  reconstruction.target_sha256 = std::string(64, '9');

  const GateResult result = mh::check_comparability(manifest, release, reconstruction);
  MH_CHECK_EQ(result.status, GateStatus::invalid);
}

MH_TEST(comparator_gate_summarises_a_variant_from_traces) {
  // One structurally valid run, so the summary is complete rather than crashed. `runs`
  // counts what was handed in, which is what the "at least two" row needs.
  const std::vector<mh::TraceDocument> runs = {
      mh::read_trace(std::string{mh_test_fixtures::kExampleTrace})};
  const VariantSummary summary = mh::summarize_variant("release", runs);

  MH_CHECK_EQ(summary.runs, std::int64_t{1});
  MH_CHECK_EQ(summary.crashed_runs, std::int64_t{0});
  MH_CHECK_EQ(summary.gd_version, std::string{"2.2081"});
  MH_CHECK_EQ(summary.geode_version, std::string{"5.11.2"});
  MH_CHECK_EQ(summary.harness_id, std::string{"megahack.trace"});
  MH_CHECK_EQ(summary.scenario_id, std::string{"safe-mode-smoke"});
  MH_CHECK_EQ(summary.target_sha256,
              std::string{"1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92"});
  // This fixture's `level_started` carries the model's null value, which is "nothing was
  // observed" - not an observation of an empty level.
  MH_CHECK_EQ(summary.level_observed.size(), std::size_t{1});
  MH_CHECK(summary.level_observed.front().empty());
}

MH_TEST(comparator_gate_summary_counts_an_incomplete_trace_as_crashed) {
  std::vector<mh::TraceDocument> runs = {
      mh::read_trace(std::string{mh_test_fixtures::kExampleTrace})};
  // A trace with no end is incomplete, not a short run.
  const std::string truncated = std::string{mh_test_fixtures::kExampleTrace}.substr(0, 4000);
  runs.push_back(mh::read_trace(truncated));

  const VariantSummary summary = mh::summarize_variant("release", runs);
  MH_CHECK_EQ(summary.runs, std::int64_t{2});
  MH_CHECK_EQ(summary.crashed_runs, std::int64_t{1});
}
