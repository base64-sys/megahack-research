// Comparator core: the comparability gate, volatile-field normalisation and the canonical
// event keys.
//
// Source: notes/runtime-validation.md §Comparability gate, §Comparison rules 1 and §9.
//
//   - "Two traces are comparable only when all of these hold. If any fails, the result is
//     `invalid` or `blocked`; do not compare them 'approximately'."
//   - "Remove fields that are not behaviour: `t_ns`, `hook.va`, `thread.id`,
//     `call.this.value` (unless the manifest explicitly makes an object identity
//     meaningful), `call.duration_ns`, `save.local_path`, absolute paths, and runtime
//     module bases. Do not remove a field merely because it differs between the two
//     targets."
//   - Canonical event keys (table): hook = observer|key|phase|depth, milestone =
//     "milestone|" + name, save = "save|" + store|phase|source, error = "error|" + code.
//
// The gate answers "may these two variants be compared at all"; it never answers "do they
// agree". That is items 9 and 10.

#pragma once

#include <cstdint>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "mh/json.hpp"
#include "mh/scenario.hpp"
#include "mh/trace_reader.hpp"

namespace mh {

// §Comparison rules 1 table. `std::nullopt` for an unknown kind: a key this comparator
// does not know cannot be grouped, and inventing one would silently compare nothing.
std::optional<std::string> canonical_event_key(const json::Json& event);

// The volatility list §Comparison rules 1 names, plus the variant-identity fields the same
// paragraph calls "runtime module bases".
std::vector<std::string> default_volatile_paths();

// Returns a copy of `event` with the volatile paths removed (dotted paths; a missing path
// is not an error). `extra_volatile` is the manifest's own `comparison.volatile` list,
// which §8 says may only grow after a field has been observed to vary.
json::Json normalize_event(const json::Json& event,
                           const std::vector<std::string>& extra_volatile = {});

// What the gate needs to know about one variant's runs. Built from traces by
// `summarize_variant`; tests may build it directly.
struct VariantSummary {
  std::string role;  // "release" or "reconstruction"
  std::string target_sha256;
  std::string harness_id;
  std::string harness_version;
  std::string harness_sha256;
  std::string gd_version;
  std::string geode_version;
  std::string level_sha256;
  std::string level_source;
  std::string scenario_id;
  std::string scenario_sha256;
  json::Json settings_tags = json::Json::array();

  std::int64_t runs = 0;
  std::int64_t crashed_runs = 0;
  std::int64_t dropped_events = 0;
  std::int64_t unclosed_calls = 0;
  std::int64_t unavailable_required_fields = 0;
};

// Reads the gate's fields out of the variant's traces. Runs that are not structurally
// valid are counted as crashed, because "each with an `end` record and no crash/timeout"
// is a precondition, not a detail.
VariantSummary summarize_variant(std::string_view role, const std::vector<TraceDocument>& runs);

enum class GateStatus { pass, invalid, blocked };
std::string_view to_string(GateStatus status) noexcept;
std::ostream& operator<<(std::ostream& out, GateStatus status);

// §9 splits a failed gate in two: `invalid` when the evidence contradicts itself (a hash, a
// version, a scenario that is not the one declared), and `blocked` when the evidence that
// the row needs is simply not there yet. A declared digest of all zeros is the second kind -
// the manifest has not been enrolled against a real artifact - and saying `invalid` for it
// accuses the run of a mismatch it did not commit.
enum class GateIssueSeverity { invalid, blocked };

// 64 hex characters, all `0`: the manifest's placeholder, never a real SHA-256.
bool is_placeholder_digest(std::string_view digest);

struct GateIssue {
  std::string check;  // the §Comparability gate row, e.g. "harness"
  std::string detail;
  GateIssueSeverity severity = GateIssueSeverity::invalid;
};

struct GateResult {
  GateStatus status = GateStatus::invalid;
  std::vector<GateIssue> issues;

  bool passed() const noexcept { return status == GateStatus::pass; }
};

// §Comparability gate. Identity mismatches are `invalid` (§9: "Comparability gate failed
// (hash, version, scenario, observer, level, settings)"); a required observation that is
// missing is `blocked`.
GateResult check_comparability(const ScenarioDocument& scenario, const VariantSummary& release,
                               const VariantSummary& reconstruction);

} // namespace mh
