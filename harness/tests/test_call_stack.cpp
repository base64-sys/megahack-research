// Item 5 — call pairing and call-stack validation.
//
// Contract: notes/runtime-validation.md §Call fields ("call_id is unique within a run",
// "depth is the nesting depth on that thread, starting at 1", "parent_call_id is the
// enclosing observed call, or null") and §Comparison rules 2 ("Pair every hook enter with
// the next exit of the same call_id on the same thread. depth must be consistent with the
// observed parent chain. An enter without an exit, an exit without an enter, a reused
// call_id, or a depth jump is a trace error. A trace with a call-stack error cannot
// produce a pass, even if the payloads look equal.").
//
// §Call fields also fixes the reporting: an enter without an exit is "recorded at end as
// counts.unclosed, and do not silently close it".

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "mh/call_stack.hpp"
#include "test_runner.hpp"

namespace {

mh::CallEvent enter(std::int64_t seq, std::string thread, std::string call_id, int depth,
                    std::optional<std::string> parent = std::nullopt) {
  mh::CallEvent event;
  event.seq = seq;
  event.thread_id = std::move(thread);
  event.call_id = std::move(call_id);
  event.depth = depth;
  event.parent_call_id = std::move(parent);
  event.phase = mh::CallPhase::enter;
  return event;
}

mh::CallEvent exit(std::int64_t seq, std::string thread, std::string call_id, int depth) {
  mh::CallEvent event;
  event.seq = seq;
  event.thread_id = std::move(thread);
  event.call_id = std::move(call_id);
  event.depth = depth;
  event.phase = mh::CallPhase::exit;
  return event;
}

bool has_kind(const mh::CallStackReport& report, mh::CallStackErrorKind kind) {
  for (const mh::CallStackError& error : report.errors) {
    if (error.kind == kind) {
      return true;
    }
  }
  return false;
}

} // namespace

MH_TEST(call_stack_pairs_nested_calls_per_thread) {
  const std::vector<mh::CallEvent> events = {
      enter(1, "main", "c1", 1),
      enter(2, "main", "c2", 2, "c1"),
      exit(3, "main", "c2", 2),
      exit(4, "main", "c1", 1),
  };

  const mh::CallStackReport report = mh::validate_call_stack(events);
  MH_CHECK(report.errors.empty());
  MH_CHECK_EQ(report.unclosed, std::int64_t{0});
  MH_CHECK_EQ(report.paired, std::int64_t{2});
  MH_CHECK(report.clean());
}

MH_TEST(call_stack_counts_unclosed_enters_without_closing_them) {
  const std::vector<mh::CallEvent> events = {
      enter(1, "main", "c1", 1),
      enter(2, "main", "c2", 2, "c1"),
      exit(3, "main", "c2", 2),
  };

  const mh::CallStackReport report = mh::validate_call_stack(events);
  MH_CHECK_EQ(report.unclosed, std::int64_t{1});
  MH_CHECK_EQ(report.paired, std::int64_t{1});
  MH_CHECK(has_kind(report, mh::CallStackErrorKind::unclosed_enter));
  MH_CHECK(!report.clean());

  // The unclosed call is reported with its identity, so `end.counts.unclosed` can be
  // explained rather than merely counted.
  bool found_c1 = false;
  for (const mh::CallStackError& error : report.errors) {
    if (error.kind == mh::CallStackErrorKind::unclosed_enter && error.call_id == "c1") {
      found_c1 = true;
      MH_CHECK_EQ(error.seq, std::int64_t{1});
    }
  }
  MH_CHECK(found_c1);
}

MH_TEST(call_stack_rejects_exit_without_enter) {
  const std::vector<mh::CallEvent> events = {
      exit(1, "main", "c9", 1),
  };

  const mh::CallStackReport report = mh::validate_call_stack(events);
  MH_CHECK_EQ(report.errors.size(), std::size_t{1});
  MH_CHECK_EQ(report.errors[0].kind, mh::CallStackErrorKind::exit_without_enter);
  MH_CHECK_EQ(report.errors[0].call_id, std::string{"c9"});
  MH_CHECK_EQ(report.unclosed, std::int64_t{0});
}

MH_TEST(call_stack_rejects_reused_call_id) {
  // "A call id reused within a run is invalid" — including a second enter while the
  // first is still open.
  const std::vector<mh::CallEvent> events = {
      enter(1, "main", "c1", 1),
      enter(2, "main", "c1", 2, "c1"),
      exit(3, "main", "c1", 2),
      exit(4, "main", "c1", 1),
  };

  const mh::CallStackReport report = mh::validate_call_stack(events);
  MH_CHECK(has_kind(report, mh::CallStackErrorKind::reused_call_id));
  MH_CHECK(!report.clean());
}

MH_TEST(call_stack_rejects_reused_id_after_close) {
  const std::vector<mh::CallEvent> events = {
      enter(1, "main", "c1", 1),
      exit(2, "main", "c1", 1),
      enter(3, "main", "c1", 1),
  };

  const mh::CallStackReport report = mh::validate_call_stack(events);
  MH_CHECK(has_kind(report, mh::CallStackErrorKind::reused_call_id));
}

MH_TEST(call_stack_is_unique_across_threads) {
  // "unique within a run", not within a thread. Two threads cannot both own c1.
  const std::vector<mh::CallEvent> events = {
      enter(1, "main", "c1", 1),
      enter(2, "render", "c1", 1),
  };

  const mh::CallStackReport report = mh::validate_call_stack(events);
  MH_CHECK(has_kind(report, mh::CallStackErrorKind::reused_call_id));
}

MH_TEST(call_stack_allows_independent_stacks_per_thread) {
  const std::vector<mh::CallEvent> events = {
      enter(1, "main", "c1", 1),
      enter(2, "render", "c2", 1),
      exit(3, "render", "c2", 1),
      exit(4, "main", "c1", 1),
  };

  const mh::CallStackReport report = mh::validate_call_stack(events);
  MH_CHECK(report.clean());
  MH_CHECK_EQ(report.paired, std::int64_t{2});
}

MH_TEST(call_stack_rejects_depth_jump) {
  // Outermost observed call is depth 1 by definition; 3 is a jump.
  const std::vector<mh::CallEvent> events = {
      enter(1, "main", "c1", 3),
  };

  const mh::CallStackReport report = mh::validate_call_stack(events);
  MH_CHECK(has_kind(report, mh::CallStackErrorKind::depth_mismatch));
}

MH_TEST(call_stack_rejects_shallower_depth_than_the_chain) {
  const std::vector<mh::CallEvent> events = {
      enter(1, "main", "c1", 1),
      enter(2, "main", "c2", 1),
  };

  const mh::CallStackReport report = mh::validate_call_stack(events);
  MH_CHECK(has_kind(report, mh::CallStackErrorKind::depth_mismatch));
}

MH_TEST(call_stack_rejects_parent_that_is_not_the_enclosing_call) {
  const std::vector<mh::CallEvent> events = {
      enter(1, "main", "c1", 1),
      enter(2, "main", "c2", 2, "cX"),
  };

  const mh::CallStackReport report = mh::validate_call_stack(events);
  MH_CHECK(has_kind(report, mh::CallStackErrorKind::parent_mismatch));
}

MH_TEST(call_stack_requires_null_parent_at_depth_one) {
  const std::vector<mh::CallEvent> events = {
      enter(1, "main", "c1", 1, "c0"),
  };

  const mh::CallStackReport report = mh::validate_call_stack(events);
  MH_CHECK(has_kind(report, mh::CallStackErrorKind::parent_mismatch));
}

MH_TEST(call_stack_rejects_exit_of_a_non_innermost_call) {
  // c1 is exited while c2 is still open: the pairing rule is "the next exit of the same
  // call_id", so c2 can never be paired after this. c2 stays open and is counted.
  const std::vector<mh::CallEvent> events = {
      enter(1, "main", "c1", 1),
      enter(2, "main", "c2", 2, "c1"),
      exit(3, "main", "c1", 1),
  };

  const mh::CallStackReport report = mh::validate_call_stack(events);
  MH_CHECK(has_kind(report, mh::CallStackErrorKind::not_innermost));
  MH_CHECK_EQ(report.unclosed, std::int64_t{1});
}

MH_TEST(call_stack_rejects_exit_depth_that_does_not_match_the_enter) {
  const std::vector<mh::CallEvent> events = {
      enter(1, "main", "c1", 1),
      exit(2, "main", "c1", 2),
  };

  const mh::CallStackReport report = mh::validate_call_stack(events);
  MH_CHECK(has_kind(report, mh::CallStackErrorKind::depth_mismatch));
}

MH_TEST(call_stack_ignores_an_empty_event_list) {
  const mh::CallStackReport report = mh::validate_call_stack({});
  MH_CHECK(report.clean());
  MH_CHECK_EQ(report.paired, std::int64_t{0});
  MH_CHECK_EQ(report.unclosed, std::int64_t{0});
}
