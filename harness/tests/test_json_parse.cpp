// Item 6a — strict JSON parser.
//
// Contract: notes/runtime-validation.md §Trace format ("one complete JSON object per
// line, no comments, no BOM"). The parser is what makes "a trace with no `end` record is
// incomplete" checkable at all, and what lets the example trace in §Complete illustrative
// trace be validated rather than eyeballed.

#include <cstddef>
#include <string>

#include "mh/json.hpp"
#include "mh/json_parse.hpp"
#include "test_runner.hpp"

namespace {

mh::json::ParseResult parse(const std::string& text) { return mh::json::parse(text); }

std::string round_trip(const std::string& text) {
  const mh::json::ParseResult result = parse(text);
  if (!result.ok()) {
    return "<parse error>";
  }
  return result.value->to_json();
}

} // namespace

MH_TEST(json_parse_accepts_the_contract_value_objects) {
  // notes/runtime-validation.md §Value objects, verbatim.
  const char* const documents[] = {
      R"({"state":"observed","type":"u64","value":"0x180199E80"})",
      R"({"state":"observed","type":"i32","value":-1})",
      R"({"state":"observed","type":"f32","value":0.5,"bits":"0x3F000000"})",
      R"({"state":"observed","type":"bool","value":true})",
      R"({"state":"observed","type":"str","value":"..."})",
      R"({"state":"observed","type":"bytes","value":"<base64>","encoding":"base64"})",
      R"({"state":"observed","type":"ptr","value":"0x..."})",
      R"({"state":"null"})",
      R"({"state":"unavailable","reason":"..."})",
      R"({"state":"redacted","reason":"..."})",
  };
  for (const char* const document : documents) {
    const mh::json::ParseResult result = parse(document);
    MH_CHECK(result.ok());
    if (result.ok()) {
      // Insertion order is preserved, so the document survives a write unchanged.
      MH_CHECK_EQ(result.value->to_json(), std::string{document});
    }
  }
}

MH_TEST(json_parse_rejects_malformed_documents) {
  const char* const documents[] = {
      "",            // empty
      "{",           // unterminated object
      "[1,]",        // trailing comma
      R"({"a":1,})",  // trailing comma in object
      "tru",         // truncated keyword
      "01",          // leading zero
      "+1",          // leading plus
      "NaN",         // not JSON
      "Infinity",    // not JSON
      R"({"a" 1})",  // missing colon
      R"("unterminated)",
      R"("\q")",     // invalid escape
      "1 2",         // trailing characters
      "\xEF\xBB\xBF{}",  // BOM: §Trace format says no BOM
  };
  for (const char* const document : documents) {
    const mh::json::ParseResult result = parse(document);
    MH_CHECK(!result.ok());
    MH_CHECK(result.error.has_value());
  }
}

MH_TEST(json_parse_preserves_key_order_for_round_trip) {
  const mh::json::ParseResult result = parse(R"({"b":1,"a":2})");
  MH_CHECK(result.ok());
  MH_CHECK_EQ(result.value->to_json(), std::string{R"({"b":1,"a":2})"});
  MH_CHECK_EQ(result.value->to_canonical(), std::string{R"({"a":2,"b":1})"});
}

MH_TEST(json_parse_decodes_escapes_and_surrogate_pairs) {
  MH_CHECK_EQ(round_trip(R"("a\"b\\c\nd\te")"), std::string{R"("a\"b\\c\nd\te")"});
  MH_CHECK_EQ(round_trip(R"("\u00e9")"), std::string{"\"\xC3\xA9\""});
  MH_CHECK_EQ(round_trip(R"("\ud83d\ude00")"), std::string{"\"\xF0\x9F\x98\x80\""});
  MH_CHECK_EQ(round_trip(R"("\u0041")"), std::string{R"("A")"});
}

MH_TEST(json_parse_distinguishes_integer_from_real) {
  const mh::json::ParseResult integer = parse("8697056");
  MH_CHECK(integer.ok());
  MH_CHECK(integer.value->kind() == mh::json::Json::Kind::integer);
  MH_CHECK_EQ(integer.value->to_json(), std::string{"8697056"});

  const mh::json::ParseResult negative = parse("-42");
  MH_CHECK(negative.ok());
  MH_CHECK(negative.value->kind() == mh::json::Json::Kind::integer);

  // A fraction or an exponent is a real, even when the value is integral. 1e3 prints as
  // 1000 because the writer emits the shortest round-trip decimal, so a document using
  // exponent notation is not byte-identical after a round trip but is semantically equal.
  const mh::json::ParseResult fraction = parse("1.5");
  MH_CHECK(fraction.ok());
  MH_CHECK(fraction.value->kind() == mh::json::Json::Kind::real);
  MH_CHECK_EQ(fraction.value->to_json(), std::string{"1.5"});

  const mh::json::ParseResult exponent = parse("1e3");
  MH_CHECK(exponent.ok());
  MH_CHECK(exponent.value->kind() == mh::json::Json::Kind::real);
  MH_CHECK_EQ(exponent.value->to_json(), std::string{"1000"});
}

MH_TEST(json_parse_rejects_duplicate_keys) {
  const mh::json::ParseResult result = parse(R"({"k":1,"k":2})");
  MH_CHECK(!result.ok());
  MH_CHECK(result.error.has_value());
}

MH_TEST(json_parse_rejects_nesting_beyond_the_limit) {
  std::string document;
  for (int i = 0; i < 200; ++i) {
    document += '[';
  }
  for (int i = 0; i < 200; ++i) {
    document += ']';
  }
  MH_CHECK(!parse(document).ok());
}

MH_TEST(json_parse_reports_the_offset_of_a_syntax_error) {
  const mh::json::ParseResult result = parse("[1,]");
  MH_CHECK(!result.ok());
  MH_CHECK(result.error.has_value());
  MH_CHECK_EQ(result.error->offset, std::size_t{3});
  MH_CHECK(!result.error->message.empty());
}

MH_TEST(json_parse_round_trips_a_nested_document) {
  const std::string document =
      R"({"schema":"mh-trace/1","type":"event","seq":1,"frame":null,)"
      R"("thread":{"id":"main","role":"main"},"call":{"args":{"state":"observed","items":[]}},)"
      R"("bits":"0x3F000000","ratio":0.5,"ok":true})";

  const mh::json::ParseResult result = parse(document);
  MH_CHECK(result.ok());
  MH_CHECK_EQ(result.value->to_json(), document);
}
