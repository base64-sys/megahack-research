// Item 3 — canonical JSON writer.
//
// Contract: notes/runtime-validation.md §Trace format defines run_fingerprint as SHA-256
// over "canonical JSON (UTF-8, sorted keys, no insignificant whitespace)". The writer has
// two jobs and they are different: `to_json()` keeps insertion order so a document can
// round-trip, `to_canonical()` sorts keys so a hash is stable. Never hash the first.

#include <cstdint>
#include <stdexcept>
#include <string>

#include "mh/json.hpp"
#include "test_runner.hpp"

using mh::json::Json;

MH_TEST(json_canonical_sorts_object_keys) {
  Json object = Json::object();
  object.set("b", Json::integer(1));
  object.set("a", Json::integer(2));

  // Byte-order sort of the UTF-8 keys.
  MH_CHECK_EQ(object.to_canonical(), std::string{R"({"a":2,"b":1})"});
  // Document order is the order the caller inserted, so a parsed document can be written
  // back unchanged.
  MH_CHECK_EQ(object.to_json(), std::string{R"({"b":1,"a":2})"});
}

MH_TEST(json_canonical_has_no_insignificant_whitespace) {
  Json inner = Json::object();
  inner.set("z", Json::integer(1));
  inner.set("a", Json::boolean(true));

  Json array = Json::array();
  array.push(Json::string("x"));
  array.push(inner);

  Json root = Json::object();
  root.set("list", array);
  root.set("n", Json::null());

  MH_CHECK_EQ(root.to_canonical(),
              std::string{R"({"list":["x",{"a":true,"z":1}],"n":null})"});
}

MH_TEST(json_strings_escape_and_utf8_passes_through) {
  MH_CHECK_EQ(Json::string("a\"b\\c\nd\te").to_canonical(), std::string{R"("a\"b\\c\nd\te")"});
  MH_CHECK_EQ(Json::string(std::string("a\x01"
                                       "b"))
                  .to_canonical(),
              std::string{R"("a\u0001b")"});
  MH_CHECK_EQ(Json::string("caf\xC3\xA9").to_canonical(), std::string{R"("café")"});
}

MH_TEST(json_numbers_are_shortest_round_trip) {
  MH_CHECK_EQ(Json::integer(0).to_canonical(), std::string{"0"});
  MH_CHECK_EQ(Json::integer(-1).to_canonical(), std::string{"-1"});
  MH_CHECK_EQ(Json::integer(8697056).to_canonical(), std::string{"8697056"});
  MH_CHECK_EQ(Json::real(0.5).to_canonical(), std::string{"0.5"});
  MH_CHECK_EQ(Json::real(1.0).to_canonical(), std::string{"1"});
  MH_CHECK_EQ(Json::real(-0.25).to_canonical(), std::string{"-0.25"});
}

MH_TEST(json_empty_containers) {
  MH_CHECK_EQ(Json::object().to_canonical(), std::string{"{}"});
  MH_CHECK_EQ(Json::array().to_canonical(), std::string{"[]"});
  MH_CHECK_EQ(Json::null().to_canonical(), std::string{"null"});
}

MH_TEST(json_object_rejects_duplicate_keys) {
  Json object = Json::object();
  object.set("k", Json::integer(1));

  bool threw = false;
  try {
    object.set("k", Json::integer(2));
  } catch (const std::invalid_argument&) {
    threw = true;
  }
  MH_CHECK(threw);
}

MH_TEST(json_real_rejects_values_json_cannot_represent) {
  bool threw_nan = false;
  try {
    Json::real(0.0 / 0.0);
  } catch (const std::invalid_argument&) {
    threw_nan = true;
  }
  MH_CHECK(threw_nan);

  bool threw_inf = false;
  try {
    Json::real(1.0 / 0.0);
  } catch (const std::invalid_argument&) {
    threw_inf = true;
  }
  MH_CHECK(threw_inf);
}

MH_TEST(json_deep_nesting_keeps_order_inside_arrays) {
  Json first = Json::object();
  first.set("id", Json::string("b"));
  Json second = Json::object();
  second.set("id", Json::string("a"));

  Json array = Json::array();
  array.push(first);
  array.push(second);

  MH_CHECK_EQ(array.to_canonical(), std::string{R"([{"id":"b"},{"id":"a"}])"});
}
