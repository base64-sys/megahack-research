#include "mh/compare.hpp"

#include <algorithm>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "mh/json.hpp"

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

std::string string_member(const json::Json& object, std::string_view key) {
  const json::Json* node = member(object, key);
  if (node == nullptr || node->kind() != json::Json::Kind::string) {
    return {};
  }
  return std::string{node->string_value()};
}

// The canonical payload of an event: normalised, then stripped of the per-file envelope so
// two variants' copies of the same behaviour compare equal. `seq` is a line counter, the
// run identity is deliberately random, and `capture` is evidence about the observation
// rather than the behaviour.
json::Json comparison_payload(const json::Json& event) {
  return normalize_event(event, {"schema", "type", "run_id", "run_fingerprint", "seq",
                                 "capture"});
}

std::map<std::string, std::vector<std::string>> payloads_by_key(const TraceDocument& trace) {
  std::map<std::string, std::vector<std::string>> payloads;
  for (const json::Json& event : trace.events) {
    const std::optional<std::string> key = canonical_event_key(event);
    if (!key.has_value()) {
      continue;
    }
    payloads[*key].push_back(comparison_payload(event).to_canonical());
  }
  return payloads;
}

bool payloads_differ(const std::map<std::string, std::vector<std::string>>& lhs,
                     const std::map<std::string, std::vector<std::string>>& rhs) {
  if (lhs.size() != rhs.size()) {
    return true;
  }
  for (const auto& entry : lhs) {
    const auto other = rhs.find(entry.first);
    if (other == rhs.end() || other->second != entry.second) {
      return true;
    }
  }
  return false;
}

// §8: "compare each variant's repeated runs with the same rules. If a required event,
// payload, or order differs within a variant, the scenario is `unstable`".
bool variant_is_unstable(const std::vector<TraceDocument>& runs) {
  if (runs.size() < 2) {
    return false;
  }
  const auto baseline = payloads_by_key(runs.front());
  for (std::size_t index = 1; index < runs.size(); ++index) {
    if (payloads_differ(baseline, payloads_by_key(runs[index]))) {
      return true;
    }
  }
  return false;
}

bool is_required_key(const Scenario& scenario, const std::string& key) {
  for (const HookSpec& hook : scenario.required_hooks) {
    // hook keys are embedded in the canonical key: observer|key|phase|depth
    if (key.find("|" + hook.key + "|") != std::string::npos) {
      return true;
    }
  }
  for (const std::string& milestone : scenario.required_milestones) {
    if (key == "milestone|" + milestone) {
      return true;
    }
  }
  return false;
}

std::map<std::string, std::int64_t> hook_enter_counts(const TraceDocument& trace) {
  std::map<std::string, std::int64_t> counts;
  for (const json::Json& event : trace.events) {
    if (string_member(event, "kind") != "hook" || string_member(event, "phase") != "enter") {
      continue;
    }
    const json::Json* hook = member(event, "hook");
    if (hook == nullptr) {
      continue;
    }
    ++counts[string_member(*hook, "key")];
  }
  return counts;
}

std::vector<OrderEvent> order_events(const TraceDocument& trace) {
  std::vector<OrderEvent> events;
  for (const json::Json& event : trace.events) {
    const std::string kind = string_member(event, "kind");
    OrderEvent order;
    if (const json::Json* thread = member(event, "thread");
        thread != nullptr && thread->is_object()) {
      order.thread_role = string_member(*thread, "role");
    }
    if (kind == "milestone") {
      order.milestone_name = string_member(event, "name");
    } else if (kind == "hook") {
      if (string_member(event, "phase") != "enter") {
        continue;
      }
      if (const json::Json* hook = member(event, "hook"); hook != nullptr && hook->is_object()) {
        order.hook_key = string_member(*hook, "key");
      }
    } else {
      continue;
    }
    events.push_back(std::move(order));
  }
  return events;
}

void add_finding(ComparisonReport& report, ComparisonStatus severity, std::string check,
                 std::string detail) {
  report.findings.push_back(Finding{severity, std::move(check), std::move(detail)});
}

} // namespace

ComparisonReport compare_traces(const ScenarioDocument& manifest,
                                const std::vector<TraceDocument>& release_runs,
                                const std::vector<TraceDocument>& reconstruction_runs) {
  ComparisonReport report;

  const VariantSummary release = summarize_variant("release", release_runs);
  const VariantSummary reconstruction =
      summarize_variant("reconstruction", reconstruction_runs);
  const GateResult gate = check_comparability(manifest, release, reconstruction);
  report.gate = gate.status;

  if (gate.status != GateStatus::pass) {
    // "If any fails, the result is `invalid` or `blocked`; do not compare them
    // 'approximately'."
    report.status = finalize_status(gate.status, {});
    return report;
  }
  if (!manifest.scenario.has_value() || release_runs.empty() ||
      reconstruction_runs.empty()) {
    report.status = ComparisonStatus::blocked;
    return report;
  }
  const Scenario& scenario = *manifest.scenario;

  // -- §8 stability, within each variant, before any cross-variant claim ------
  report.unstable = variant_is_unstable(release_runs) || variant_is_unstable(reconstruction_runs);

  // -- §3 min_count on both sides, before any alignment is trusted -----------
  std::vector<RequiredKeyCount> required;
  for (const HookSpec& hook : scenario.required_hooks) {
    required.push_back(RequiredKeyCount{hook.key, hook.min_count});
  }
  const auto release_counts = hook_enter_counts(release_runs.front());
  const auto reconstruction_counts = hook_enter_counts(reconstruction_runs.front());
  for (const MinCountFinding& shortfall :
       check_min_counts(required, release_counts, reconstruction_counts)) {
    add_finding(report, ComparisonStatus::fail, "min_count",
                shortfall.key + ": release " + std::to_string(shortfall.release_count) +
                    ", reconstruction " + std::to_string(shortfall.reconstruction_count) +
                    ", required " + std::to_string(shortfall.required));
  }

  // -- §3 per-key alignment --------------------------------------------------
  report.alignments =
      align_by_key(payloads_by_key(release_runs.front()),
                   payloads_by_key(reconstruction_runs.front()));
  for (const KeyAlignment& key_alignment : report.alignments) {
    const bool required_key = is_required_key(scenario, key_alignment.key);
    const ComparisonStatus severity =
        required_key ? ComparisonStatus::fail : ComparisonStatus::pass_with_warnings;

    if (key_alignment.alignment.changed != 0) {
      // A changed payload on an optional key is still a changed event; §6 makes only
      // *absence* of an optional event a warning.
      add_finding(report, ComparisonStatus::fail, "alignment",
                  key_alignment.key + ": " +
                      std::to_string(key_alignment.alignment.changed) + " changed event(s)");
    }
    if (key_alignment.alignment.missing != 0) {
      add_finding(report, severity, "alignment",
                  key_alignment.key + ": " +
                      std::to_string(key_alignment.alignment.missing) +
                      " event(s) missing from the reconstruction");
    }
    if (key_alignment.alignment.extra != 0) {
      add_finding(report, severity, "alignment",
                  key_alignment.key + ": " +
                      std::to_string(key_alignment.alignment.extra) +
                      " extra event(s) in the reconstruction");
    }
  }

  // -- §4 ordered groups, on main-thread evidence only -----------------------
  const std::vector<OrderEvent> release_order = order_events(release_runs.front());
  const std::vector<OrderEvent> reconstruction_order = order_events(reconstruction_runs.front());
  for (const std::vector<std::string>& group : scenario.order_groups) {
    if (!check_order_group(release_order, group).satisfied) {
      add_finding(report, ComparisonStatus::fail, "ordering",
                  "release does not satisfy an ordered group");
    }
    if (!check_order_group(reconstruction_order, group).satisfied) {
      add_finding(report, ComparisonStatus::fail, "ordering",
                  "reconstruction does not satisfy an ordered group");
    }
  }

  report.status = finalize_status(gate.status, report.findings, report.unstable);
  return report;
}

} // namespace mh
