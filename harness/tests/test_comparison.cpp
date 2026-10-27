// Item 10 — ordering groups, value comparison with tolerances, result statuses.
//
// Contract: notes/runtime-validation.md §Comparison rules 4, 5, 6 and 9.

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "mh/comparison.hpp"
#include "mh/json_parse.hpp"
#include "test_runner.hpp"

namespace {

mh::json::Json value_json(const std::string& text) {
  mh::json::ParseResult parsed = mh::json::parse(text);
  if (!parsed.ok()) {
    return mh::json::Json::null();
  }
  return *parsed.value;
}

mh::Tolerance tolerance(double abs_tolerance, double rel_tolerance, std::string why) {
  mh::Tolerance result;
  result.abs_tolerance = abs_tolerance;
  result.rel_tolerance = rel_tolerance;
  result.why = std::move(why);
  return result;
}

mh::Finding finding(mh::ComparisonStatus severity, std::string detail = "") {
  return mh::Finding{severity, "test", std::move(detail)};
}

} // namespace

// --- value objects ------------------------------------------------------------------

MH_TEST(comparison_value_from_json_round_trips_the_contract_shapes) {
  const char* const documents[] = {
      R"({"state":"observed","type":"u64","value":"0x180199E80"})",
      R"({"state":"observed","type":"i32","value":-1})",
      R"({"state":"observed","type":"f32","value":0.5,"bits":"0x3F000000"})",
      R"({"state":"observed","type":"bool","value":true})",
      R"({"state":"observed","type":"str","value":"..."})",
      R"({"state":"observed","type":"bytes","value":"<base64>","encoding":"base64"})",
      R"({"state":"observed","type":"ptr","value":"0x1F4A2B00000"})",
      R"({"state":"null"})",
      R"({"state":"unavailable","reason":"..."})",
      R"({"state":"redacted","reason":"..."})",
  };
  for (const char* const document : documents) {
    const mh::json::Json node = value_json(document);
    const std::optional<mh::Value> value = mh::value_from_json(node);
    MH_CHECK(value.has_value());
    if (value.has_value()) {
      MH_CHECK_EQ(value->to_json(), std::string{document});
    }
  }
}

MH_TEST(comparison_value_rejects_a_malformed_object) {
  MH_CHECK(!mh::value_from_json(value_json(R"({"state":"observed","type":"u128"})"))
                .has_value());
  MH_CHECK(!mh::value_from_json(value_json(R"({"type":"u64","value":"0x1"})")).has_value());
  MH_CHECK(!mh::value_from_json(value_json(R"("0x1")")).has_value());
}

MH_TEST(comparison_values_are_exact_by_default) {
  const auto equal = mh::compare_value_json(
      value_json(R"({"state":"observed","type":"u64","value":"0x10"})"),
      value_json(R"({"state":"observed","type":"u64","value":"0x10"})"));
  MH_CHECK(equal.verdict == mh::ValueVerdict::equal);

  const auto different = mh::compare_value_json(
      value_json(R"({"state":"observed","type":"u64","value":"0x10"})"),
      value_json(R"({"state":"observed","type":"u64","value":"0x11"})"));
  MH_CHECK(different.verdict == mh::ValueVerdict::different);

  // Floats compare by bits: +0.0 and -0.0 are different observations.
  const auto zeroes = mh::compare_value_json(
      value_json(R"({"state":"observed","type":"f32","value":0,"bits":"0x0"})"),
      value_json(R"({"state":"observed","type":"f32","value":0,"bits":"0x80000000"})"));
  MH_CHECK(zeroes.verdict == mh::ValueVerdict::different);
}

MH_TEST(comparison_tolerance_applies_to_decoded_floats) {
  // 0.5 and the next float up differ in bits but are within 1e-6.
  const auto within = mh::compare_value_json(
      value_json(R"({"state":"observed","type":"f32","value":0.5,"bits":"0x3F000000"})"),
      value_json(R"({"state":"observed","type":"f32","value":0.5,"bits":"0x3F000001"})"),
      tolerance(0.0001, 0.000001, "physics float cross-build rounding"));
  MH_CHECK(within.verdict == mh::ValueVerdict::equal);

  const auto outside = mh::compare_value_json(
      value_json(R"({"state":"observed","type":"f32","value":0.5,"bits":"0x3F000000"})"),
      value_json(R"({"state":"observed","type":"f32","value":0.75,"bits":"0x3F400000"})"),
      tolerance(0.0001, 0.000001, "physics float cross-build rounding"));
  MH_CHECK(outside.verdict == mh::ValueVerdict::different);
}

MH_TEST(comparison_tolerance_without_a_why_is_ignored) {
  // "A tolerance without a `why` is invalid", so the exact-bits rule stands.
  const auto result = mh::compare_value_json(
      value_json(R"({"state":"observed","type":"f32","value":0.5,"bits":"0x3F000000"})"),
      value_json(R"({"state":"observed","type":"f32","value":0.5,"bits":"0x3F000001"})"),
      tolerance(0.0001, 0.000001, ""));
  MH_CHECK(result.verdict == mh::ValueVerdict::different);
}

MH_TEST(comparison_tolerance_is_not_applied_to_counts) {
  // "Do not apply a tolerance to a hash, a boolean, an enum, a count, or a frame index."
  const auto result = mh::compare_value_json(
      value_json(R"({"state":"observed","type":"i32","value":10})"),
      value_json(R"({"state":"observed","type":"i32","value":11})"),
      tolerance(100.0, 1.0, "would hide a real difference"));
  MH_CHECK(result.verdict == mh::ValueVerdict::different);
}

MH_TEST(comparison_unavailable_and_redacted_are_blocked) {
  const auto unavailable = mh::compare_value_json(
      value_json(R"({"state":"unavailable","reason":"file locked"})"),
      value_json(R"({"state":"unavailable","reason":"file locked"})"));
  MH_CHECK(unavailable.verdict == mh::ValueVerdict::blocked);

  const auto redacted = mh::compare_value_json(
      value_json(R"({"state":"redacted","reason":"player name"})"),
      value_json(R"({"state":"observed","type":"str","value":"x"})"));
  MH_CHECK(redacted.verdict == mh::ValueVerdict::blocked);
}

MH_TEST(comparison_pointers_are_excluded_not_different) {
  const auto result = mh::compare_value_json(
      value_json(R"({"state":"observed","type":"ptr","value":"0x1000"})"),
      value_json(R"({"state":"observed","type":"ptr","value":"0x2000"})"));
  MH_CHECK(result.verdict == mh::ValueVerdict::excluded);
}

MH_TEST(comparison_null_is_equal_only_to_null) {
  const auto both_null = mh::compare_value_json(value_json(R"({"state":"null"})"),
                                                value_json(R"({"state":"null"})"));
  MH_CHECK(both_null.verdict == mh::ValueVerdict::equal);

  const auto null_versus_value = mh::compare_value_json(
      value_json(R"({"state":"null"})"),
      value_json(R"({"state":"observed","type":"bool","value":false})"));
  MH_CHECK(null_versus_value.verdict == mh::ValueVerdict::different);
}

// --- ordered groups -----------------------------------------------------------------

MH_TEST(comparison_order_group_in_order_is_satisfied) {
  const std::vector<mh::OrderEvent> events = {
      {"level_started", "", "main"},
      {"attempt_started", "", "main"},
      {"", "GeometryDash.exe:rva:0x3b8eb0", "main"},
  };
  const std::vector<std::string> group = {"level_started", "attempt_started"};

  const mh::OrderingResult result = mh::check_order_group(events, group);
  MH_CHECK(result.satisfied);
  MH_CHECK(result.missing.empty());
}

MH_TEST(comparison_order_group_out_of_order_fails) {
  const std::vector<mh::OrderEvent> events = {
      {"attempt_started", "", "main"},
      {"level_started", "", "main"},
  };
  const mh::OrderingResult result =
      mh::check_order_group(events, {"level_started", "attempt_started"});

  // "A required group that is out of order is a failure."
  MH_CHECK(!result.satisfied);
  MH_CHECK_EQ(result.missing.size(), std::size_t{1});
  MH_CHECK_EQ(result.missing[0], std::string{"attempt_started"});
}

MH_TEST(comparison_order_group_matches_hook_enter_keys) {
  const std::vector<mh::OrderEvent> events = {
      {"", "GeometryDash.exe:rva:0x3b8eb0", "main"},
      {"", "GeometryDash.exe:rva:0x3b4ba0", "main"},
  };
  MH_CHECK(mh::check_order_group(events, {"GeometryDash.exe:rva:0x3b8eb0",
                                          "GeometryDash.exe:rva:0x3b4ba0"})
               .satisfied);
  MH_CHECK(!mh::check_order_group(events, {"GeometryDash.exe:rva:0x3b4ba0",
                                           "GeometryDash.exe:rva:0x3b8eb0"})
                .satisfied);
}

MH_TEST(comparison_order_group_ignores_events_not_proven_to_be_main_thread) {
  // "If the harness cannot prove a thread is `main`, its events are not eligible to satisfy
  // an ordered group."
  const std::vector<mh::OrderEvent> events = {
      {"level_started", "", "unknown"},
      {"attempt_started", "", "render"},
  };
  const mh::OrderingResult result =
      mh::check_order_group(events, {"level_started", "attempt_started"});

  MH_CHECK(!result.satisfied);
  MH_CHECK_EQ(result.missing.size(), std::size_t{2});
}

MH_TEST(comparison_order_group_requires_each_name_to_appear) {
  const std::vector<mh::OrderEvent> events = {{"level_started", "", "main"}};
  const mh::OrderingResult result =
      mh::check_order_group(events, {"level_started", "level_completed"});

  MH_CHECK(!result.satisfied);
  MH_CHECK_EQ(result.missing.size(), std::size_t{1});
  MH_CHECK_EQ(result.missing[0], std::string{"level_completed"});
}

MH_TEST(comparison_order_group_of_nothing_is_satisfied) {
  MH_CHECK(mh::check_order_group({}, {}).satisfied);
}

// --- result statuses ----------------------------------------------------------------

MH_TEST(comparison_status_pass_needs_a_passing_gate_and_no_findings) {
  MH_CHECK(mh::finalize_status(mh::GateStatus::pass, {}) == mh::ComparisonStatus::pass);
}

MH_TEST(comparison_status_warnings_only_is_pass_with_warnings) {
  const std::vector<mh::Finding> findings = {
      finding(mh::ComparisonStatus::pass_with_warnings, "optional hook missing")};
  MH_CHECK(mh::finalize_status(mh::GateStatus::pass, findings) ==
           mh::ComparisonStatus::pass_with_warnings);
}

MH_TEST(comparison_status_a_failure_beats_a_warning) {
  const std::vector<mh::Finding> findings = {
      finding(mh::ComparisonStatus::pass_with_warnings),
      finding(mh::ComparisonStatus::fail, "value differs")};
  MH_CHECK(mh::finalize_status(mh::GateStatus::pass, findings) == mh::ComparisonStatus::fail);
}

MH_TEST(comparison_status_invalid_gate_dominates) {
  // §9: "invalid | Comparability gate failed (hash, version, scenario, observer, level,
  // settings)". A later finding cannot promote it.
  const std::vector<mh::Finding> findings = {
      finding(mh::ComparisonStatus::pass_with_warnings)};
  MH_CHECK(mh::finalize_status(mh::GateStatus::invalid, findings) ==
           mh::ComparisonStatus::invalid);
}

MH_TEST(comparison_status_blocked_gate_or_finding_is_blocked) {
  MH_CHECK(mh::finalize_status(mh::GateStatus::blocked, {}) == mh::ComparisonStatus::blocked);

  const std::vector<mh::Finding> findings = {finding(mh::ComparisonStatus::blocked, "unavailable")};
  MH_CHECK(mh::finalize_status(mh::GateStatus::pass, findings) ==
           mh::ComparisonStatus::blocked);
}

MH_TEST(comparison_status_unstable_variant_stops_the_claim) {
  // §8: "If a required event, payload, or order differs within a variant, the scenario is
  // `unstable`; do not proceed to a cross-variant claim."
  MH_CHECK(mh::finalize_status(mh::GateStatus::pass, {}, /*unstable=*/true) ==
           mh::ComparisonStatus::unstable);
}
