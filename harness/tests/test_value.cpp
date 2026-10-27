// Item 2 — the value model.
//
// Contract: notes/runtime-validation.md §Value objects. Every captured value is a
// tagged object so that "not captured" cannot be confused with a real null; floats carry
// `bits` and the bits are authoritative; pointers are recorded but excluded from
// equality by default (§Comparison rules 5, table "Default comparison").

#include <cstdint>
#include <string>
#include <string_view>

#include "mh/value.hpp"
#include "test_runner.hpp"

namespace {

using mh::Equality;
using mh::Value;

std::string json(const Value& value) { return value.to_json(); }

} // namespace

// --- Serialisation: the shapes are the contract's, character for character. ----------

MH_TEST(value_u64_serialises_as_hex_string) {
  MH_CHECK_EQ(json(Value::observed_u64(0x180199E80ull)),
              std::string{R"({"state":"observed","type":"u64","value":"0x180199E80"})"});
  MH_CHECK_EQ(json(Value::observed_u64(0)), std::string{R"({"state":"observed","type":"u64","value":"0x0"})"});
}

MH_TEST(value_i32_serialises_as_json_number) {
  MH_CHECK_EQ(json(Value::observed_i32(-1)),
              std::string{R"({"state":"observed","type":"i32","value":-1})"});
  MH_CHECK_EQ(json(Value::observed_i32(2147483647)),
              std::string{R"({"state":"observed","type":"i32","value":2147483647})"});
}

MH_TEST(value_f32_serialises_value_and_bits) {
  // notes/runtime-validation.md §Value objects: bits is authoritative, value is
  // convenience.
  MH_CHECK_EQ(json(Value::observed_f32_bits(0x3F000000u)),
              std::string{R"({"state":"observed","type":"f32","value":0.5,"bits":"0x3F000000"})"});
}

MH_TEST(value_bool_str_bytes_ptr_serialise_like_the_contract) {
  MH_CHECK_EQ(json(Value::observed_bool(true)),
              std::string{R"({"state":"observed","type":"bool","value":true})"});
  MH_CHECK_EQ(json(Value::observed_str("hello")),
              std::string{R"({"state":"observed","type":"str","value":"hello"})"});
  MH_CHECK_EQ(json(Value::observed_bytes("AAECAw==")),
              std::string{R"({"state":"observed","type":"bytes","value":"AAECAw==","encoding":"base64"})"});
  MH_CHECK_EQ(json(Value::observed_ptr(0x1F4A2B00000ull)),
              std::string{R"({"state":"observed","type":"ptr","value":"0x1F4A2B00000"})"});
}

MH_TEST(value_null_unavailable_redacted_serialise_like_the_contract) {
  MH_CHECK_EQ(json(Value::null_value()), std::string{R"({"state":"null"})"});
  MH_CHECK_EQ(json(Value::unavailable("not captured")),
              std::string{R"({"state":"unavailable","reason":"not captured"})"});
  MH_CHECK_EQ(json(Value::redacted("player name")),
              std::string{R"({"state":"redacted","reason":"player name"})"});
}

MH_TEST(value_strings_escape_json_and_keep_utf8) {
  MH_CHECK_EQ(json(Value::observed_str("a\"b\\c\nd\te")),
              std::string{R"({"state":"observed","type":"str","value":"a\"b\\c\nd\te"})"});
  // Control characters that have no short escape become \u00XX.
  MH_CHECK_EQ(json(Value::observed_str(std::string("a\x01"
                                                  "b"))),
              std::string{R"({"state":"observed","type":"str","value":"a\u0001b"})"});
  // Valid UTF-8 is passed through, not escaped.
  MH_CHECK_EQ(json(Value::observed_str("caf\xC3\xA9")),
              std::string{R"({"state":"observed","type":"str","value":"café"})"});
}

MH_TEST(value_hex_digits_are_upper_case_and_minimal) {
  // Upper case matches the contract's own value-object examples (0x3F000000, 0x1F4A).
  // Equality is numeric, so the case is presentation only.
  MH_CHECK_EQ(json(Value::observed_u64(0xabcull)),
              std::string{R"({"state":"observed","type":"u64","value":"0xABC"})"});
  MH_CHECK_EQ(json(Value::observed_f32_bits(0x3f800000u)),
              std::string{R"({"state":"observed","type":"f32","value":1,"bits":"0x3F800000"})"});
}

// --- Equality (§Comparison rules 5 and 6). ------------------------------------------

MH_TEST(value_observed_equality_is_typed_and_exact) {
  MH_CHECK(mh::compare(Value::observed_u64(7), Value::observed_u64(7)) == Equality::equal);
  MH_CHECK(mh::compare(Value::observed_u64(7), Value::observed_u64(8)) == Equality::different);
  MH_CHECK(mh::compare(Value::observed_i32(-1), Value::observed_i32(-1)) == Equality::equal);
  MH_CHECK(mh::compare(Value::observed_bool(true), Value::observed_bool(true)) == Equality::equal);
  MH_CHECK(mh::compare(Value::observed_bool(true), Value::observed_bool(false)) == Equality::different);
  MH_CHECK(mh::compare(Value::observed_str("a"), Value::observed_str("a")) == Equality::equal);
  MH_CHECK(mh::compare(Value::observed_str("a"), Value::observed_str("b")) == Equality::different);
  MH_CHECK(mh::compare(Value::observed_bytes("AA=="), Value::observed_bytes("AA==")) == Equality::equal);

  // Same numeric content, different declared type: not equal. The trace declares a type
  // per value, and a u64 and an i32 are different observations.
  MH_CHECK(mh::compare(Value::observed_u64(0), Value::observed_i32(0)) == Equality::different);
  MH_CHECK(mh::compare(Value::observed_u64(0x3F000000ull), Value::observed_f32_bits(0x3F000000u)) ==
           Equality::different);
}

MH_TEST(value_float_equality_uses_bits_not_value) {
  MH_CHECK(mh::compare(Value::observed_f32_bits(0x3F000000u), Value::observed_f32_bits(0x3F000000u)) ==
           Equality::equal);
  // +0.0 and -0.0 compare equal as floats but are different bit patterns, and the
  // contract says exact bits.
  MH_CHECK(mh::compare(Value::observed_f32_bits(0x00000000u), Value::observed_f32_bits(0x80000000u)) ==
           Equality::different);
  // Two NaN payloads with identical bits are the same observation; bits are the identity.
  MH_CHECK(mh::compare(Value::observed_f32_bits(0x7FC00000u), Value::observed_f32_bits(0x7FC00000u)) ==
           Equality::equal);
  MH_CHECK(mh::compare(Value::observed_f32_bits(0x7FC00000u), Value::observed_f32_bits(0x7FC00001u)) ==
           Equality::different);
}

MH_TEST(value_null_equals_only_null) {
  MH_CHECK(mh::compare(Value::null_value(), Value::null_value()) == Equality::equal);
  MH_CHECK(mh::compare(Value::null_value(), Value::observed_bool(false)) == Equality::different);
  MH_CHECK(mh::compare(Value::null_value(), Value::unavailable("x")) == Equality::different);
  MH_CHECK(mh::compare(Value::observed_u64(0), Value::null_value()) == Equality::different);
}

MH_TEST(value_unavailable_and_redacted_are_never_equal) {
  const Value unavailable = Value::unavailable("not captured");
  const Value unavailable_same = Value::unavailable("not captured");
  const Value redacted = Value::redacted("player name");

  MH_CHECK(mh::compare(unavailable, unavailable_same) == Equality::different);
  MH_CHECK(mh::compare(redacted, Value::redacted("player name")) == Equality::different);
  MH_CHECK(mh::compare(unavailable, redacted) == Equality::different);
  MH_CHECK(mh::compare(redacted, Value::null_value()) == Equality::different);
  MH_CHECK(mh::compare(redacted, Value::observed_str("x")) == Equality::different);
}

MH_TEST(value_pointer_equality_is_excluded_by_default) {
  MH_CHECK(mh::compare(Value::observed_ptr(0x1000), Value::observed_ptr(0x1000)) == Equality::excluded);
  MH_CHECK(mh::compare(Value::observed_ptr(0x1000), Value::observed_ptr(0x2000)) == Equality::excluded);
  // A pointer is not a u64, even with the same numeric value.
  MH_CHECK(mh::compare(Value::observed_ptr(0x1000), Value::observed_u64(0x1000)) == Equality::different);
}

// --- Accessors: callers must not have to re-derive the tag. ------------------------

MH_TEST(value_exposes_state_and_type) {
  const Value value = Value::observed_f32_bits(0x3F800000u);
  MH_CHECK(value.state() == mh::ValueState::observed);
  MH_CHECK(value.type() == mh::ValueType::f32);
  MH_CHECK(value.is_observed());
  MH_CHECK_EQ(value.f32_bits(), 0x3F800000u);

  const Value missing = Value::unavailable("not captured");
  MH_CHECK(missing.state() == mh::ValueState::unavailable);
  MH_CHECK(missing.type() == mh::ValueType::none);
  MH_CHECK_EQ(missing.reason(), std::string_view{"not captured"});

  MH_CHECK(Value::null_value().state() == mh::ValueState::null_value);
  MH_CHECK(Value::redacted("x").state() == mh::ValueState::redacted);

  // Typed accessors return exactly what was stored, with no conversion.
  MH_CHECK_EQ(Value::observed_u64(0x1234ull).as_u64(), 0x1234ull);
  MH_CHECK_EQ(Value::observed_i32(-7).as_i32(), std::int32_t{-7});
  MH_CHECK_EQ(Value::observed_bool(false).as_bool(), false);
  MH_CHECK_EQ(Value::observed_str("abc").as_str(), std::string_view{"abc"});
  MH_CHECK_EQ(Value::observed_bytes("AA==").as_bytes_base64(), std::string_view{"AA=="});
  MH_CHECK_EQ(Value::observed_ptr(0x2000).as_ptr(), 0x2000ull);
  MH_CHECK_EQ(Value::observed_f32_bits(0x3F000000u).f32_value(), 0.5f);
}
