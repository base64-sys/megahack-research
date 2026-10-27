// Scenario manifest (`mh-scenario/1`): parsing and validation.
//
// Source: notes/runtime-validation.md §Scenario manifest, §Level and profile, §Input,
// §Timing and stopping, §Expected milestones and hooks. The rules this unit enforces:
//
//   - `schema` is `mh-scenario/1`; `status` is `draft`, `approved` or `retired`, and
//     "Only `approved` scenarios may produce M3 evidence. `draft` scenarios have
//     unresolved trigger or fixture questions."
//   - Level: "A generated fixture is preferred... A user-created level is not allowed,
//     even if hashed. A downloaded level is not allowed." `source` is therefore
//     `generated` or `local-fixture`.
//   - Profile: "a run against a live profile is not eligible for M3 evidence."
//   - Hooks: keys use the §Hook identity spelling, and `phases` is a subset of
//     `enter`/`exit`.
//   - Tolerances: "A tolerance without a `why` is invalid."
//   - Milestones: the note's "minimum vocabulary" is the vocabulary.
//
// The manifest's SHA-256 is the hash of the file bytes, not of a re-serialisation:
// "The trace's `scenario.sha256` is the SHA-256 of the manifest file bytes, so the manifest
// must not contain its own hash."

#pragma once

#include <cstdint>
#include <cstddef>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "mh/json.hpp"

namespace mh {

enum class ScenarioStatus { draft, approved, retired };
std::string_view to_string(ScenarioStatus status) noexcept;
std::optional<ScenarioStatus> parse_scenario_status(std::string_view text);

enum class ScenarioIssueSeverity { warning, error };

enum class ScenarioIssueKind {
  parse_error,
  missing_key,
  bad_type,
  wrong_schema,
  unknown_status,
  bad_sha256,
  bad_level_source,
  live_profile,
  bad_driver,
  bad_action,
  unknown_milestone,
  bad_hook_key,
  bad_hook_phase,
  bad_tolerance,
  bad_settings_tag,
  observer_not_listed,
  too_few_repetitions,
};

std::string_view to_string(ScenarioIssueKind kind) noexcept;
std::ostream& operator<<(std::ostream& out, ScenarioIssueKind kind);

struct ScenarioIssue {
  ScenarioIssueSeverity severity = ScenarioIssueSeverity::error;
  ScenarioIssueKind kind = ScenarioIssueKind::missing_key;
  // Dotted path into the manifest, e.g. "level.source". Not a JSON Pointer: readability
  // wins here because the message is for a human fixing the manifest.
  std::string path;
  std::string detail;
};

struct HookSpec {
  std::string key;
  std::vector<std::string> phases;
  std::int64_t min_count = 1;
};

struct Tolerance {
  double abs_tolerance = 0.0;
  double rel_tolerance = 0.0;
  std::string why;
};

struct Scenario {
  std::string id;
  ScenarioStatus status = ScenarioStatus::draft;
  std::string description;

  std::string level_fixture;
  std::string level_sha256;
  std::string level_source;

  std::string profile_isolation;

  std::string harness_id;
  std::string harness_version;
  std::string harness_sha256;

  std::string release_artifact;
  std::string release_sha256;
  std::string reconstruction_artifact;
  std::string reconstruction_sha256;

  std::vector<std::string> required_mods;
  std::int64_t repetitions = 0;

  // The semantic settings list, kept as JSON because it is compared tag by tag and the
  // file hash is explicitly not the cross-variant identity (§meta record).
  json::Json settings_tags = json::Json::array();

  std::vector<HookSpec> required_hooks;
  std::vector<HookSpec> optional_hooks;
  std::vector<std::string> required_milestones;
  std::vector<std::vector<std::string>> order_groups;
  std::vector<std::pair<std::string, Tolerance>> tolerances;
  std::vector<std::string> volatile_paths;
};

struct ScenarioDocument {
  std::optional<Scenario> scenario;
  // SHA-256 of the bytes handed to read_scenario: the trace's `scenario.sha256`.
  std::string sha256;
  std::vector<ScenarioIssue> issues;

  bool valid() const noexcept;
  bool has_errors() const noexcept;
  // "Only `approved` scenarios may produce M3 evidence."
  bool approved_for_evidence() const noexcept;
};

ScenarioDocument read_scenario(std::string_view json_text);

} // namespace mh
