#include "mh/comparator.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mh {
namespace {

const json::Json* member(const json::Json& object, std::string_view key) {
  for (const auto& entry : object.members()) {
    if (entry.first == key) {
      return &entry.second;
    }
  }
  return nullptr;
}

std::string string_at(const json::Json& object, std::initializer_list<std::string_view> path) {
  const json::Json* node = &object;
  for (const std::string_view key : path) {
    if (node->kind() != json::Json::Kind::object) {
      return {};
    }
    node = member(*node, key);
    if (node == nullptr) {
      return {};
    }
  }
  if (node->kind() != json::Json::Kind::string) {
    return {};
  }
  return std::string{node->string_value()};
}

const json::Json* node_at(const json::Json& object,
                          std::initializer_list<std::string_view> path) {
  const json::Json* node = &object;
  for (const std::string_view key : path) {
    if (node->kind() != json::Json::Kind::object) {
      return nullptr;
    }
    node = member(*node, key);
    if (node == nullptr) {
      return nullptr;
    }
  }
  return node;
}

std::int64_t integer_at(const json::Json& object,
                        std::initializer_list<std::string_view> path,
                        std::int64_t fallback = 0) {
  const json::Json* node = node_at(object, path);
  if (node == nullptr || node->kind() != json::Json::Kind::integer) {
    return fallback;
  }
  return node->integer_value();
}

// Returns a copy of an object tree without one dotted path. Rebuilding is necessary
// because Json has no erase and no in-place mutation of a member list; the trees are one
// event, so the cost is irrelevant. A parent left with no members is dropped as well: it
// held only volatile data, and an empty object is not evidence.
json::Json without_path(const json::Json& node, const std::vector<std::string_view>& path,
                        std::size_t index) {
  if (node.kind() != json::Json::Kind::object || index >= path.size()) {
    return node;
  }

  const bool is_leaf = index + 1 == path.size();
  json::Json out = json::Json::object();
  for (const auto& entry : node.members()) {
    if (entry.first == path[index]) {
      if (is_leaf) {
        continue;
      }
      json::Json child = without_path(entry.second, path, index + 1);
      if (child.is_object() && child.members().empty()) {
        continue;
      }
      out.set(entry.first, std::move(child));
      continue;
    }
    out.set(entry.first, entry.second);
  }
  return out;
}

std::vector<std::string_view> split_path(const std::string& path) {
  std::vector<std::string_view> parts;
  std::size_t start = 0;
  while (start <= path.size()) {
    const std::size_t dot = path.find('.', start);
    const std::size_t end = dot == std::string::npos ? path.size() : dot;
    if (end > start) {
      parts.push_back(std::string_view{path}.substr(start, end - start));
    }
    if (dot == std::string::npos) {
      break;
    }
    start = dot + 1;
  }
  return parts;
}

// Sorts a settings tag list by tag so "same semantic list" does not depend on the order
// the file happened to use. Element-only: tags are objects and each has a "tag" string.
json::Json sorted_tags(const json::Json& tags) {
  if (!tags.is_array()) {
    return tags;
  }
  std::vector<json::Json> items = tags.items();
  std::stable_sort(items.begin(), items.end(), [](const json::Json& lhs, const json::Json& rhs) {
    return string_at(lhs, {"tag"}) < string_at(rhs, {"tag"});
  });
  json::Json sorted = json::Json::array();
  for (json::Json& item : items) {
    sorted.push(std::move(item));
  }
  return sorted;
}

void add_gate_issue(GateResult& result, std::string check, std::string detail,
                    GateIssueSeverity severity = GateIssueSeverity::invalid) {
  result.issues.push_back(GateIssue{std::move(check), std::move(detail), severity});
}

} // namespace

std::string_view to_string(GateStatus status) noexcept {
  switch (status) {
    case GateStatus::pass:
      return "pass";
    case GateStatus::invalid:
      return "invalid";
    case GateStatus::blocked:
      return "blocked";
  }
  return {};
}

std::ostream& operator<<(std::ostream& out, GateStatus status) {
  return out << to_string(status);
}

std::optional<std::string> canonical_event_key(const json::Json& event) {  const std::string kind = string_at(event, {"kind"});
  if (kind == "hook") {
    // hook.observer + "|" + hook.key + "|" + phase + "|" + call.depth
    const std::string observer = string_at(event, {"hook", "observer"});
    const std::string key = string_at(event, {"hook", "key"});
    const std::string phase = string_at(event, {"phase"});
    const json::Json* depth = node_at(event, {"call", "depth"});
    if (observer.empty() || key.empty() || phase.empty() || depth == nullptr ||
        depth->kind() != json::Json::Kind::integer) {
      return std::nullopt;
    }
    return observer + "|" + key + "|" + phase + "|" + std::to_string(depth->integer_value());
  }
  if (kind == "milestone") {
    const std::string name = string_at(event, {"name"});
    if (name.empty()) {
      return std::nullopt;
    }
    return "milestone|" + name;
  }
  if (kind == "save") {
    // "save|" + store + "|" + phase + "|" + source
    const std::string store = string_at(event, {"store"});
    const std::string phase = string_at(event, {"phase"});
    const std::string source = string_at(event, {"source"});
    if (store.empty() || phase.empty() || source.empty()) {
      return std::nullopt;
    }
    return "save|" + store + "|" + phase + "|" + source;
  }
  if (kind == "error") {
    const std::string code = string_at(event, {"code"});
    if (code.empty()) {
      return std::nullopt;
    }
    return "error|" + code;
  }
  return std::nullopt;
}

std::vector<std::string> default_volatile_paths() {
  return {
      "t_ns",
      "hook.va",
      "thread.id",
      "call.this.value",
      "call.duration_ns",
      "save.local_path",
      // "runtime module bases": the registering mod's addresses are variant identity, not
      // behaviour. The observed GD method (hook.key/rva) is kept: the gate requires both
      // variants to be the same GD build.
      "hook.target",
  };
}

json::Json normalize_event(const json::Json& event,
                           const std::vector<std::string>& extra_volatile) {
  std::vector<std::string> paths = default_volatile_paths();
  paths.insert(paths.end(), extra_volatile.begin(), extra_volatile.end());

  json::Json normalized = event;
  const std::string kind = string_at(normalized, {"kind"});
  for (const std::string& raw_path : paths) {
    // The contract's list is semantic, not a literal path: `save.local_path` means "the
    // local_path member of a save event", and `save` is the event's kind, not an object.
    std::string path = raw_path;
    if (path == "save.local_path" && kind == "save") {
      path = "local_path";
    }
    const std::vector<std::string_view> parts = split_path(path);
    if (parts.empty()) {
      continue;
    }
    normalized = without_path(normalized, parts, 0);
  }
  return normalized;
}

VariantSummary summarize_variant(std::string_view role,
                                 const std::vector<TraceDocument>& runs) {
  VariantSummary summary;
  summary.role = std::string{role};
  summary.runs = static_cast<std::int64_t>(runs.size());

  for (const TraceDocument& run : runs) {
    if (!run.structurally_valid()) {
      ++summary.crashed_runs;
      continue;
    }
    const json::Json& meta = *run.meta;
    const json::Json& end = *run.end;

    if (summary.target_sha256.empty()) {
      summary.target_sha256 = string_at(meta, {"target", "sha256"});
      summary.harness_id = string_at(meta, {"environment", "harness", "id"});
      summary.harness_version = string_at(meta, {"environment", "harness", "version"});
      summary.harness_sha256 = string_at(meta, {"environment", "harness", "sha256"});
      summary.gd_version = string_at(meta, {"environment", "gd", "version"});
      summary.geode_version = string_at(meta, {"environment", "geode", "version"});
      summary.level_sha256 = string_at(meta, {"level", "sha256"});
      summary.level_source = string_at(meta, {"level", "source"});
      summary.scenario_id = string_at(meta, {"scenario", "id"});
      summary.scenario_sha256 = string_at(meta, {"scenario", "sha256"});
      if (const json::Json* tags = node_at(meta, {"settings", "tags"}); tags != nullptr) {
        summary.settings_tags = *tags;
      }
    }

    const std::string status = string_at(end, {"status"});
    if (status == "crashed" || status == "timeout") {
      ++summary.crashed_runs;
    }

    const std::int64_t dropped = integer_at(end, {"counts", "dropped"});
    const std::int64_t unclosed = integer_at(end, {"counts", "unclosed"});
    summary.dropped_events += dropped;
    summary.unclosed_calls += unclosed;

    // The level digest this run observed. `level_started` carries it as a hex string value; a
    // run that recorded `unavailable` or `redacted` leaves an empty string here, which the gate
    // treats as "not observed" rather than as a mismatch.
    std::string observed;
    for (const json::Json& event : run.events) {
      if (string_at(event, {"kind"}) != "milestone" ||
          string_at(event, {"name"}) != "level_started") {
        continue;
      }
      const json::Json* data = node_at(event, {"data"});
      if (data == nullptr || string_at(*data, {"state"}) != "observed") {
        continue;
      }
      observed = string_at(*data, {"value"});
      break;
    }
    summary.level_observed.push_back(observed);
  }

  // "no unavailable required fields" (§Comparability gate, Completeness).
  for (const TraceDocument& run : runs) {
    for (const TraceIssue& issue : run.issues) {
      if (issue.kind == TraceIssueKind::capture_incomplete) {
        ++summary.unavailable_required_fields;
      }
    }
  }

  return summary;
}

GateResult check_comparability(const ScenarioDocument& scenario_document,
                               const VariantSummary& release,
                               const VariantSummary& reconstruction) {
  GateResult result;

  if (!scenario_document.scenario.has_value()) {
    add_gate_issue(result, "scenario", "the manifest did not parse");
    result.status = GateStatus::invalid;
    return result;
  }
  const Scenario& scenario = *scenario_document.scenario;

  // -- Runs: the precondition for every row below -------------------------
  // Checked first because each following row compares the two variants: with one absent there
  // is nothing to compare, and reporting every row as a mismatch buries the single real reason.
  // A run with no reconstruction at all used to come back as ten identity failures and
  // `invalid`, instead of one `blocked` row saying the evidence is not there yet.
  if (release.runs < 2 || reconstruction.runs < 2) {
    add_gate_issue(result, "runs", "at least two runs per variant are required",
                   GateIssueSeverity::blocked);
  }
  if (release.crashed_runs != 0 || reconstruction.crashed_runs != 0) {
    add_gate_issue(result, "runs", "a variant has a crashed, timed-out or incomplete run");
  }
  if (release.runs == 0 || reconstruction.runs == 0) {
    result.status = GateStatus::blocked;
    return result;
  }

  // -- Digest rows: a placeholder is missing evidence, not a mismatch ------
  // §9 reserves `invalid` for evidence that contradicts itself. A declared digest of all
  // zeros contradicts nothing: the manifest simply has not been enrolled against a real
  // artifact yet, and calling that `invalid` accuses the run of a mismatch it never made.
  // Two placeholders that "match" are worse - the row would pass having compared nothing.
  const auto placeholder_row = [&result](std::string_view check, std::string_view side,
                                         std::string_view declared) {
    add_gate_issue(result, std::string{check},
                   std::string{side} + " digest is the manifest's placeholder (" +
                       std::string{declared} + "); the evidence is not there yet",
                   GateIssueSeverity::blocked);
  };

  // -- Scenario: same scenario.sha256, same scenario.id --------------------
  if (release.scenario_sha256 != scenario_document.sha256 ||
      reconstruction.scenario_sha256 != scenario_document.sha256) {
    add_gate_issue(result, "scenario",
                   "scenario.sha256 does not match the manifest bytes");
  }
  if (release.scenario_id != scenario.id || reconstruction.scenario_id != scenario.id) {
    add_gate_issue(result, "scenario", "scenario.id does not match the manifest");
  }

  // -- Target: expected role and SHA-256 for each side ---------------------
  if (release.role != "release" || reconstruction.role != "reconstruction") {
    add_gate_issue(result, "target", "variants must be labeled release and reconstruction");
  }
  if (is_placeholder_digest(scenario.release_sha256) ||
      is_placeholder_digest(release.target_sha256)) {
    placeholder_row("target", "release", "targets.release.sha256");
  } else if (release.target_sha256 != scenario.release_sha256) {
    add_gate_issue(result, "target", "release target hash does not match the manifest");
  }
  if (is_placeholder_digest(scenario.reconstruction_sha256) ||
      is_placeholder_digest(reconstruction.target_sha256)) {
    placeholder_row("target", "reconstruction", "targets.reconstruction.sha256");
  } else if (reconstruction.target_sha256 != scenario.reconstruction_sha256) {
    add_gate_issue(result, "target", "reconstruction target hash does not match the manifest");
  }

  // -- Harness: same id, version and SHA-256 -------------------------------
  if (release.harness_sha256 != reconstruction.harness_sha256 ||
      release.harness_id != reconstruction.harness_id ||
      release.harness_version != reconstruction.harness_version) {
    add_gate_issue(result, "harness", "harness id/version/sha256 differ between variants");
  }
  if (is_placeholder_digest(scenario.harness_sha256)) {
    placeholder_row("harness", "observer", "harness.sha256");
  } else if (release.harness_id != scenario.harness_id ||
             release.harness_version != scenario.harness_version ||
             release.harness_sha256 != scenario.harness_sha256) {
    add_gate_issue(result, "harness",
                   "harness id/version/sha256 differ from the manifest");
  }

  // -- Runtime: same GD and runtime Geode --------------------------------
  if (release.gd_version.empty() || release.gd_version != reconstruction.gd_version) {
    add_gate_issue(result, "runtime", "GD version differs between variants");
  }
  if (release.geode_version.empty() || release.geode_version != reconstruction.geode_version) {
    add_gate_issue(result, "runtime", "runtime Geode version differs between variants");
  }

  // -- Level: the manifest names a fixture, and the traces must show it ran ----
  if (is_placeholder_digest(scenario.level_sha256) ||
      is_placeholder_digest(release.level_sha256) ||
      is_placeholder_digest(reconstruction.level_sha256)) {
    // Both sides usually carry the same zeros, so the equality below would "pass" having
    // compared nothing. That is the one gate row where a placeholder cannot be caught by an
    // inequality, which is why it is tested for explicitly.
    placeholder_row("level", "level fixture", "level.sha256");
  } else if (release.level_sha256 != scenario.level_sha256 ||
             reconstruction.level_sha256 != scenario.level_sha256) {
    add_gate_issue(result, "level", "level hash does not match the manifest fixture");
  }
  if (release.level_source != scenario.level_source ||
      reconstruction.level_source != scenario.level_source) {
    add_gate_issue(result, "level", "level source does not match the manifest");
  }

  // The row above compares the run context (copied from the manifest) with the manifest, so on
  // its own it proves nothing about which level the game loaded. This is the observation that
  // does: every run must have recorded the digest of the level it started, and that digest must
  // be the fixture's. A run that recorded nothing is `blocked`, because absence of an
  // observation is not agreement.
  if (!is_placeholder_digest(scenario.level_sha256)) {
    const auto first_unobserved = [](const VariantSummary& variant) {
      for (const std::string& digest : variant.level_observed) {
        if (digest.empty()) {
          return true;
        }
      }
      return variant.level_observed.empty();
    };
    if (first_unobserved(release) || first_unobserved(reconstruction)) {
      add_gate_issue(result, "level_observed",
                     "a run did not record the digest of the level it started",
                     GateIssueSeverity::blocked);
    } else {
      for (const VariantSummary* variant : {&release, &reconstruction}) {
        for (const std::string& digest : variant->level_observed) {
          if (digest != scenario.level_sha256) {
            add_gate_issue(result, "level_observed",
                           "a run started a level whose digest is not the manifest's fixture");
            break;
          }
        }
      }
    }
  }

  // -- Settings: same semantic tags, file hashes may differ --------------
  if (sorted_tags(release.settings_tags).to_canonical() !=
      sorted_tags(reconstruction.settings_tags).to_canonical()) {
    add_gate_issue(result, "settings", "settings.tags differ between variants");
  }

  // -- Runs: at least two per variant, no crash or timeout ---------------
  // (checked before the identity rows; see the top of this function)

  // A contradiction is `invalid` and dominates; evidence that is merely absent is `blocked`.
  const bool contradicts = std::any_of(result.issues.begin(), result.issues.end(),
                                       [](const GateIssue& issue) {
                                         return issue.severity == GateIssueSeverity::invalid;
                                       });
  if (contradicts) {
    result.status = GateStatus::invalid;
    return result;
  }
  if (!result.issues.empty()) {
    result.status = GateStatus::blocked;
    return result;
  }

  // -- Completeness ------------------------------------------------------
  if (release.dropped_events != 0 || reconstruction.dropped_events != 0) {
    add_gate_issue(result, "completeness", "required events were dropped");
  }
  if (release.unclosed_calls != 0 || reconstruction.unclosed_calls != 0) {
    add_gate_issue(result, "completeness", "a required call has no exit");
  }
  if (release.unavailable_required_fields != 0 ||
      reconstruction.unavailable_required_fields != 0) {
    add_gate_issue(result, "completeness", "a required field could not be captured");
  }

  result.status = result.issues.empty() ? GateStatus::pass : GateStatus::blocked;
  return result;
}

bool is_placeholder_digest(std::string_view digest) {
  if (digest.size() != 64) {
    return false;
  }
  for (const char digit : digest) {
    if (digit != '0') {
      return false;
    }
  }
  return true;
}

} // namespace mh
