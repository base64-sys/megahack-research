// Record builders: the shapes the observer emits, pinned byte-for-byte.
//
// The authority is a captured line, not this file's opinion: `fixtures/records_captured.jsonl`
// holds two payloads extracted from run `8a59be9e` (seq 1155 and seq 1), with
// `call.this.value` normalised from `0x2335B9BB010` to `"0x0"` and nothing else changed.
// A refactor that changes a key name, a key order or a spelling breaks
// `records_captured_line_round_trips_through_the_builders`, which is the point: a drifted
// record shape would invalidate every trace without failing anything else.

#include <cstdint>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>

#include "mh/json.hpp"
#include "mh/json_parse.hpp"
#include "mh/records.hpp"
#include "test_runner.hpp"

namespace {

using mh::json::Json;

std::string fixture_line(int index) {
  std::ifstream input(std::string(MH_FIXTURES_DIR) + "/records_captured.jsonl");
  std::string line;
  for (int current = 0; current <= index && std::getline(input, line); ++current) {
  }
  return line;
}

// "0x397F40" -> 0x397F40
std::uint64_t parse_hex(std::string_view text) {
  std::uint64_t value = 0;
  for (char digit : text) {
    if (digit == 'x' || digit == 'X') {
      continue;
    }
    value <<= 4;
    if (digit >= '0' && digit <= '9') {
      value |= static_cast<std::uint64_t>(digit - '0');
    } else if (digit >= 'a' && digit <= 'f') {
      value |= static_cast<std::uint64_t>(digit - 'a' + 10);
    } else if (digit >= 'A' && digit <= 'F') {
      value |= static_cast<std::uint64_t>(digit - 'A' + 10);
    }
  }
  return value;
}

const Json* member(const Json& object, std::string_view key) {
  for (const auto& entry : object.members()) {
    if (entry.first == key) {
      return &entry.second;
    }
  }
  return nullptr;
}

std::string text(const Json& object, std::string_view key) {
  const Json* node = member(object, key);
  return node == nullptr ? std::string{} : std::string{node->string_value()};
}

constexpr const char* kHookObject =
    R"({"observer":"gd_method","key":"GeometryDash.exe:rva:0x397f40","module":"GeometryDash.exe",)"
    R"("method":"PlayerObject::pushButton","resolution":"rva","rva":"0x397F40",)"
    R"("va":"0x7FF6762E7F40","target":{"module":"absolllute.hackmega.dll","callback_va":null,)"
    R"("callback_rva":null,"registration_va":null,"tag":null,"tag_label":null,)"
    R"("order_raw":null,"order_signed":null,"priority":3032},"convention":"unknown"})";

constexpr const char* kCallObject =
    R"({"call_id":"c577","depth":1,"parent_call_id":null,)"
    R"("this":{"state":"observed","type":"ptr","value":"0x0"},)"
    R"("args":{"state":"unavailable","reason":"arguments are not captured by this hook"},)"
    R"("return":{"state":"null"}})";

mh::records::HookIdentity captured_identity() {
  return mh::records::HookIdentity{"GeometryDash.exe:rva:0x397f40", "PlayerObject::pushButton",
                                   "GeometryDash.exe", 0x397F40, 0x7FF6762E7F40ULL};
}

}  // namespace

MH_TEST(records_hook_object_matches_the_observer_bytes) {
  const Json hook = mh::records::hook_object(
      captured_identity(), mh::records::hook_target("absolllute.hackmega.dll", 3032));
  MH_CHECK_EQ(hook.to_json(), kHookObject);
}

MH_TEST(records_call_object_nests_and_carries_duration) {
  const Json enter = mh::records::call_object("c577", 1, std::nullopt, 0);
  MH_CHECK_EQ(enter.to_json(), kCallObject);

  const Json exit = mh::records::call_object("c577", 2, "c576", 0x1234, 42);
  MH_CHECK(exit.to_json().find(R"("parent_call_id":"c576")") != std::string::npos);
  MH_CHECK(exit.to_json().find(R"("duration_ns":42)") != std::string::npos);
  MH_CHECK(exit.to_json().find(R"("value":"0x1234")") != std::string::npos);
}

MH_TEST(records_hook_event_wraps_phase_and_call) {
  const Json event = mh::records::hook_event(
      "enter", captured_identity(), mh::records::hook_target("absolllute.hackmega.dll", 3032),
      mh::records::call_object("c577", 1, std::nullopt, 0));
  const std::string expected = std::string(R"({"kind":"hook","phase":"enter","hook":)") +
                               kHookObject + R"(,"call":)" + kCallObject + "}";
  MH_CHECK_EQ(event.to_json(), expected);
}

MH_TEST(records_milestone_event_has_no_invented_data) {
  MH_CHECK_EQ(mh::records::milestone_event("level_started").to_json(),
              R"({"kind":"milestone","name":"level_started","data":{"state":"null"}})");
}

// A milestone may carry what was observed at the site. This is a builder test, not a statement
// about level identities: player-data strings are redacted at the call site, and the level-name
// shape is asserted separately below.
MH_TEST(records_milestone_event_can_carry_an_observed_value) {
  const Json started = mh::records::milestone_event(
      "level_started", mh::Value::observed_str("example observed value"));
  MH_CHECK_EQ(started.to_json(),
              R"({"kind":"milestone","name":"level_started",)"
              R"("data":{"state":"observed","type":"str","value":"example observed value"}})");
}

// The record `level_started` emits when a level exists. notes/runtime-validation.md requires a
// level name to be redacted, so the only thing this record may carry is the reason it is
// withheld - no name, no digest of a name, and no string type that could hold one.
MH_TEST(records_milestone_level_started_redacts_the_level_name) {
  const Json started = mh::records::milestone_event(
      "level_started", mh::Value::redacted("level name is player data"));
  MH_CHECK_EQ(started.to_json(),
              R"({"kind":"milestone","name":"level_started",)"
              R"("data":{"state":"redacted","reason":"level name is player data"}})");
  MH_CHECK(started.to_json().find(R"("type":"str")") == std::string::npos);
}

// The record `level_started` emits when PlayLayer::init is given no level at all. It must stay
// distinguishable from the redacted form: "there was no level" is not "the level is withheld".
MH_TEST(records_milestone_level_started_states_a_missing_level) {
  const Json started = mh::records::milestone_event(
      "level_started", mh::Value::unavailable("no level was passed to PlayLayer::init"));
  MH_CHECK_EQ(started.to_json(),
              R"({"kind":"milestone","name":"level_started",)"
              R"("data":{"state":"unavailable","reason":"no level was passed to PlayLayer::init"}})");
  MH_CHECK(started.to_json().find(R"("state":"redacted")") == std::string::npos);
}

MH_TEST(records_save_event_states_absence_with_nulls) {
  const Json absent =
      mh::records::save_event("snapshot_before", false, std::nullopt, std::nullopt, "saves/x.dat");
  MH_CHECK_EQ(absent.to_json(),
              R"({"kind":"save","store":"gd_save","phase":"snapshot_before",)"
              R"("source":"filesystem","path_template":"<gd_save>/CCLocalLevels.dat",)"
              R"("exists":false,"size":null,"sha256":null,"keys":null,)"
              R"("local_path":"saves/x.dat"})");

  const Json present = mh::records::save_event("snapshot_after", true, 1234, std::string("beef"),
                                               "saves/x.dat");
  MH_CHECK(present.to_json().find(R"("exists":true)") != std::string::npos);
  MH_CHECK(present.to_json().find(R"("size":1234)") != std::string::npos);
  MH_CHECK(present.to_json().find(R"("sha256":"beef")") != std::string::npos);
}

MH_TEST(records_error_event_nulls_an_absent_hook_key) {
  MH_CHECK_EQ(
      mh::records::error_event("capture_overflow", "cap", true, std::nullopt).to_json(),
      R"({"kind":"error","code":"capture_overflow","message":"cap","fatal":true,)"
      R"("hook_key":null})");
  MH_CHECK(mh::records::error_event("hook_install_failed", "x", true,
                                    std::string_view("GeometryDash.exe:rva:0x397f40"))
               .to_json()
               .find(R"("hook_key":"GeometryDash.exe:rva:0x397f40")") != std::string::npos);
}

// The proof the refactor did not change a byte: each captured payload is rebuilt from its own
// parsed fields and must come back identical.
MH_TEST(records_captured_line_round_trips_through_the_builders) {
  for (int index = 0; index < 2; ++index) {
    const std::string line = fixture_line(index);
    MH_CHECK(!line.empty());
    const mh::json::ParseResult parsed = mh::json::parse(line);
    MH_CHECK(parsed.value.has_value());
    if (!parsed.value) {
      return;
    }
    const Json& record = *parsed.value;

    if (text(record, "kind") == "milestone") {
      MH_CHECK_EQ(mh::records::milestone_event(text(record, "name")).to_json(), line);
      continue;
    }

    const Json* hook = member(record, "hook");
    const Json* call = member(record, "call");
    MH_CHECK(hook != nullptr && call != nullptr);
    if (hook == nullptr || call == nullptr) {
      continue;
    }

    mh::records::HookIdentity identity;
    identity.key = text(*hook, "key");
    identity.method = text(*hook, "method");
    identity.module = text(*hook, "module");
    identity.rva = static_cast<std::uint32_t>(parse_hex(text(*hook, "rva")));
    identity.va = static_cast<std::uintptr_t>(parse_hex(text(*hook, "va")));

    Json target = Json::null();
    if (const Json* captured_target = member(*hook, "target");
        captured_target != nullptr && captured_target->is_object()) {
      const Json* priority = member(*captured_target, "priority");
      target = mh::records::hook_target(
          text(*captured_target, "module"),
          priority == nullptr ? 0 : priority->integer_value());
    }

    const Json* parent = member(*call, "parent_call_id");
    std::optional<std::string> parent_id;
    if (parent != nullptr && parent->kind() == Json::Kind::string) {
      parent_id = std::string{parent->string_value()};
    }
    const Json* receiver = member(*call, "this");
    const Json* self = receiver == nullptr ? nullptr : member(*receiver, "value");
    const Json* duration = member(*call, "duration_ns");
    std::optional<std::int64_t> duration_ns;
    if (duration != nullptr) {
      duration_ns = duration->integer_value();
    }
    const Json* depth = member(*call, "depth");

    const Json rebuilt_call = mh::records::call_object(
        text(*call, "call_id"), depth == nullptr ? 1 : depth->integer_value(),
        parent_id.has_value() ? std::optional<std::string_view>(*parent_id) : std::nullopt,
        self == nullptr ? 0 : parse_hex(self->string_value()), duration_ns);

    MH_CHECK_EQ(mh::records::hook_event(text(record, "phase"), identity, target, rebuilt_call)
                    .to_json(),
                line);
  }
}
