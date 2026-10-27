// Trace writer: meta / event / end, JSONL, seq, counters.
//
// Source: notes/runtime-validation.md §Trace format, §Common fields, §`end` record.
// The rules this unit owns:
//
//   - "A trace is UTF-8 JSON Lines (JSONL): one complete JSON object per line, no
//     comments, no BOM. Every line carries "schema": "mh-trace/1"."
//   - "`seq` is assigned before the write is attempted. `end.counts.attempted` and
//     `end.counts.dropped` make a write failure visible; a gap without that explanation
//     is invalid."
//   - "`counts.attempted` is the number of event records the harness attempted to write;
//     meta and end are excluded."
//   - "A trace without an `end` record is incomplete, not a successful truncated trace."
//
// The writer owns identity, ordering and counting. The record *payload* is the observer's
// JSON: this unit refuses to let a payload overwrite an envelope key, and refuses an
// event without a `kind`, but it does not invent payload fields.

#pragma once

#include <cstdint>
#include <fstream>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "mh/json.hpp"

namespace mh {

// §Common fields: capture.status has exactly three values.
enum class CaptureStatus { complete, partial, unavailable };
std::string_view to_string(CaptureStatus status) noexcept;

// §Common fields: { "status": ..., "missing": [paths], "reasons": {path: reason} }.
// `complete` carries neither list: a complete observation has nothing to explain.
struct Capture {
  CaptureStatus status = CaptureStatus::complete;
  std::vector<std::string> missing;
  std::vector<std::pair<std::string, std::string>> reasons;

  json::Json to_json() const;
};

// §Common fields: thread.role. "Do not guess a role from a thread name", so unknown is
// the default and main must be proven by the harness.
enum class ThreadRole { main, render, audio, worker, unknown };
std::string_view to_string(ThreadRole role) noexcept;

struct ThreadInfo {
  std::string id = "unknown";
  ThreadRole role = ThreadRole::unknown;

  json::Json to_json() const;
};

// §`end` record: counts. events/attempted/dropped/hook_enter/hook_exit/errors are
// maintained by the writer; unclosed is supplied by call-stack validation (item 5),
// because only it can see an enter that never exits.
struct TraceCounts {
  std::int64_t events = 0;
  std::int64_t attempted = 0;
  std::int64_t dropped = 0;
  std::int64_t unclosed = 0;
  std::int64_t hook_enter = 0;
  std::int64_t hook_exit = 0;
  std::int64_t errors = 0;

  json::Json to_json() const;
};

// The per-file identity every record repeats (notes §Common fields).
struct TraceHeader {
  std::string run_id;
  std::string run_fingerprint;
};

// Per-event common fields.
struct EventContext {
  std::int64_t t_ns = 0;
  std::optional<std::int64_t> frame;
  ThreadInfo thread;
  Capture capture;
};

// §`end` record: hashes of sidecars, never a self-hash of the trace file.
struct TraceArtifacts {
  std::optional<std::string> stdout_sha256;
  std::optional<std::string> stderr_sha256;
  std::optional<std::string> minidump_sha256;

  json::Json to_json() const;
};

// §`end` record: status vocabulary. "A crash or timeout is a failed run, not a successful
// comparison."
enum class EndStatus { completed, stopped, crashed, timeout, harness_error };
std::string_view to_string(EndStatus status) noexcept;

struct EndInfo {
  EndStatus status = EndStatus::completed;
  std::int64_t exit_code = 0;
  std::int64_t frames = 0;
  std::int64_t duration_ns = 0;
  std::int64_t t_ns = 0;
  std::int64_t unclosed = 0;
  TraceArtifacts artifacts;
  Capture capture;
};

class TraceWriter {
 public:
  TraceWriter(std::ostream& sink, TraceHeader header);

  // Must be called once, before any other record. `body` carries the meta payload
  // (target, scenario, environment, mods, settings, level, ...); the writer adds the
  // envelope and `capture`.
  bool write_meta(const json::Json& body, const Capture& capture);

  // `body` must contain `kind`. The envelope plus frame/thread/capture is added. Returns
  // false when the record was attempted and dropped; the counters explain it at `end`.
  bool write_event(const json::Json& body, const EventContext& context);

  // Writes the end record. After this the trace is closed and further records throw.
  bool write_end(const EndInfo& info);

  const TraceCounts& counts() const noexcept { return counts_; }
  std::int64_t next_seq() const noexcept { return next_seq_; }
  bool meta_written() const noexcept { return meta_written_; }
  bool end_written() const noexcept { return end_written_; }

 private:
  bool emit(const json::Json& record);

  std::ostream& sink_;
  TraceHeader header_;
  TraceCounts counts_;
  std::int64_t next_seq_ = 0;
  bool meta_written_ = false;
  bool meta_attempted_ = false;
  bool end_written_ = false;
};

// Binary mode so Windows never rewrites LF as CRLF; the file is opened truncated and
// UTF-8 with no BOM (nothing here writes a byte-order mark).
std::ofstream open_trace_file(const std::string& path);

} // namespace mh
