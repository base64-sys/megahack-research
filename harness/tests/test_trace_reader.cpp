// Item 6b — trace reader and structural validator.
//
// Contract: notes/runtime-validation.md §Trace format, §Common fields, §`end` record,
// §Comparison rules 2 and 7. The positive fixture is the contract's own seven-line example
// (tests/fixtures/example_trace.hpp, generated from the note), because the note declares
// that example valid; every negative case is that file with one thing broken.

#include <string>
#include <string_view>
#include <vector>

#include "fixtures/example_trace.hpp"
#include "mh/trace_reader.hpp"
#include "test_runner.hpp"

namespace {

using mh::TraceDocument;
using mh::TraceIssue;
using mh::TraceIssueKind;
using mh::TraceIssueSeverity;

std::string example() { return std::string{mh_test_fixtures::kExampleTrace}; }

bool has_error(const TraceDocument& document, TraceIssueKind kind) {
  for (const TraceIssue& issue : document.issues) {
    if (issue.severity == TraceIssueSeverity::error && issue.kind == kind) {
      return true;
    }
  }
  return false;
}

bool has_warning(const TraceDocument& document, TraceIssueKind kind) {
  for (const TraceIssue& issue : document.issues) {
    if (issue.severity == TraceIssueSeverity::warning && issue.kind == kind) {
      return true;
    }
  }
  return false;
}

std::vector<std::string> lines_of(const std::string& text) {
  std::vector<std::string> lines;
  std::string current;
  for (const char c : text) {
    if (c == '\n') {
      if (!current.empty()) {
        lines.push_back(current);
      }
      current.clear();
    } else {
      current += c;
    }
  }
  if (!current.empty()) {
    lines.push_back(current);
  }
  return lines;
}

std::string join(const std::vector<std::string>& lines) {
  std::string text;
  for (const std::string& line : lines) {
    text += line;
    text += '\n';
  }
  return text;
}

// Removes every line containing `needle`; returns the text unchanged when nothing matches
// (so a broken fixture fails loudly in the assertion, not silently).
std::string without(const std::string& text, std::string_view needle) {
  std::vector<std::string> kept;
  for (const std::string& line : lines_of(text)) {
    if (line.find(needle) == std::string::npos) {
      kept.push_back(line);
    }
  }
  return join(kept);
}

std::string replaced(const std::string& text, std::string_view from, std::string_view to) {
  std::string copy = text;
  const std::size_t at = copy.find(from);
  if (at != std::string::npos) {
    copy.replace(at, from.size(), to);
  }
  return copy;
}

} // namespace

MH_TEST(trace_reader_accepts_the_contract_example_trace) {
  const TraceDocument document = mh::read_trace(example());

  MH_CHECK(!document.has_errors());
  MH_CHECK(document.structurally_valid());
  MH_CHECK(document.meta.has_value());
  MH_CHECK(document.end.has_value());
  MH_CHECK_EQ(document.events.size(), std::size_t{5});

  // The save observation in the example is deliberately unavailable. That is a warning
  // about evidence, not a structural error: the note calls the file valid.
  MH_CHECK(has_warning(document, TraceIssueKind::capture_incomplete));
}

MH_TEST(trace_reader_requires_an_end_record) {
  const TraceDocument document = mh::read_trace(without(example(), R"("type":"end")"));

  MH_CHECK(has_error(document, TraceIssueKind::missing_end));
  MH_CHECK(!document.structurally_valid());
  // The events are still readable: a truncated trace is incomplete, not unreadable.
  MH_CHECK_EQ(document.events.size(), std::size_t{5});
}

MH_TEST(trace_reader_rejects_a_seq_gap) {
  const TraceDocument document = mh::read_trace(without(example(), R"("seq":3,)"));

  MH_CHECK(has_error(document, TraceIssueKind::seq_gap));
  MH_CHECK(!document.structurally_valid());
}

MH_TEST(trace_reader_rejects_a_parse_error_and_names_the_line) {
  std::vector<std::string> lines = lines_of(example());
  lines.insert(lines.begin() + 2, "not json");
  const TraceDocument document = mh::read_trace(join(lines));

  MH_CHECK(has_error(document, TraceIssueKind::parse_error));
  bool line_reported = false;
  for (const TraceIssue& issue : document.issues) {
    if (issue.kind == TraceIssueKind::parse_error && issue.line == 3) {
      line_reported = true;
    }
  }
  MH_CHECK(line_reported);
}

MH_TEST(trace_reader_rejects_the_wrong_schema_string) {
  const TraceDocument document =
      mh::read_trace(replaced(example(), R"("schema":"mh-trace/1")", R"("schema":"mh-trace/2")"));

  MH_CHECK(has_error(document, TraceIssueKind::wrong_schema));
}

MH_TEST(trace_reader_rejects_run_identity_changing_mid_file) {
  // Sanity: the untouched fixture is accepted, so the mutation below is the only change.
  MH_CHECK(!mh::read_trace(example()).has_errors());

  std::vector<std::string> lines = lines_of(example());
  lines[4] = replaced(lines[4], "00000000-0000-4000-8000-000000000001",
                      "00000000-0000-4000-8000-000000000009");
  const TraceDocument mutated = mh::read_trace(join(lines));
  MH_CHECK(has_error(mutated, TraceIssueKind::run_id_mismatch));
}

MH_TEST(trace_reader_rejects_capture_without_a_reason) {
  // §meta record: "A null hash with no capture.reasons entry is invalid." Rename the
  // reason key so the document stays well-formed and only the bookkeeping is wrong.
  const TraceDocument document = mh::read_trace(
      replaced(example(), R"("environment.geode.sha256":"not hashed")",
               R"("environment.geode.sha256.renamed":"not hashed")"));
  MH_CHECK(has_error(document, TraceIssueKind::capture_missing_reason));
}

MH_TEST(trace_reader_rejects_a_record_after_end) {
  std::string text = example();
  text += '\n';
  text +=
      R"({"schema":"mh-trace/1","type":"event","run_id":"00000000-0000-4000-8000-000000000001",)"
      R"("run_fingerprint":"0000000000000000000000000000000000000000000000000000000000000000",)"
      R"("seq":7,"t_ns":500,"frame":1,"thread":{"id":"main","role":"main"},)"
      R"("kind":"milestone","name":"run_end","data":{"state":"null"},)"
      R"("capture":{"status":"complete"}})";
  text += '\n';

  const TraceDocument document = mh::read_trace(text);
  MH_CHECK(has_error(document, TraceIssueKind::end_not_last));
}

MH_TEST(trace_reader_detects_an_unclosed_call) {
  // Drop the exit: the enter never pairs, and end.counts.unclosed is now a lie.
  const TraceDocument document =
      mh::read_trace(without(example(), R"("phase":"exit")"));

  MH_CHECK(has_error(document, TraceIssueKind::call_stack_error));
  MH_CHECK(has_error(document, TraceIssueKind::unclosed_mismatch));
}

MH_TEST(trace_reader_rejects_counts_that_disagree_with_the_events) {
  const TraceDocument document =
      mh::read_trace(replaced(example(), R"("counts":{"events":5,)", R"("counts":{"events":4,)"));

  MH_CHECK(has_error(document, TraceIssueKind::counts_mismatch));
}

MH_TEST(trace_reader_reports_dropped_events_as_a_warning) {
  const TraceDocument document = mh::read_trace(
      replaced(example(), R"("attempted":5,"dropped":0,)", R"("attempted":5,"dropped":2,)"));

  // Dropped events make the run ineligible unless the manifest proves them optional, so
  // the reader reports and the gate decides.
  MH_CHECK(has_warning(document, TraceIssueKind::dropped_events));
  MH_CHECK(!has_error(document, TraceIssueKind::dropped_events));
}

MH_TEST(trace_reader_rejects_a_crashed_run) {
  const TraceDocument document =
      mh::read_trace(replaced(example(), R"("status":"completed")", R"("status":"crashed")"));

  MH_CHECK(has_error(document, TraceIssueKind::run_failed));
  MH_CHECK(!document.structurally_valid());
}

MH_TEST(trace_reader_rejects_non_object_and_empty_lines) {
  MH_CHECK(has_error(mh::read_trace("[1,2,3]\n"), TraceIssueKind::parse_error));

  std::vector<std::string> lines = lines_of(example());
  lines.insert(lines.begin() + 1, "");
  MH_CHECK(has_error(mh::read_trace(join(lines)), TraceIssueKind::parse_error));
}
