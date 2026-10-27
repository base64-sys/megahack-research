// Item 4 — trace writer.
//
// Contract: notes/runtime-validation.md §Trace format, §Common fields, §`end` record.
// What is being pinned here is the envelope (schema/type/run identity/seq), the ordering
// guarantee (meta first, end last), the counters that make a lost write visible, and the
// JSONL spelling (LF, no BOM, one object per line).

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "mh/trace_writer.hpp"
#include "test_runner.hpp"

namespace {

const std::string kRunId = "00000000-0000-4000-8000-000000000001";
const std::string kZeros(64, '0');

mh::TraceHeader header() { return mh::TraceHeader{kRunId, kZeros}; }

std::string envelope(std::string_view type, std::int64_t seq) {
  return R"({"schema":"mh-trace/1","type":")" + std::string(type) + R"(","run_id":")" + kRunId +
         R"(","run_fingerprint":")" + kZeros + R"(","seq":)" + std::to_string(seq) + ",";
}

std::vector<std::string> split_lines(const std::string& text) {
  std::vector<std::string> lines;
  std::string current;
  for (const char c : text) {
    if (c == '\n') {
      lines.push_back(current);
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

mh::json::Json body_with(std::string_view key, std::string_view value) {
  mh::json::Json body = mh::json::Json::object();
  body.set(std::string{key}, mh::json::Json::string(value));
  return body;
}

mh::EventContext context(std::int64_t t_ns) {
  mh::EventContext ctx;
  ctx.t_ns = t_ns;
  ctx.frame = 0;
  ctx.thread.id = "main";
  ctx.thread.role = mh::ThreadRole::main;
  return ctx;
}

} // namespace

MH_TEST(trace_writer_meta_is_seq_zero_and_first_line) {
  std::ostringstream out;
  mh::TraceWriter writer(out, header());

  mh::json::Json body = mh::json::Json::object();
  body.set("created_utc", mh::json::Json::string("2026-10-27T12:00:00Z"));

  MH_CHECK(writer.write_meta(body, mh::Capture{}));
  const std::vector<std::string> lines = split_lines(out.str());
  MH_CHECK_EQ(lines.size(), std::size_t{1});
  MH_CHECK_EQ(lines[0], envelope("meta", 0) +
                            R"("t_ns":0,"created_utc":"2026-10-27T12:00:00Z","capture":{"status":"complete"}})");
  MH_CHECK_EQ(writer.next_seq(), std::int64_t{1});
}

MH_TEST(trace_writer_event_envelope_is_complete) {
  std::ostringstream out;
  mh::TraceWriter writer(out, header());
  writer.write_meta(mh::json::Json::object(), mh::Capture{});

  MH_CHECK(writer.write_event(body_with("kind", "milestone"), context(100)));
  const std::vector<std::string> lines = split_lines(out.str());
  MH_CHECK_EQ(lines.size(), std::size_t{2});
  MH_CHECK_EQ(lines[1], envelope("event", 1) +
                            R"("t_ns":100,"frame":0,"thread":{"id":"main","role":"main"},)" +
                            R"("kind":"milestone","capture":{"status":"complete"}})");
}

MH_TEST(trace_writer_seq_increments_per_attempted_event) {
  std::ostringstream out;
  mh::TraceWriter writer(out, header());
  writer.write_meta(mh::json::Json::object(), mh::Capture{});

  writer.write_event(body_with("kind", "milestone"), context(100));
  writer.write_event(body_with("kind", "milestone"), context(200));

  mh::EndInfo end;
  end.t_ns = 300;
  writer.write_end(end);

  const std::vector<std::string> lines = split_lines(out.str());
  MH_CHECK_EQ(lines.size(), std::size_t{4});
  MH_CHECK(lines[1].find(R"("seq":1,)") != std::string::npos);
  MH_CHECK(lines[2].find(R"("seq":2,)") != std::string::npos);
  MH_CHECK(lines[3].find(R"("seq":3,)") != std::string::npos);
}

MH_TEST(trace_writer_counts_written_and_dropped_events) {
  std::ostringstream out;
  mh::TraceWriter writer(out, header());
  writer.write_meta(mh::json::Json::object(), mh::Capture{});

  MH_CHECK(writer.write_event(body_with("kind", "milestone"), context(100)));
  MH_CHECK_EQ(writer.counts().attempted, std::int64_t{1});
  MH_CHECK_EQ(writer.counts().events, std::int64_t{1});
  MH_CHECK_EQ(writer.counts().dropped, std::int64_t{0});

  // A sink that refuses the write: the event is attempted, dropped, and the counter says
  // so, which is the only thing that distinguishes loss from a short trace.
  std::ostringstream failing;
  failing.setstate(std::ios::failbit);
  mh::TraceWriter lossy(failing, header());
  lossy.write_meta(mh::json::Json::object(), mh::Capture{});
  MH_CHECK(!lossy.write_event(body_with("kind", "milestone"), context(100)));
  MH_CHECK_EQ(lossy.counts().attempted, std::int64_t{1});
  MH_CHECK_EQ(lossy.counts().dropped, std::int64_t{1});
  MH_CHECK_EQ(lossy.counts().events, std::int64_t{0});
}

MH_TEST(trace_writer_derives_hook_and_error_counters) {
  std::ostringstream out;
  mh::TraceWriter writer(out, header());
  writer.write_meta(mh::json::Json::object(), mh::Capture{});

  mh::json::Json enter = body_with("kind", "hook");
  enter.set("phase", mh::json::Json::string("enter"));
  writer.write_event(enter, context(100));

  mh::json::Json exit = body_with("kind", "hook");
  exit.set("phase", mh::json::Json::string("exit"));
  writer.write_event(exit, context(200));

  writer.write_event(body_with("kind", "error"), context(300));
  writer.write_event(body_with("kind", "save"), context(400));

  MH_CHECK_EQ(writer.counts().hook_enter, std::int64_t{1});
  MH_CHECK_EQ(writer.counts().hook_exit, std::int64_t{1});
  MH_CHECK_EQ(writer.counts().errors, std::int64_t{1});
  MH_CHECK_EQ(writer.counts().events, std::int64_t{4});
}

MH_TEST(trace_writer_end_carries_counts_and_null_artifacts) {
  std::ostringstream out;
  mh::TraceWriter writer(out, header());
  writer.write_meta(mh::json::Json::object(), mh::Capture{});
  writer.write_event(body_with("kind", "milestone"), context(100));

  mh::EndInfo end;
  end.t_ns = 250;
  end.frames = 3;
  end.duration_ns = 250;
  end.unclosed = 2;
  end.capture.status = mh::CaptureStatus::partial;
  end.capture.missing = {"artifacts.stdout_sha256"};
  end.capture.reasons = {{"artifacts.stdout_sha256", "not collected"}};
  MH_CHECK(writer.write_end(end));

  const std::vector<std::string> lines = split_lines(out.str());
  MH_CHECK_EQ(lines.size(), std::size_t{3});
  MH_CHECK_EQ(lines[2],
              envelope("end", 2) +
                  R"("t_ns":250,"status":"completed","exit_code":0,"frames":3,"duration_ns":250,)" +
                  R"("counts":{"events":1,"attempted":1,"dropped":0,"unclosed":2,)" +
                  R"("hook_enter":0,"hook_exit":0,"errors":0},)" +
                  R"("artifacts":{"stdout_sha256":null,"stderr_sha256":null,"minidump_sha256":null},)" +
                  R"("capture":{"status":"partial","missing":["artifacts.stdout_sha256"],)" +
                  R"("reasons":{"artifacts.stdout_sha256":"not collected"}}})");
  MH_CHECK(writer.end_written());
}

MH_TEST(trace_writer_capture_statuses_serialise_without_dangling_lists) {
  MH_CHECK_EQ(mh::Capture{}.to_json().to_json(), std::string{R"({"status":"complete"})"});

  mh::Capture partial;
  partial.status = mh::CaptureStatus::partial;
  partial.missing = {"environment.gd.sha256"};
  partial.reasons = {{"environment.gd.sha256", "not hashed"}};
  MH_CHECK_EQ(partial.to_json().to_json(),
              std::string{R"J({"status":"partial","missing":["environment.gd.sha256"],)J"
                          R"J("reasons":{"environment.gd.sha256":"not hashed"}})J"});

  mh::Capture no_observation;
  no_observation.status = mh::CaptureStatus::unavailable;
  no_observation.missing = {"size", "sha256", "keys"};
  MH_CHECK_EQ(no_observation.to_json().to_json(),
              std::string{R"({"status":"unavailable","missing":["size","sha256","keys"],"reasons":{}})"});
}

MH_TEST(trace_writer_refuses_payload_envelope_keys) {
  std::ostringstream out;
  mh::TraceWriter writer(out, header());
  writer.write_meta(mh::json::Json::object(), mh::Capture{});

  bool threw = false;
  try {
    writer.write_event(body_with("seq", "0"), context(0));
  } catch (const std::invalid_argument&) {
    threw = true;
  }
  MH_CHECK(threw);

  bool threw_kind = false;
  try {
    writer.write_event(mh::json::Json::object(), context(0));
  } catch (const std::invalid_argument&) {
    threw_kind = true;
  }
  MH_CHECK(threw_kind);
}

MH_TEST(trace_writer_requires_meta_first_and_closes_after_end) {
  std::ostringstream out;
  mh::TraceWriter writer(out, header());

  bool threw = false;
  try {
    writer.write_event(body_with("kind", "milestone"), context(0));
  } catch (const std::logic_error&) {
    threw = true;
  }
  MH_CHECK(threw);

  writer.write_meta(mh::json::Json::object(), mh::Capture{});
  writer.write_end(mh::EndInfo{});

  bool threw_after_end = false;
  try {
    writer.write_event(body_with("kind", "milestone"), context(0));
  } catch (const std::logic_error&) {
    threw_after_end = true;
  }
  MH_CHECK(threw_after_end);
}

MH_TEST(trace_file_is_lf_terminated_utf8_without_bom) {
  const std::filesystem::path path = "mh_trace_writer_test.jsonl";
  {
    std::ofstream file = mh::open_trace_file(path.string());
    mh::TraceWriter writer(file, header());
    writer.write_meta(mh::json::Json::object(), mh::Capture{});
    writer.write_event(body_with("kind", "milestone"), context(100));
    writer.write_end(mh::EndInfo{});
  }

  std::ifstream input(path, std::ios::binary);
  const std::string bytes((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
  input.close();
  std::filesystem::remove(path);

  MH_CHECK(!bytes.empty());
  // No BOM, no CR: a trace is UTF-8 JSONL with LF endings on every platform.
  MH_CHECK(bytes.size() < 3 || static_cast<unsigned char>(bytes[0]) != 0xEF);
  MH_CHECK_EQ(bytes.find('\r'), std::string::npos);
  MH_CHECK_EQ(bytes.back(), '\n');
  MH_CHECK_EQ(split_lines(bytes).size(), std::size_t{3});
}
