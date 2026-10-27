#include "mh/call_stack.hpp"

#include <map>
#include <ostream>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace mh {
namespace {

struct Frame {
  std::string call_id;
  std::int32_t depth;
  std::int64_t seq;
};

std::string depth_detail(std::int32_t observed, std::int32_t implied) {
  return "depth " + std::to_string(observed) + " where the open chain implies " +
         std::to_string(implied);
}

const Frame* find_frame(const std::vector<Frame>& stack, const std::string& call_id) {
  for (const Frame& frame : stack) {
    if (frame.call_id == call_id) {
      return &frame;
    }
  }
  return nullptr;
}

void record(CallStackReport& report, CallStackErrorKind kind, const CallEvent& event,
            std::string detail) {
  CallStackError error;
  error.kind = kind;
  error.seq = event.seq;
  error.thread_id = event.thread_id;
  error.call_id = event.call_id;
  error.detail = std::move(detail);
  report.errors.push_back(std::move(error));
}

} // namespace

std::string_view to_string(CallStackErrorKind kind) noexcept {
  switch (kind) {
    case CallStackErrorKind::exit_without_enter:
      return "exit_without_enter";
    case CallStackErrorKind::reused_call_id:
      return "reused_call_id";
    case CallStackErrorKind::depth_mismatch:
      return "depth_mismatch";
    case CallStackErrorKind::parent_mismatch:
      return "parent_mismatch";
    case CallStackErrorKind::not_innermost:
      return "not_innermost";
    case CallStackErrorKind::unclosed_enter:
      return "unclosed_enter";
  }
  return {};
}

std::ostream& operator<<(std::ostream& out, CallStackErrorKind kind) {
  return out << to_string(kind);
}

CallStackReport validate_call_stack(const std::vector<CallEvent>& events) {
  CallStackReport report;

  // One stack per thread, ordered by thread id so the report is deterministic.
  std::map<std::string, std::vector<Frame>> stacks;
  // "unique within a run", so this set is per run, not per thread.
  std::set<std::string> seen_ids;

  for (const CallEvent& event : events) {
    std::vector<Frame>& stack = stacks[event.thread_id];

    const bool first_sighting = seen_ids.insert(event.call_id).second;

    if (event.phase == CallPhase::enter) {
      if (!first_sighting) {
        record(report, CallStackErrorKind::reused_call_id, event,
               "call_id already used in this run");
      }

      const auto implied_depth = static_cast<std::int32_t>(stack.size()) + 1;
      if (event.depth != implied_depth) {
        record(report, CallStackErrorKind::depth_mismatch, event,
               depth_detail(event.depth, implied_depth));
      }

      const std::optional<std::string> implied_parent =
          stack.empty() ? std::nullopt : std::optional<std::string>{stack.back().call_id};
      if (event.parent_call_id != implied_parent) {
        record(report, CallStackErrorKind::parent_mismatch, event,
               implied_parent.has_value() ? "expected parent " + *implied_parent
                                          : "depth 1 must have a null parent");
      }

      // The frame is pushed even when it was already seen: refusing to track it would
      // hide the rest of the thread's chain.
      stack.push_back(Frame{event.call_id, event.depth, event.seq});
      continue;
    }

    // Exit. "Pair every hook enter with the next exit of the same call_id on the same
    // thread."
    if (first_sighting) {
      record(report, CallStackErrorKind::exit_without_enter, event,
             "exit with no earlier enter");
      continue;
    }

    const Frame* const frame = find_frame(stack, event.call_id);
    if (frame == nullptr) {
      record(report, CallStackErrorKind::reused_call_id, event, "call_id already closed");
      continue;
    }

    if (&stack.back() != frame) {
      // Do not silently close the calls above it: they become unclosed, which is what the
      // contract asks for when a call cannot be paired in order.
      record(report, CallStackErrorKind::not_innermost, event,
             "exit of " + event.call_id + " while " + stack.back().call_id + " is open");
      for (auto it = stack.begin(); it != stack.end(); ++it) {
        if (it->call_id == event.call_id) {
          stack.erase(it);
          break;
        }
      }
      continue;
    }

    if (event.depth != frame->depth) {
      record(report, CallStackErrorKind::depth_mismatch, event,
             depth_detail(event.depth, frame->depth));
    }
    stack.pop_back();
    ++report.paired;
  }

  for (const auto& entry : stacks) {
    for (const Frame& frame : entry.second) {
      ++report.unclosed;
      CallStackError error;
      error.kind = CallStackErrorKind::unclosed_enter;
      error.seq = frame.seq;
      error.thread_id = entry.first;
      error.call_id = frame.call_id;
      error.detail = "enter without matching exit";
      report.errors.push_back(std::move(error));
    }
  }

  return report;
}

} // namespace mh
