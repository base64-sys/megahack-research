// End-to-end comparison: manifest + two variants' traces → one result status.
//
// Source: notes/runtime-validation.md §Comparability gate, §Comparison rules 1–9. This unit
// only sequences the pieces; each rule lives in its own module:
//
//   comparator.hpp  gate, canonical keys, volatile normalisation
//   sequence.hpp    min_count and per-key alignment
//   comparison.hpp  values with tolerances, ordered groups, result statuses
//
// What is decided here and nowhere else:
//   - stability (§8) is checked *within* each variant before any cross-variant claim, using
//     the same normalisation and keys;
//   - a difference on a key the manifest names as required is a failure, and a difference on
//     anything else is a warning (§6 "Optional event | Warning");
//   - payload equality is exact here. Declared tolerances are applied by
//     `compare_value_json` when a caller compares the values themselves; a corpus pair that
//     needs a tolerance must declare it and be compared through that path.

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "mh/comparison.hpp"
#include "mh/comparator.hpp"
#include "mh/scenario.hpp"
#include "mh/sequence.hpp"
#include "mh/trace_reader.hpp"

namespace mh {

// What a `pass` proves. §9's statuses say whether the two variants agreed; they do not say
// how much that agreement is worth. The manifest decides: a scenario whose *required*
// observations can only be produced by the target (a `save` snapshot taken at the target's
// own veto site) supports a behavioural claim, while one that requires only GD lifecycle
// hooks the harness installs itself proves the surface and nothing more. Never quote a
// `surface` pass as "the reconstruction behaves like the release".
enum class ClaimClass { surface, behavioural };
std::string_view to_string(ClaimClass claim) noexcept;
std::ostream& operator<<(std::ostream& out, ClaimClass claim);

// The record kinds the manifest makes target-dependent by requiring their site. The registry
// lives in `src/compare.cpp` and holds one entry today: `0x16c8b0` (the save-percentage veto),
// whose `save` snapshots are read from the filesystem around the target's own hook.
std::vector<std::string> required_target_dependent_kinds(const Scenario& scenario);

// `behavioural` only when the manifest requires a target-dependent kind *and* both variants
// actually contain a record of that kind. A manifest that asks for one and a run that never
// produced it is surface, not a claim.
ClaimClass derive_claim(const ScenarioDocument& manifest,
                        const std::vector<TraceDocument>& release_runs,
                        const std::vector<TraceDocument>& reconstruction_runs);

struct ComparisonReport {
  ComparisonStatus status = ComparisonStatus::invalid;
  GateStatus gate = GateStatus::invalid;
  bool unstable = false;
  ClaimClass claim = ClaimClass::surface;
  std::vector<Finding> findings;
  std::vector<KeyAlignment> alignments;
};

ComparisonReport compare_traces(const ScenarioDocument& manifest,
                                const std::vector<TraceDocument>& release_runs,
                                const std::vector<TraceDocument>& reconstruction_runs);

} // namespace mh
