#include "mh/trace_reader.hpp"

#include <cstddef>
#include <initializer_list>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "mh/call_stack.hpp"
#include "mh/contract.hpp"
#include "mh/json_parse.hpp"

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

bool is_object(const json::Json* node) {
  return node != nullptr && node->kind() == json::Json::Kind::object;
}

bool is_array(const json::Json* node) {
  return node != nullptr && node->kind() == json::Json::Kind::array;
}

std::optional<std::string> string_member(const json::Json& object, std::string_view key) {
  const json::Json* node = member(object, key);
  if (node == nullptr || node->kind() != json::Json::Kind::string) {
    return std::nullopt;
  }
  return std::string{node->string_value()};
}

std::optional<std::int64_t> integer_member(const json::Json& object, std::string_view key) {
  const json::Json* node = member(object, key);
  if (node == nullptr || node->kind() != json::Json::Kind::integer) {
    return std::nullopt;
  }
  return node->integer_value();
}

void add_issue(TraceDocument& document, TraceIssueSeverity severity, TraceIssueKind kind,
               std::size_t line, std::string detail) {
  TraceIssue issue;
  issue.severity = severity;
  issue.kind = kind;
  issue.line = line;
  issue.detail = std::move(detail);
  document.issues.push_back(std::move(issue));
}

// "Every key defined by this schema is present even when its value is null. An absent key
// is invalid." Severity is the caller's: the contract's own seven-line example omits
// `meta.mods` and `end.artifacts`, and the note calls that example valid, so those are
// warnings rather than errors (PROGRESS.md, open questions).
void require_keys(TraceDocument& document, const json::Json& record, std::size_t line,
                  std::initializer_list<std::string_view> keys, TraceIssueSeverity severity) {
  for (const std::string_view key : keys) {
    if (member(record, key) == nullptr) {
      add_issue(document, severity, TraceIssueKind::missing_key, line,
                "missing required key " + std::string{key});
    }
  }
}

void validate_capture(TraceDocument& document, const json::Json& record, std::size_t line) {
  const json::Json* capture = member(record, "capture");
  if (!is_object(capture)) {
    add_issue(document, TraceIssueSeverity::error, TraceIssueKind::missing_key, line,
              "capture object missing");
    return;
  }

  const std::optional<std::string> status = string_member(*capture, "status");
  if (!status.has_value()) {
    add_issue(document, TraceIssueSeverity::error, TraceIssueKind::missing_key, line,
              "capture.status missing");
    return;
  }
  if (*status != "complete" && *status != "partial" && *status != "unavailable") {
    add_issue(document, TraceIssueSeverity::error, TraceIssueKind::capture_unknown_status,
              line, "capture.status=" + *status);
    return;
  }
  if (*status == "unavailable") {
    // §Common fields: "No usable observation, or a required field is absent; the record
    // cannot establish its claim." A warning, not an error: "The comparability gate, not
    // the record alone, decides whether a partial field is required." The contract's own
    // seven-line example carries an unavailable save observation and is declared valid.
    add_issue(document, TraceIssueSeverity::warning, TraceIssueKind::capture_incomplete,
              line, "capture.status=unavailable: this record establishes nothing");
  }
  if (*status == "complete") {
    return;
  }

  const json::Json* missing = member(*capture, "missing");
  const json::Json* reasons = member(*capture, "reasons");
  if (!is_array(missing)) {
    add_issue(document, TraceIssueSeverity::error, TraceIssueKind::missing_key, line,
              "capture.missing missing or not an array");
    return;
  }
  if (!is_object(reasons)) {
    add_issue(document, TraceIssueSeverity::error, TraceIssueKind::missing_key, line,
              "capture.reasons missing or not an object");
    return;
  }

  // "A null hash with no capture.reasons entry is invalid."
  for (const json::Json& path : missing->items()) {
    if (path.kind() != json::Json::Kind::string) {
      add_issue(document, TraceIssueSeverity::error, TraceIssueKind::missing_key, line,
                "capture.missing contains a non-string entry");
      continue;
    }
    if (member(*reasons, path.string_value()) == nullptr) {
      add_issue(document, TraceIssueSeverity::error, TraceIssueKind::capture_missing_reason,
                line, "no capture.reasons entry for " + std::string{path.string_value()});
    }
  }
}

void validate_thread(TraceDocument& document, const json::Json& record, std::size_t line) {
  const json::Json* thread = member(record, "thread");
  if (!is_object(thread)) {
    add_issue(document, TraceIssueSeverity::error, TraceIssueKind::missing_key, line,
              "thread object missing");
    return;
  }
  if (!string_member(*thread, "id").has_value()) {
    add_issue(document, TraceIssueSeverity::error, TraceIssueKind::missing_key, line,
              "thread.id missing");
  }
  const std::optional<std::string> role = string_member(*thread, "role");
  if (!role.has_value()) {
    add_issue(document, TraceIssueSeverity::error, TraceIssueKind::missing_key, line,
              "thread.role missing");
    return;
  }
  if (*role != "main" && *role != "render" && *role != "audio" && *role != "worker" &&
      *role != "unknown") {
    add_issue(document, TraceIssueSeverity::error, TraceIssueKind::capture_unknown_status,
              line, "thread.role=" + *role);
  }
}

std::vector<std::string_view> split_lines(std::string_view text) {
  std::vector<std::string_view> lines;
  std::size_t start = 0;
  while (start < text.size()) {
    const std::size_t newline = text.find('\n', start);
    if (newline == std::string_view::npos) {
      lines.push_back(text.substr(start));
      break;
    }
    lines.push_back(text.substr(start, newline - start));
    start = newline + 1;
  }
  return lines;
}

} // namespace

std::string_view to_string(TraceIssueKind kind) noexcept {
  switch (kind) {
    case TraceIssueKind::parse_error:
      return "parse_error";
    case TraceIssueKind::missing_key:
      return "missing_key";
    case TraceIssueKind::wrong_schema:
      return "wrong_schema";
    case TraceIssueKind::unknown_type:
      return "unknown_type";
    case TraceIssueKind::meta_not_first:
      return "meta_not_first";
    case TraceIssueKind::missing_meta:
      return "missing_meta";
    case TraceIssueKind::missing_end:
      return "missing_end";
    case TraceIssueKind::end_not_last:
      return "end_not_last";
    case TraceIssueKind::duplicate_end:
      return "duplicate_end";
    case TraceIssueKind::seq_gap:
      return "seq_gap";
    case TraceIssueKind::run_id_mismatch:
      return "run_id_mismatch";
    case TraceIssueKind::fingerprint_mismatch:
      return "fingerprint_mismatch";
    case TraceIssueKind::capture_unknown_status:
      return "capture_unknown_status";
    case TraceIssueKind::capture_missing_reason:
      return "capture_missing_reason";
    case TraceIssueKind::capture_incomplete:
      return "capture_incomplete";
    case TraceIssueKind::run_failed:
      return "run_failed";
    case TraceIssueKind::dropped_events:
      return "dropped_events";
    case TraceIssueKind::counts_mismatch:
      return "counts_mismatch";
    case TraceIssueKind::unclosed_mismatch:
      return "unclosed_mismatch";
    case TraceIssueKind::call_stack_error:
      return "call_stack_error";
  }
  return {};
}

std::ostream& operator<<(std::ostream& out, TraceIssueKind kind) {
  return out << to_string(kind);
}

bool TraceDocument::has_errors() const noexcept {
  for (const TraceIssue& issue : issues) {
    if (issue.severity == TraceIssueSeverity::error) {
      return true;
    }
  }
  return false;
}

bool TraceDocument::structurally_valid() const noexcept {
  return meta.has_value() && end.has_value() && !has_errors();
}

TraceDocument read_trace(std::string_view jsonl) {
  TraceDocument document;

  std::vector<CallEvent> calls;
  std::vector<std::pair<std::int64_t, std::size_t>> call_lines;  // seq -> line
  std::optional<std::string> run_id;
  std::optional<std::string> run_fingerprint;

  std::int64_t expected_seq = 0;
  std::size_t line_number = 0;
  bool end_seen = false;
  bool first_record = true;

  for (const std::string_view line : split_lines(jsonl)) {
    ++line_number;

    if (line.empty()) {
      add_issue(document, TraceIssueSeverity::error, TraceIssueKind::parse_error, line_number,
                "empty line");
      continue;
    }

    const json::ParseResult parsed = json::parse(line);
    if (!parsed.ok()) {
      add_issue(document, TraceIssueSeverity::error, TraceIssueKind::parse_error, line_number,
                "line " + std::to_string(line_number) + ": " + parsed.error->message +
                    " at offset " + std::to_string(parsed.error->offset));
      continue;
    }
    const json::Json& record = *parsed.value;

    if (!record.is_object()) {
      add_issue(document, TraceIssueSeverity::error, TraceIssueKind::parse_error, line_number,
                "record is not a JSON object");
      continue;
    }

    // -- type and schema -----------------------------------------------------
    const std::optional<std::string> type_text = string_member(record, "type");
    if (!type_text.has_value()) {
      add_issue(document, TraceIssueSeverity::error, TraceIssueKind::missing_key, line_number,
                "type missing");
      continue;
    }
    const std::optional<contract::RecordType> type = contract::parse_record_type(*type_text);
    if (!type.has_value()) {
      add_issue(document, TraceIssueSeverity::error, TraceIssueKind::unknown_type, line_number,
                "type=" + *type_text);
      continue;
    }

    require_keys(document, record, line_number,
                 {"schema", "type", "run_id", "run_fingerprint", "seq", "t_ns"},
                 TraceIssueSeverity::error);

    const std::optional<std::string> schema = string_member(record, "schema");
    if (schema.has_value() && *schema != contract::kTraceSchema) {
      add_issue(document, TraceIssueSeverity::error, TraceIssueKind::wrong_schema, line_number,
                "schema=" + *schema);
    }

    // -- identity ------------------------------------------------------------
    const std::optional<std::string> record_run_id = string_member(record, "run_id");
    if (record_run_id.has_value()) {
      if (!run_id.has_value()) {
        run_id = record_run_id;
      } else if (*run_id != *record_run_id) {
        add_issue(document, TraceIssueSeverity::error, TraceIssueKind::run_id_mismatch,
                  line_number, "run_id changed mid-file");
      }
    }
    const std::optional<std::string> record_fingerprint =
        string_member(record, "run_fingerprint");
    if (record_fingerprint.has_value()) {
      if (!run_fingerprint.has_value()) {
        run_fingerprint = record_fingerprint;
      } else if (*run_fingerprint != *record_fingerprint) {
        add_issue(document, TraceIssueSeverity::error, TraceIssueKind::fingerprint_mismatch,
                  line_number, "run_fingerprint changed mid-file");
      }
    }

    // -- seq -----------------------------------------------------------------
    const std::optional<std::int64_t> seq = integer_member(record, "seq");
    if (seq.has_value()) {
      if (*seq != expected_seq) {
        add_issue(document, TraceIssueSeverity::error, TraceIssueKind::seq_gap, line_number,
                  "expected seq " + std::to_string(expected_seq) + ", got " +
                      std::to_string(*seq));
      }
      expected_seq = *seq + 1;
    }

    validate_capture(document, record, line_number);

    // -- record-type-specific ------------------------------------------------
    switch (*type) {
      case contract::RecordType::meta: {
        if (!first_record) {
          add_issue(document, TraceIssueSeverity::error, TraceIssueKind::meta_not_first,
                    line_number, "meta record is not the first line");
        }
        if (document.meta.has_value()) {
          add_issue(document, TraceIssueSeverity::error, TraceIssueKind::meta_not_first,
                    line_number, "second meta record");
        }
        // The seven-line example in §Complete illustrative trace omits `mods` and `clock`,
        // so absence is a warning, not an error.
        require_keys(document, record, line_number,
                     {"target", "scenario", "environment", "settings", "level"},
                     TraceIssueSeverity::error);
        require_keys(document, record, line_number, {"mods"}, TraceIssueSeverity::warning);
        document.meta = record;
        break;
      }
      case contract::RecordType::event: {
        require_keys(document, record, line_number, {"frame", "thread", "kind"},
                     TraceIssueSeverity::error);
        validate_thread(document, record, line_number);

        const std::optional<std::string> kind = string_member(record, "kind");
        if (!kind.has_value()) {
          add_issue(document, TraceIssueSeverity::error, TraceIssueKind::missing_key,
                    line_number, "kind missing");
          break;
        }

        if (*kind == "hook") {
          const std::optional<std::string> phase = string_member(record, "phase");
          if (!phase.has_value() || (*phase != "enter" && *phase != "exit")) {
            add_issue(document, TraceIssueSeverity::error, TraceIssueKind::missing_key,
                      line_number, "hook event needs phase enter|exit");
          }
          if (!is_object(member(record, "hook"))) {
            add_issue(document, TraceIssueSeverity::error, TraceIssueKind::missing_key,
                      line_number, "hook event needs hook object");
          }
          const json::Json* call = member(record, "call");
          if (!is_object(call)) {
            add_issue(document, TraceIssueSeverity::error, TraceIssueKind::missing_key,
                      line_number, "hook event needs call object");
            break;
          }
          const std::optional<std::string> call_id = string_member(*call, "call_id");
          const std::optional<std::int64_t> depth = integer_member(*call, "depth");
          if (!call_id.has_value() || !depth.has_value()) {
            add_issue(document, TraceIssueSeverity::error, TraceIssueKind::missing_key,
                      line_number, "call needs call_id and depth");
            break;
          }
          if (seq.has_value()) {
            CallEvent event;
            event.seq = *seq;
            const json::Json* thread = member(record, "thread");
            event.thread_id =
                is_object(thread) ? string_member(*thread, "id").value_or("unknown")
                                  : "unknown";
            event.call_id = *call_id;
            event.depth = static_cast<std::int32_t>(*depth);
            if (const std::optional<std::string> parent =
                    string_member(*call, "parent_call_id");
                parent.has_value()) {
              event.parent_call_id = *parent;
            }
            event.phase = *phase == "exit" ? CallPhase::exit : CallPhase::enter;
            calls.push_back(std::move(event));
            call_lines.emplace_back(*seq, line_number);
          }
        }
        document.events.push_back(record);
        break;
      }
      case contract::RecordType::end: {
        if (end_seen) {
          add_issue(document, TraceIssueSeverity::error, TraceIssueKind::duplicate_end,
                    line_number, "second end record");
        }
        end_seen = true;
        require_keys(document, record, line_number,
                     {"status", "exit_code", "frames", "duration_ns", "counts"},
                     TraceIssueSeverity::error);
        require_keys(document, record, line_number, {"artifacts"},
                     TraceIssueSeverity::warning);

        const std::optional<std::string> status = string_member(record, "status");
        if (status.has_value() && *status != "completed" && *status != "stopped" &&
            *status != "crashed" && *status != "timeout" &&
            *status != "harness_error") {
          add_issue(document, TraceIssueSeverity::error, TraceIssueKind::missing_key,
                    line_number, "end.status=" + *status);
        }
        if (status.has_value() && (*status == "crashed" || *status == "timeout")) {
          // §Comparison rules 7: "A crash in either variant is a failure... A timeout is a
          // failure." The reader records it; the comparator refuses to compare.
          add_issue(document, TraceIssueSeverity::error, TraceIssueKind::run_failed,
                    line_number, "end.status=" + *status + ": the run failed");
        }

        const json::Json* counts = member(record, "counts");
        if (is_object(counts)) {
          require_keys(document, *counts, line_number,
                       {"events", "attempted", "dropped", "unclosed", "hook_enter",
                        "hook_exit", "errors"},
                       TraceIssueSeverity::error);
          const std::optional<std::int64_t> dropped = integer_member(*counts, "dropped");
          if (dropped.has_value() && *dropped != 0) {
            // "not eligible for a pass unless the dropped events are proven optional by
            // the manifest" — the manifest decides; the reader reports.
            add_issue(document, TraceIssueSeverity::warning, TraceIssueKind::dropped_events,
                      line_number,
                      std::to_string(*dropped) + " event(s) dropped");
          }
          const std::optional<std::int64_t> events_count = integer_member(*counts, "events");
          if (events_count.has_value() &&
              *events_count != static_cast<std::int64_t>(document.events.size())) {
            add_issue(document, TraceIssueSeverity::error,
                      TraceIssueKind::counts_mismatch, line_number,
                      "counts.events=" + std::to_string(*events_count) + " but " +
                          std::to_string(document.events.size()) + " event record(s) parsed");
          }
        }
        document.end = record;
        break;
      }
    }

    if (end_seen && *type != contract::RecordType::end) {
      add_issue(document, TraceIssueSeverity::error, TraceIssueKind::end_not_last,
                line_number, "record after end");
    }

    first_record = false;
  }

  if (!document.meta.has_value()) {
    add_issue(document, TraceIssueSeverity::error, TraceIssueKind::missing_meta, 0,
              "no meta record");
  }
  if (!document.end.has_value()) {
    // "A trace without an end record is incomplete, not a successful truncated trace."
    add_issue(document, TraceIssueSeverity::error, TraceIssueKind::missing_end, 0,
              "no end record: the trace is incomplete");
  }

  // -- call stack (§Comparison rules 2) --------------------------------------
  const CallStackReport stack = validate_call_stack(calls);
  for (const CallStackError& error : stack.errors) {
    std::size_t line = 0;
    for (const auto& entry : call_lines) {
      if (entry.first == error.seq && error.seq != 0) {
        line = entry.second;
        break;
      }
    }
    add_issue(document, TraceIssueSeverity::error, TraceIssueKind::call_stack_error, line,
              std::string{to_string(error.kind)} + " " + error.call_id + ": " + error.detail);
  }

  if (document.end.has_value()) {
    const json::Json* counts = member(*document.end, "counts");
    if (is_object(counts)) {
      const std::optional<std::int64_t> declared = integer_member(*counts, "unclosed");
      if (declared.has_value() && *declared != stack.unclosed) {
        add_issue(document, TraceIssueSeverity::error, TraceIssueKind::unclosed_mismatch, 0,
                  "end.counts.unclosed=" + std::to_string(*declared) + " but " +
                      std::to_string(stack.unclosed) + " call(s) never exited");
      }
    }
  }

  return document;
}

} // namespace mh
