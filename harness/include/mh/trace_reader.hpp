// Trace reader and structural validator.
//
// Source: notes/runtime-validation.md §Trace format, §Common fields, §Call fields,
// §`end` record, and §Comparison rules 2. What is decided here:
//
//   - "A trace without an `end` record is incomplete, not a successful truncated trace."
//   - "A trace whose `seq` has a gap, or whose `end.counts.dropped` is non-zero, is not
//     eligible for a pass unless the dropped events are proven optional by the manifest."
//   - "Every key defined by this schema is present even when its value is null. An absent
//     key is invalid."
//   - "A `null` hash with no `capture.reasons` entry is invalid."
//   - "A trace with a call-stack error cannot produce a pass."
//
// The reader reports; it does not compare. Whether a `partial` capture is acceptable is
// the comparability gate's question (§Comparability gate), not the record's.

#pragma once

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "mh/json.hpp"

namespace mh {

enum class TraceIssueSeverity { warning, error };

enum class TraceIssueKind {
  parse_error,
  missing_key,
  wrong_schema,
  unknown_type,
  meta_not_first,
  missing_meta,
  missing_end,
  end_not_last,
  duplicate_end,
  seq_gap,
  run_id_mismatch,
  fingerprint_mismatch,
  capture_unknown_status,
  capture_missing_reason,
  capture_incomplete,
  run_failed,
  dropped_events,
  counts_mismatch,
  unclosed_mismatch,
  call_stack_error,
};

std::string_view to_string(TraceIssueKind kind) noexcept;
std::ostream& operator<<(std::ostream& out, TraceIssueKind kind);

struct TraceIssue {
  TraceIssueSeverity severity = TraceIssueSeverity::error;
  TraceIssueKind kind = TraceIssueKind::missing_key;
  // 1-based line in the JSONL file; 0 for a document-level finding.
  std::size_t line = 0;
  std::string detail;
};

struct TraceDocument {
  std::optional<json::Json> meta;
  std::vector<json::Json> events;
  std::optional<json::Json> end;
  std::vector<TraceIssue> issues;

  // No error-severity issue: the file is structurally a trace. It says nothing about
  // comparability, which is the gate's job.
  bool structurally_valid() const noexcept;
  bool has_errors() const noexcept;
};

// Parses JSONL (one object per line, LF separated) and validates structure, sequencing,
// capture bookkeeping and the call stack.
TraceDocument read_trace(std::string_view jsonl);

} // namespace mh
