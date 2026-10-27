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
#include <vector>

#include "mh/comparison.hpp"
#include "mh/comparator.hpp"
#include "mh/scenario.hpp"
#include "mh/sequence.hpp"
#include "mh/trace_reader.hpp"

namespace mh {

struct ComparisonReport {
  ComparisonStatus status = ComparisonStatus::invalid;
  GateStatus gate = GateStatus::invalid;
  bool unstable = false;
  std::vector<Finding> findings;
  std::vector<KeyAlignment> alignments;
};

ComparisonReport compare_traces(const ScenarioDocument& manifest,
                                const std::vector<TraceDocument>& release_runs,
                                const std::vector<TraceDocument>& reconstruction_runs);

} // namespace mh
