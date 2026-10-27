// Item 7 — scenario manifest (`mh-scenario/1`) parsing and validation.
//
// Contract: notes/runtime-validation.md §Scenario manifest, §Level and profile, §Input,
// §Timing and stopping, §Expected milestones and hooks. The positive fixture is the note's
// own example manifest (tests/fixtures/example_scenario.hpp, generated from the note),
// which is `status: draft`: it must validate, and it must not be eligible for evidence.

#include <cstddef>
#include <string>
#include <string_view>

#include "fixtures/example_scenario.hpp"
#include "mh/scenario.hpp"
#include "mh/sha256.hpp"
#include "test_runner.hpp"

namespace {

using mh::ScenarioDocument;
using mh::ScenarioIssue;
using mh::ScenarioIssueKind;
using mh::ScenarioIssueSeverity;

std::string example() { return std::string{mh_test_fixtures::kExampleScenario}; }

bool has_error(const ScenarioDocument& document, ScenarioIssueKind kind) {
  for (const ScenarioIssue& issue : document.issues) {
    if (issue.severity == ScenarioIssueSeverity::error && issue.kind == kind) {
      return true;
    }
  }
  return false;
}

bool has_warning(const ScenarioDocument& document, ScenarioIssueKind kind) {
  for (const ScenarioIssue& issue : document.issues) {
    if (issue.severity == ScenarioIssueSeverity::warning && issue.kind == kind) {
      return true;
    }
  }
  return false;
}

bool has_path(const ScenarioDocument& document, std::string_view path) {
  for (const ScenarioIssue& issue : document.issues) {
    if (issue.path == path) {
      return true;
    }
  }
  return false;
}

std::string replaced(const std::string& text, std::string_view from, std::string_view to) {
  std::string copy = text;
  const std::size_t at = copy.find(from);
  if (at != std::string::npos) {
    copy.replace(at, from.size(), to);
  }
  return copy;
}

} // namespace

MH_TEST(scenario_accepts_the_contract_example_manifest) {
  const ScenarioDocument document = mh::read_scenario(example());

  MH_CHECK(document.valid());
  MH_CHECK(!document.has_errors());
  MH_CHECK(document.scenario.has_value());
  MH_CHECK(!document.approved_for_evidence());  // status is draft on purpose

  const mh::Scenario& scenario = *document.scenario;
  MH_CHECK(scenario.status == mh::ScenarioStatus::draft);
  MH_CHECK_EQ(scenario.id, std::string{"safe-mode-smoke"});
  MH_CHECK_EQ(scenario.level_source, std::string{"generated"});
  MH_CHECK_EQ(scenario.profile_isolation, std::string{"disposable"});
  MH_CHECK_EQ(scenario.harness_id, std::string{"megahack.trace"});
  MH_CHECK_EQ(scenario.repetitions, std::int64_t{2});
  MH_CHECK_EQ(scenario.required_hooks.size(), std::size_t{4});
  MH_CHECK_EQ(scenario.optional_hooks.size(), std::size_t{1});
  MH_CHECK_EQ(scenario.required_milestones.size(), std::size_t{2});
  MH_CHECK_EQ(scenario.order_groups.size(), std::size_t{1});
  MH_CHECK(scenario.tolerances.empty());
}

MH_TEST(scenario_accepts_a_baseline_that_differs_from_the_runtime) {
  // Two Geode versions in one file is not a contradiction: `baseline` is the static analysis
  // reference (the release was read against 5.6.1 bindings) and `runtime_baseline` is what a
  // trace runs on (5.11.2). Neither is an identity row - the gate compares the traces'
  // `environment.geode.version` against each other - so editing `baseline` must not make a
  // manifest invalid, and this pins that it stays descriptive.
  const ScenarioDocument document = mh::read_scenario(example());
  MH_CHECK(document.valid());

  const ScenarioDocument edited = mh::read_scenario(
      replaced(example(), R"("geode": "5.6.1")", R"("geode": "9.9.9")"));
  MH_CHECK(edited.valid());
  MH_CHECK_EQ(edited.scenario->id, std::string{"safe-mode-smoke"});
}

MH_TEST(scenario_sha256_is_the_hash_of_the_file_bytes) {
  const std::string text = example();
  const ScenarioDocument document = mh::read_scenario(text);
  MH_CHECK_EQ(document.sha256, mh::sha256_hex(text));

  // Whitespace is part of the file, so it is part of the identity: the trace's
  // scenario.sha256 is the hash of the bytes, not of a canonical re-serialisation.
  const ScenarioDocument reformatted = mh::read_scenario(replaced(text, "  \"id\"", "    \"id\""));
  MH_CHECK(reformatted.sha256 != document.sha256);
}

MH_TEST(scenario_rejects_a_wrong_schema_string) {
  const ScenarioDocument document =
      mh::read_scenario(replaced(example(), R"("schema": "mh-scenario/1")",
                                 R"("schema": "mh-scenario/2")"));
  MH_CHECK(has_error(document, ScenarioIssueKind::wrong_schema));
  MH_CHECK(!document.valid());
}

MH_TEST(scenario_rejects_an_unknown_status) {
  const ScenarioDocument document =
      mh::read_scenario(replaced(example(), R"("status": "draft")", R"("status": "blessed")"));
  MH_CHECK(has_error(document, ScenarioIssueKind::unknown_status));
  MH_CHECK(!document.approved_for_evidence());
}

MH_TEST(scenario_approved_status_is_required_for_evidence) {
  const ScenarioDocument document =
      mh::read_scenario(replaced(example(), R"("status": "draft")", R"("status": "approved")"));
  MH_CHECK(document.valid());
  MH_CHECK(document.approved_for_evidence());
}

MH_TEST(scenario_rejects_a_user_level) {
  // "A user-created level is not allowed, even if hashed."
  const ScenarioDocument document =
      mh::read_scenario(replaced(example(), R"("source": "generated")",
                                 R"("source": "user-level")"));
  MH_CHECK(has_error(document, ScenarioIssueKind::bad_level_source));
}

MH_TEST(scenario_rejects_a_live_profile) {
  // "a run against a live profile is not eligible for M3 evidence."
  const ScenarioDocument document =
      mh::read_scenario(replaced(example(), R"("isolation": "disposable")",
                                 R"("isolation": "live")"));
  MH_CHECK(has_error(document, ScenarioIssueKind::live_profile));
  MH_CHECK(!document.approved_for_evidence());
}

MH_TEST(scenario_rejects_a_short_level_hash) {
  const ScenarioDocument document = mh::read_scenario(replaced(
      example(), "0000000000000000000000000000000000000000000000000000000000000000",
      "000000000000000000000000000000000000000000000000000000000000000"));
  MH_CHECK(has_error(document, ScenarioIssueKind::bad_sha256));
}

MH_TEST(scenario_requires_the_observer_in_mods) {
  const ScenarioDocument document =
      mh::read_scenario(replaced(example(), R"(["absolllute.hackmega", "megahack.trace"])",
                                 R"(["absolllute.hackmega"])"));
  MH_CHECK(has_error(document, ScenarioIssueKind::observer_not_listed));
}

MH_TEST(scenario_rejects_a_tolerance_without_a_why) {
  // "A tolerance without a `why` is invalid."
  const ScenarioDocument document = mh::read_scenario(replaced(
      example(), R"("tolerances": {},)", R"("tolerances": {"/call.args.items/0/value": {"abs": 0.0001}},)"));
  MH_CHECK(has_error(document, ScenarioIssueKind::bad_tolerance));

  // The same tolerance with a why is accepted.
  const ScenarioDocument explained = mh::read_scenario(replaced(
      example(), R"("tolerances": {},)",
      R"("tolerances": {"/call.args.items/0/value": {"abs": 0.0001, "rel": 0.000001, "why": "physics float cross-build rounding"}},)"));
  MH_CHECK(!has_error(explained, ScenarioIssueKind::bad_tolerance));
  MH_CHECK_EQ(explained.scenario->tolerances.size(), std::size_t{1});
  MH_CHECK_EQ(explained.scenario->tolerances[0].second.why,
              std::string{"physics float cross-build rounding"});
}

MH_TEST(scenario_rejects_an_unknown_milestone) {
  const ScenarioDocument document = mh::read_scenario(
      replaced(example(), R"(["level_started", "attempt_started"])",
               R"(["level_began", "attempt_started"])"));
  MH_CHECK(has_error(document, ScenarioIssueKind::unknown_milestone));
}

MH_TEST(scenario_rejects_a_hook_key_that_is_not_contract_shaped) {
  // §Hook identity: the RVA is spelled lowercase in the key.
  const ScenarioDocument document = mh::read_scenario(
      replaced(example(), "GeometryDash.exe:rva:0x3b8eb0", "GeometryDash.exe:rva:0x3B8EB0"));
  MH_CHECK(has_error(document, ScenarioIssueKind::bad_hook_key));
  MH_CHECK(has_path(document, "hooks.required[0]"));
}

MH_TEST(scenario_rejects_a_settings_tag_that_is_not_hex) {
  const ScenarioDocument document =
      mh::read_scenario(replaced(example(), R"("tag": "0x1E2")", R"("tag": "1E2")"));
  MH_CHECK(has_error(document, ScenarioIssueKind::bad_settings_tag));
}

MH_TEST(scenario_rejects_quit_with_a_key) {
  const ScenarioDocument document = mh::read_scenario(
      replaced(example(), R"("action": "quit", "key": null)",
               R"("action": "quit", "key": "space")"));
  MH_CHECK(has_error(document, ScenarioIssueKind::bad_action));
}

MH_TEST(scenario_warns_when_a_single_run_is_requested) {
  const ScenarioDocument document =
      mh::read_scenario(replaced(example(), R"("repetitions": 2)", R"("repetitions": 1)"));
  MH_CHECK(document.valid());
  MH_CHECK(has_warning(document, ScenarioIssueKind::too_few_repetitions));
}

MH_TEST(scenario_rejects_malformed_json) {
  const ScenarioDocument document = mh::read_scenario(R"({"schema": "mh-scenario/1",})");
  MH_CHECK(has_error(document, ScenarioIssueKind::parse_error));
  MH_CHECK(!document.scenario.has_value());
  MH_CHECK(!document.valid());
}
