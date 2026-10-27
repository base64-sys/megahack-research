#include "mh/trace_writer.hpp"

#include <array>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>

#include "mh/contract.hpp"

namespace mh {
namespace {

// Keys the writer owns. A payload that tries to set one would silently desynchronise the
// file from its own counters, so it is rejected instead.
constexpr std::array<std::string_view, 9> kReservedKeys = {
    "schema", "type", "run_id", "run_fingerprint", "seq",
    "t_ns",   "frame", "thread",           "capture",
};

bool is_reserved(std::string_view key) {
  for (const std::string_view reserved : kReservedKeys) {
    if (key == reserved) {
      return true;
    }
  }
  return false;
}

void merge_payload(json::Json& record, const json::Json& payload) {
  if (!payload.is_object()) {
    throw std::invalid_argument("trace: record payload must be a JSON object");
  }
  for (const auto& member : payload.members()) {
    if (is_reserved(member.first)) {
      throw std::invalid_argument("trace: payload must not set envelope key " + member.first);
    }
    record.set(member.first, member.second);
  }
}

// Payload lookup that does not disturb the record: the writer only reads `kind` and
// `phase`, everything else is opaque to it.
const json::Json* find_member(const json::Json& object, std::string_view key) {
  for (const auto& member : object.members()) {
    if (member.first == key) {
      return &member.second;
    }
  }
  return nullptr;
}

std::string_view string_member(const json::Json& object, std::string_view key) {
  const json::Json* member = find_member(object, key);
  if (member == nullptr || member->kind() != json::Json::Kind::string) {
    return {};
  }
  return member->string_value();
}

} // namespace

std::string_view to_string(CaptureStatus status) noexcept {
  switch (status) {
    case CaptureStatus::complete:
      return "complete";
    case CaptureStatus::partial:
      return "partial";
    case CaptureStatus::unavailable:
      return "unavailable";
  }
  return {};
}

std::string_view to_string(ThreadRole role) noexcept {
  switch (role) {
    case ThreadRole::main:
      return "main";
    case ThreadRole::render:
      return "render";
    case ThreadRole::audio:
      return "audio";
    case ThreadRole::worker:
      return "worker";
    case ThreadRole::unknown:
      return "unknown";
  }
  return {};
}

std::string_view to_string(EndStatus status) noexcept {
  switch (status) {
    case EndStatus::completed:
      return "completed";
    case EndStatus::stopped:
      return "stopped";
    case EndStatus::crashed:
      return "crashed";
    case EndStatus::timeout:
      return "timeout";
    case EndStatus::harness_error:
      return "harness_error";
  }
  return {};
}

json::Json Capture::to_json() const {
  json::Json object = json::Json::object();
  object.set("status", json::Json::string(to_string(status)));

  if (status != CaptureStatus::complete) {
    // §Common fields: the two lists only exist to name what is missing and why. An empty
    // list still means "nothing was missing", which `complete` already says.
    json::Json missing_list = json::Json::array();
    for (const std::string& path : missing) {
      missing_list.push(json::Json::string(path));
    }
    object.set("missing", missing_list);

    json::Json reason_map = json::Json::object();
    for (const auto& reason : reasons) {
      reason_map.set(reason.first, json::Json::string(reason.second));
    }
    object.set("reasons", reason_map);
  }

  return object;
}

json::Json ThreadInfo::to_json() const {
  json::Json object = json::Json::object();
  object.set("id", json::Json::string(id));
  object.set("role", json::Json::string(to_string(role)));
  return object;
}

json::Json TraceCounts::to_json() const {
  json::Json object = json::Json::object();
  object.set("events", json::Json::integer(events));
  object.set("attempted", json::Json::integer(attempted));
  object.set("dropped", json::Json::integer(dropped));
  object.set("unclosed", json::Json::integer(unclosed));
  object.set("hook_enter", json::Json::integer(hook_enter));
  object.set("hook_exit", json::Json::integer(hook_exit));
  object.set("errors", json::Json::integer(errors));
  return object;
}

json::Json TraceArtifacts::to_json() const {
  // "Every key defined by this schema is present even when its value is null."
  const auto hash_or_null = [](const std::optional<std::string>& hash) {
    return hash.has_value() ? json::Json::string(*hash) : json::Json::null();
  };

  json::Json object = json::Json::object();
  object.set("stdout_sha256", hash_or_null(stdout_sha256));
  object.set("stderr_sha256", hash_or_null(stderr_sha256));
  object.set("minidump_sha256", hash_or_null(minidump_sha256));
  return object;
}

TraceWriter::TraceWriter(std::ostream& sink, TraceHeader header)
    : sink_(sink), header_(std::move(header)) {}

bool TraceWriter::emit(const json::Json& record) {
  const std::string line = record.to_json();
  sink_.write(line.data(), static_cast<std::streamsize>(line.size()));
  sink_.put('\n');
  sink_.flush();
  return sink_.good();
}

bool TraceWriter::write_meta(const json::Json& body, const Capture& capture) {
  if (meta_written_) {
    throw std::logic_error("trace: meta written twice");
  }

  json::Json record = json::Json::object();
  record.set("schema", json::Json::string(contract::kTraceSchema));
  record.set("type", json::Json::string(contract::kTypeMeta));
  record.set("run_id", json::Json::string(header_.run_id));
  record.set("run_fingerprint", json::Json::string(header_.run_fingerprint));
  record.set("seq", json::Json::integer(next_seq_));
  record.set("t_ns", json::Json::integer(0));
  merge_payload(record, body);
  record.set("capture", capture.to_json());

  // seq is consumed even if the write fails: a gap plus a dropped meta is visible, a
  // reused seq is not.
  ++next_seq_;
  const bool written = emit(record);
  meta_written_ = written;
  // A lost meta must not also lose the evidence of the loss: events stay attemptable so
  // `end.counts.dropped` can report what happened.
  meta_attempted_ = true;
  return written;
}

bool TraceWriter::write_event(const json::Json& body, const EventContext& context) {
  if (!meta_attempted_) {
    throw std::logic_error("trace: event before meta");
  }
  if (end_written_) {
    throw std::logic_error("trace: event after end");
  }

  bool has_kind = false;
  for (const auto& member : body.members()) {
    if (member.first == "kind") {
      has_kind = true;
    }
  }
  if (!has_kind) {
    throw std::invalid_argument("trace: event payload requires kind");
  }

  // "seq is assigned before the write is attempted."
  const std::int64_t seq = next_seq_++;
  ++counts_.attempted;

  const std::string_view kind = string_member(body, "kind");
  if (kind.empty()) {
    throw std::invalid_argument("trace: event kind must be a string");
  }

  json::Json record = json::Json::object();
  record.set("schema", json::Json::string(contract::kTraceSchema));
  record.set("type", json::Json::string(contract::kTypeEvent));
  record.set("run_id", json::Json::string(header_.run_id));
  record.set("run_fingerprint", json::Json::string(header_.run_fingerprint));
  record.set("seq", json::Json::integer(seq));
  record.set("t_ns", json::Json::integer(context.t_ns));
  record.set("frame", context.frame.has_value() ? json::Json::integer(*context.frame)
                                                : json::Json::null());
  record.set("thread", context.thread.to_json());
  merge_payload(record, body);
  record.set("capture", context.capture.to_json());

  if (!emit(record)) {
    ++counts_.dropped;
    return false;
  }

  ++counts_.events;
  // §end record: hook_enter/hook_exit/errors are derivable from the payloads, so the
  // writer derives them rather than trusting a caller's arithmetic.
  if (kind == "hook") {
    const std::string_view phase = string_member(body, "phase");
    if (phase == "enter") {
      ++counts_.hook_enter;
    } else if (phase == "exit") {
      ++counts_.hook_exit;
    }
  } else if (kind == "error") {
    ++counts_.errors;
  }
  return true;
}

bool TraceWriter::write_end(const EndInfo& info) {
  if (!meta_attempted_) {
    throw std::logic_error("trace: end before meta");
  }
  if (end_written_) {
    throw std::logic_error("trace: end written twice");
  }

  counts_.unclosed = info.unclosed;

  json::Json record = json::Json::object();
  record.set("schema", json::Json::string(contract::kTraceSchema));
  record.set("type", json::Json::string(contract::kTypeEnd));
  record.set("run_id", json::Json::string(header_.run_id));
  record.set("run_fingerprint", json::Json::string(header_.run_fingerprint));
  record.set("seq", json::Json::integer(next_seq_++));
  record.set("t_ns", json::Json::integer(info.t_ns));
  record.set("status", json::Json::string(to_string(info.status)));
  record.set("exit_code", json::Json::integer(info.exit_code));
  record.set("frames", json::Json::integer(info.frames));
  record.set("duration_ns", json::Json::integer(info.duration_ns));
  record.set("counts", counts_.to_json());
  record.set("artifacts", info.artifacts.to_json());
  record.set("capture", info.capture.to_json());

  const bool written = emit(record);
  end_written_ = written;
  return written;
}

std::ofstream open_trace_file(const std::string& path) {
  return std::ofstream(path, std::ios::binary | std::ios::trunc | std::ios::out);
}

} // namespace mh
