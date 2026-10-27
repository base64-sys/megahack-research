// The tail of the comparison pipeline: value comparison with tolerances, ordered groups,
// and the result statuses.
//
// Source: notes/runtime-validation.md §Comparison rules 4, 5, 6 and 9.
//
//   - §4 "Compare the sequence of required milestone names and the hook-enter keys named in
//     each `order_groups` group with the same alignment. A required group that is out of
//     order is a failure. Events on threads whose role is not `main` are compared as
//     multisets, not ordered streams. If the harness cannot prove a thread is `main`, its
//     events are not eligible to satisfy an ordered group."
//   - §5 "Float | Exact `bits`; tolerance only when declared for that field" and
//     `abs(a-b) <= abs_tol + rel_tol * max(abs(a), abs(b))`, with "A tolerance without a
//     `why` is invalid" and "Do not apply a tolerance to a hash, a boolean, an enum, a
//     count, or a frame index."
//   - §6 "Capture unavailable | Blocked", "Unavailable | Never equal to anything".
//   - §9 the six statuses.

#pragma once

#include <cstddef>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "mh/comparator.hpp"
#include "mh/json.hpp"
#include "mh/scenario.hpp"
#include "mh/value.hpp"

namespace mh {

// --- value objects ------------------------------------------------------------------

// A trace's value object as the model's Value. `std::nullopt` when the object is not one of
// the §Value objects shapes, which is a malformed trace rather than a value.
std::optional<Value> value_from_json(const json::Json& node);

enum class ValueVerdict { equal, different, excluded, blocked };
std::string_view to_string(ValueVerdict verdict) noexcept;
std::ostream& operator<<(std::ostream& out, ValueVerdict verdict);

struct ValueComparison {
  ValueVerdict verdict = ValueVerdict::blocked;
  std::string detail;
};

// Compares two value objects. `tolerance` applies only to floats and only when it carries a
// `why`; §5 forbids applying it to counts, enums, booleans and hashes, and this comparator
// never sees those as float payloads.
ValueComparison compare_value_json(const json::Json& release, const json::Json& reconstruction,
                                   const std::optional<Tolerance>& tolerance = std::nullopt);

// --- ordered groups -----------------------------------------------------------------

// The parts of an event §4 orders on. `milestone_name` is set for milestones, `hook_key`
// for hook enters; both empty means the event cannot satisfy an ordered group.
struct OrderEvent {
  std::string milestone_name;
  std::string hook_key;
  std::string thread_role = "unknown";
};

struct OrderingResult {
  bool satisfied = false;
  // Group entries that never appeared, in main-thread order, after the previous match.
  std::vector<std::string> missing;
};

// §4: only events the harness proved to be main-thread may satisfy an ordered group, so
// this filter is applied by the checker itself rather than trusted to the caller.
OrderingResult check_order_group(const std::vector<OrderEvent>& events,
                                 const std::vector<std::string>& group);

// --- result statuses ----------------------------------------------------------------

enum class ComparisonStatus { pass, pass_with_warnings, fail, blocked, unstable, invalid };
std::string_view to_string(ComparisonStatus status) noexcept;
std::ostream& operator<<(std::ostream& out, ComparisonStatus status);

struct Finding {
  // `fail`, `blocked` or `pass_with_warnings`: the status this finding would produce.
  ComparisonStatus severity = ComparisonStatus::fail;
  std::string check;  // e.g. "alignment", "ordering", "values"
  std::string detail;
};

// §9. `invalid` and `blocked` come from the gate; `unstable` from a variant disagreeing
// with itself (§8); a `fail` finding beats a warning; warnings alone are
// `pass_with_warnings`.
ComparisonStatus finalize_status(GateStatus gate, const std::vector<Finding>& findings,
                                 bool unstable = false);

} // namespace mh
