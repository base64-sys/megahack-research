#include "mh/compare.hpp"

#include <algorithm>
#include <map>
#include <set>
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
//
// `frame` is `CCDirector::getTotalFrames()`: an absolute count since the process started, so
// the same behaviour never repeats its frame numbers across launches (the first four bounded
// captures opened at frames 12507 and 37963 of the same level). The behaviour-carrying clock
// is "frames since the level started", which is the unit the bounded window is measured in,
// so a document's frames are rebased on its own `level_started` observation, 1-based. The
// field is kept, not dropped: a hook that fires one frame later must still be a difference.
// A document with no `level_started` keeps absolute frames - absence of the anchor is not a
// licence to rewrite the clock, and such a scenario has to declare `frame` volatile.
json::Json comparison_payload(const json::Json& event,
                              std::optional<std::int64_t> level_started_frame,
                              const std::vector<std::string>& declared_volatile) {
  // The envelope is per-file; `comparison.volatile` is the manifest's own list of fields it
  // observed to vary between runs of one build ("record the varying field ... only after it
  // has been observed to vary, with the observed run ids"). Both are removed here or nowhere:
  // a declared path that the payload still contained would be decoration.
  std::vector<std::string> volatile_paths{"schema", "type",   "run_id", "run_fingerprint",
                                          "seq",    "capture"};
  volatile_paths.insert(volatile_paths.end(), declared_volatile.begin(),
                        declared_volatile.end());
  json::Json payload = normalize_event(event, volatile_paths);
  if (!level_started_frame.has_value()) {
    return payload;
  }
  // Rebuilt member by member: `Json::set` refuses an existing key, so the rebased field is
  // written while the object is constructed rather than patched in place.
  json::Json rebased = json::Json::object();
  bool rebased_any = false;
  for (const auto& [name, value] : payload.members()) {
    if (name == "frame" && value.kind() == json::Json::Kind::integer) {
      rebased.set(name, json::Json::integer(value.integer_value() - *level_started_frame + 1));
      rebased_any = true;
    } else {
      rebased.set(name, value);
    }
  }
  return rebased_any ? rebased : payload;
}

// The document's own `level_started` frame, or `std::nullopt` when it never observed one.
std::optional<std::int64_t> anchor_frame(const TraceDocument& trace) {
  for (const json::Json& event : trace.events) {
    if (string_member(event, "kind") != "milestone" ||
        string_member(event, "name") != "level_started") {
      continue;
    }
    const json::Json* frame = member(event, "frame");
    if (frame == nullptr || frame->kind() != json::Json::Kind::integer) {
      return std::nullopt;
    }
    return frame->integer_value();
  }
  return std::nullopt;
}

std::map<std::string, std::vector<std::string>> payloads_by_key(
    const TraceDocument& trace, const std::vector<std::string>& declared_volatile) {
  const std::optional<std::int64_t> anchor = anchor_frame(trace);
  std::map<std::string, std::vector<std::string>> payloads;
  for (const json::Json& event : trace.events) {
    const std::optional<std::string> key = canonical_event_key(event);
    if (!key.has_value()) {
      continue;
    }
    payloads[*key].push_back(
        comparison_payload(event, anchor, declared_volatile).to_canonical());
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

// The event keys that disagree between a variant's first run and its other runs, newest last.
// Pure and bounded: this feeds a diagnostic message, not a verdict.
std::vector<std::pair<std::string, std::string>> variant_differences(
    const std::vector<TraceDocument>& runs,
    const std::vector<std::string>& declared_volatile, std::size_t limit = 8) {
  std::vector<std::pair<std::string, std::string>> differences;
  if (runs.size() < 2) {
    return differences;
  }
  const auto baseline = payloads_by_key(runs.front(), declared_volatile);
  for (std::size_t index = 1; index < runs.size(); ++index) {
    const auto other = payloads_by_key(runs[index], declared_volatile);
    const std::string run = "run " + std::to_string(index + 1) + " vs run 1";
    // Stable order: every key either side, so the first reported difference is reproducible.
    std::set<std::string> keys;
    for (const auto& entry : baseline) {
      keys.insert(entry.first);
    }
    for (const auto& entry : other) {
      keys.insert(entry.first);
    }
    for (const std::string& key : keys) {
      if (differences.size() >= limit) {
        return differences;
      }
      const auto left = baseline.find(key);
      const auto right = other.find(key);
      if (left == baseline.end()) {
        differences.emplace_back(key, run + ": absent from run 1");
      } else if (right == other.end()) {
        differences.emplace_back(key, run + ": present in run 1, absent here");
      } else if (left->second != right->second) {
        differences.emplace_back(key, run + ": payload differs");
      }
    }
  }
  return differences;
}

// §8: "compare each variant's repeated runs with the same rules. If a required event,
// payload, or order differs within a variant, the scenario is `unstable`".
bool variant_is_unstable(const std::vector<TraceDocument>& runs,
                         const std::vector<std::string>& declared_volatile) {
  if (runs.size() < 2) {
    return false;
  }
  const auto baseline = payloads_by_key(runs.front(), declared_volatile);
  for (std::size_t index = 1; index < runs.size(); ++index) {
    if (payloads_differ(baseline, payloads_by_key(runs[index], declared_volatile))) {
      return true;
    }
  }
  return false;
}

bool is_required_key(const Scenario& scenario, const std::string& key) {  for (const HookSpec& hook : scenario.required_hooks) {
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

// Diagnostic only: `finalize_status` ignores `unstable` findings, so this cannot change a
// verdict. It answers the question the bare `unstable: yes` leaves open - which keys disagreed.
void note_variant_drift(ComparisonReport& report, const char* variant,
                        const std::vector<TraceDocument>& runs,
                        const std::vector<std::string>& declared_volatile) {
  const auto differences = variant_differences(runs, declared_volatile);
  if (differences.empty()) {
    return;
  }
  for (const auto& [key, detail] : differences) {
    add_finding(report, ComparisonStatus::unstable,
                std::string(variant) + " variant", key + " - " + detail);
  }
  if (differences.size() == 8) {
    add_finding(report, ComparisonStatus::unstable, std::string(variant) + " variant",
                "further differences not listed (capped at 8)");
  }
}

// The sites whose records only the target can produce. One entry today, and it is a registry
// rather than a guess: `0x16c8b0` is the save-percentage veto, and the observer's `save`
// snapshots around it read the file the target decides whether to write. A hook key the
// registry does not know is *not* target-dependent - the observer installs it, so both
// variants produce it identically.
struct TargetDependentSite {
  std::string_view hook_key;
  std::string_view record_kind;
};

constexpr TargetDependentSite kTargetDependentSites[] = {
    {"GeometryDash.exe:rva:0x16c8b0", "save"},
};

bool has_record_kind(const std::vector<TraceDocument>& runs, std::string_view kind) {
  for (const TraceDocument& run : runs) {
    for (const json::Json& event : run.events) {
      if (string_member(event, "kind") == kind) {
        return true;
      }
    }
  }
  return false;
}

} // namespace

std::string_view to_string(ClaimClass claim) noexcept {
  switch (claim) {
    case ClaimClass::surface:
      return "surface";
    case ClaimClass::behavioural:
      return "behavioural";
  }
  return {};
}

std::ostream& operator<<(std::ostream& out, ClaimClass claim) {
  return out << to_string(claim);
}

std::vector<std::string> required_target_dependent_kinds(const Scenario& scenario) {
  std::vector<std::string> kinds;
  for (const HookSpec& hook : scenario.required_hooks) {
    for (const TargetDependentSite& site : kTargetDependentSites) {
      if (hook.key != site.hook_key) {
        continue;
      }
      const std::string kind{site.record_kind};
      if (std::find(kinds.begin(), kinds.end(), kind) == kinds.end()) {
        kinds.push_back(kind);
      }
    }
  }
  return kinds;
}

ClaimClass derive_claim(const ScenarioDocument& manifest,
                        const std::vector<TraceDocument>& release_runs,
                        const std::vector<TraceDocument>& reconstruction_runs) {
  if (!manifest.scenario.has_value()) {
    return ClaimClass::surface;
  }
  const std::vector<std::string> kinds =
      required_target_dependent_kinds(*manifest.scenario);
  if (kinds.empty()) {
    return ClaimClass::surface;
  }
  // Required by the manifest is not the same as observed by the run: a site that never fired
  // leaves the trace with nothing that depends on the target.
  for (const std::string& kind : kinds) {
    if (!has_record_kind(release_runs, kind) || !has_record_kind(reconstruction_runs, kind)) {
      return ClaimClass::surface;
    }
  }
  return ClaimClass::behavioural;
}

ComparisonReport compare_traces(const ScenarioDocument& manifest,
                                const std::vector<TraceDocument>& release_runs,
                                const std::vector<TraceDocument>& reconstruction_runs) {
  ComparisonReport report;
  // Written before any early return: the claim is a property of the manifest and the
  // evidence, so it exists even when the comparison stops at the gate.
  report.claim = derive_claim(manifest, release_runs, reconstruction_runs);

  const VariantSummary release = summarize_variant("release", release_runs);
  const VariantSummary reconstruction =
      summarize_variant("reconstruction", reconstruction_runs);
  const GateResult gate = check_comparability(manifest, release, reconstruction);
  report.gate = gate.status;

  if (gate.status != GateStatus::pass) {
    // "If any fails, the result is `invalid` or `blocked`; do not compare them
    // 'approximately'." The gate's own reasons travel with the report: a status with no
    // explanation is unusable, which is how a draft manifest reported `invalid` and said
    // nothing about its zeroed digests.
    const ComparisonStatus severity = gate.status == GateStatus::blocked
                                          ? ComparisonStatus::blocked
                                          : ComparisonStatus::invalid;
    for (const GateIssue& issue : gate.issues) {
      report.findings.push_back(Finding{severity, issue.check, issue.detail});
    }
    report.status = finalize_status(gate.status, report.findings);
    return report;
  }
  if (!manifest.scenario.has_value() || release_runs.empty() ||
      reconstruction_runs.empty()) {
    report.status = ComparisonStatus::blocked;
    return report;
  }
  const Scenario& scenario = *manifest.scenario;

  // -- §8 stability, within each variant, before any cross-variant claim ------
  report.unstable = variant_is_unstable(release_runs, scenario.volatile_paths) ||
                    variant_is_unstable(reconstruction_runs, scenario.volatile_paths);
  // `unstable` is a verdict, and on its own it does not say what disagreed. These findings are
  // diagnostics: `finalize_status` ignores this severity, so the verdict is unchanged and the
  // operator learns which event keys differ between two runs of the same variant.
  note_variant_drift(report, "release", release_runs, scenario.volatile_paths);
  note_variant_drift(report, "reconstruction", reconstruction_runs,
                     scenario.volatile_paths);

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
      align_by_key(payloads_by_key(release_runs.front(), scenario.volatile_paths),
                   payloads_by_key(reconstruction_runs.front(), scenario.volatile_paths));
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
