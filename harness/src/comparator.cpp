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

void add_gate_issue(GateResult& result, std::string check, std::string detail) {
  result.issues.push_back(GateIssue{std::move(check), std::move(detail)});
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
  if (release.target_sha256 != scenario.release_sha256) {
    add_gate_issue(result, "target", "release target hash does not match the manifest");
  }
  if (reconstruction.target_sha256 != scenario.reconstruction_sha256) {
    add_gate_issue(result, "target", "reconstruction target hash does not match the manifest");
  }

  // -- Harness: same id, version and SHA-256 -------------------------------
  if (release.harness_id != scenario.harness_id ||
      release.harness_version != scenario.harness_version ||
      release.harness_sha256 != scenario.harness_sha256 ||
      reconstruction.harness_id != release.harness_id ||
      reconstruction.harness_version != release.harness_version ||
      reconstruction.harness_sha256 != release.harness_sha256) {
    add_gate_issue(result, "harness",
                   "harness id/version/sha256 differ between variants or from the manifest");
  }

  // -- Runtime: same GD and runtime Geode --------------------------------
  if (release.gd_version.empty() || release.gd_version != reconstruction.gd_version) {
    add_gate_issue(result, "runtime", "GD version differs between variants");
  }
  if (release.geode_version.empty() || release.geode_version != reconstruction.geode_version) {
    add_gate_issue(result, "runtime", "runtime Geode version differs between variants");
  }

  // -- Level: same fixture SHA-256 and source ----------------------------
  if (release.level_sha256 != scenario.level_sha256 ||
      reconstruction.level_sha256 != scenario.level_sha256) {
    add_gate_issue(result, "level", "level hash does not match the manifest fixture");
  }
  if (release.level_source != scenario.level_source ||
      reconstruction.level_source != scenario.level_source) {
    add_gate_issue(result, "level", "level source does not match the manifest");
  }

  // -- Settings: same semantic tags, file hashes may differ --------------
  if (sorted_tags(release.settings_tags).to_canonical() !=
      sorted_tags(reconstruction.settings_tags).to_canonical()) {
    add_gate_issue(result, "settings", "settings.tags differ between variants");
  }

  // -- Runs: at least two per variant, no crash or timeout ---------------
  if (release.runs < 2 || reconstruction.runs < 2) {
    add_gate_issue(result, "runs", "at least two runs per variant are required");
  }
  if (release.crashed_runs != 0 || reconstruction.crashed_runs != 0) {
    add_gate_issue(result, "runs", "a variant has a crashed, timed-out or incomplete run");
  }

  // Identity failures are `invalid`; a missing required observation is `blocked`.
  const bool identity_failed = !result.issues.empty();
  if (identity_failed) {
    result.status = GateStatus::invalid;
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

} // namespace mh
