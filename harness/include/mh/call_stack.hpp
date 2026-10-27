// Call pairing and call-stack validation.
//
// Source: notes/runtime-validation.md §Call fields and §Comparison rules 2:
//
//   - "call.call_id is unique within a run. call.depth is the nesting depth on that
//     thread, starting at 1 for the outermost observed call; parent_call_id is the
//     enclosing observed call, or null."
//   - "Pair every hook enter with the next exit of the same call_id on the same thread.
//     depth must be consistent with the observed parent chain. An enter without an exit,
//     an exit without an enter, a reused call_id, or a depth jump is a trace error. A
//     trace with a call-stack error cannot produce a pass, even if the payloads look
//     equal."
//   - "A hook enter without a matching exit is an incomplete call: record it at `end` as
//     counts.unclosed, and do not silently close it."
//
// The validator is a pure function over hook events in file order, because that is what
// the trace reader has after parsing (item 6) and it keeps the rules testable without a
// live observer.

#pragma once

#include <cstdint>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mh {

enum class CallPhase { enter, exit };

// The subset of a hook event these rules need.
struct CallEvent {
  std::int64_t seq = 0;
  std::string thread_id;
  std::string call_id;
  std::optional<std::string> parent_call_id;
  std::int32_t depth = 0;
  CallPhase phase = CallPhase::enter;
};

enum class CallStackErrorKind {
  exit_without_enter,
  reused_call_id,
  depth_mismatch,
  parent_mismatch,
  not_innermost,
  unclosed_enter,
};

std::string_view to_string(CallStackErrorKind kind) noexcept;
std::ostream& operator<<(std::ostream& out, CallStackErrorKind kind);

struct CallStackError {
  CallStackErrorKind kind = CallStackErrorKind::depth_mismatch;
  std::int64_t seq = 0;
  std::string thread_id;
  std::string call_id;
  std::string detail;
};

struct CallStackReport {
  std::vector<CallStackError> errors;
  // Enter/exit pairs matched in order.
  std::int64_t paired = 0;
  // Enters still open at the end of the file; this is `end.counts.unclosed`.
  std::int64_t unclosed = 0;

  bool clean() const noexcept { return errors.empty() && unclosed == 0; }
};

CallStackReport validate_call_stack(const std::vector<CallEvent>& events);

} // namespace mh
