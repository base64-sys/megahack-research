// Value model: serialisation and equality.
//
// Source: notes/runtime-validation.md §Value objects (object shapes, bits-authoritative
// floats) and §Comparison rules 5 ("Default comparison" table).

#include "mh/value.hpp"

#include <bit>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>

#include "mh/text.hpp"

namespace mh {
namespace {

// Shortest decimal that round-trips to the same float. Non-finite values have no JSON
// number literal; `bits` stays authoritative, so `value` is emitted as null for them so
// the line remains parseable. UNVERIFIED: the contract does not spell NaN/Infinity, so
// a reader must use `bits` for those (harness/PROGRESS.md, open questions).
void append_float_value(std::string& out, std::uint32_t bits) {
  const float value = std::bit_cast<float>(bits);
  if (!std::isfinite(value)) {
    out += "null";
    return;
  }
  char buffer[32];
  const auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
  out.append(buffer, result.ptr);
}

} // namespace

Value::Value(ValueState state, ValueType type, std::uint64_t number, std::string text)
    : state_(state), type_(type), number_(number), text_(std::move(text)) {}

Value Value::observed_u64(std::uint64_t value) {
  return Value(ValueState::observed, ValueType::u64, value, {});
}

Value Value::observed_i32(std::int32_t value) {
  return Value(ValueState::observed, ValueType::i32, static_cast<std::uint32_t>(value), {});
}

Value Value::observed_f32_bits(std::uint32_t bits) {
  return Value(ValueState::observed, ValueType::f32, bits, {});
}

Value Value::observed_bool(bool value) {
  return Value(ValueState::observed, ValueType::boolean, value ? 1u : 0u, {});
}

Value Value::observed_str(std::string_view value) {
  return Value(ValueState::observed, ValueType::str, 0, std::string{value});
}

Value Value::observed_bytes(std::string_view base64) {
  return Value(ValueState::observed, ValueType::bytes, 0, std::string{base64});
}

Value Value::observed_ptr(std::uint64_t address) {
  return Value(ValueState::observed, ValueType::ptr, address, {});
}

Value Value::null_value() {
  return Value(ValueState::null_value, ValueType::none, 0, {});
}

Value Value::unavailable(std::string reason) {
  return Value(ValueState::unavailable, ValueType::none, 0, std::move(reason));
}

Value Value::redacted(std::string reason) {
  return Value(ValueState::redacted, ValueType::none, 0, std::move(reason));
}

float Value::f32_value() const noexcept { return std::bit_cast<float>(f32_bits()); }

std::string Value::to_json() const {
  std::string out;
  out.reserve(48);

  out += R"({"state":)";
  switch (state_) {
    case ValueState::observed:
      out += R"("observed")";
      break;
    case ValueState::null_value:
      out += R"("null"})";
      return out;
    case ValueState::unavailable:
      out += R"("unavailable","reason":)";
      text::append_json_string(out, text_);
      out += '}';
      return out;
    case ValueState::redacted:
      out += R"("redacted","reason":)";
      text::append_json_string(out, text_);
      out += '}';
      return out;
  }

  switch (type_) {
    case ValueType::u64: {
      out += R"(,"type":"u64","value":)";
      std::string hex;
      text::append_hex(hex, number_);
      text::append_json_string(out, hex);
      break;
    }
    case ValueType::i32:
      out += R"(,"type":"i32","value":)";
      out += std::to_string(as_i32());
      break;
    case ValueType::f32: {
      out += R"(,"type":"f32","value":)";
      append_float_value(out, f32_bits());
      out += R"(,"bits":)";
      std::string hex;
      text::append_hex(hex, f32_bits());
      text::append_json_string(out, hex);
      break;
    }
    case ValueType::boolean:
      out += R"(,"type":"bool","value":)";
      out += as_bool() ? "true" : "false";
      break;
    case ValueType::str:
      out += R"(,"type":"str","value":)";
      text::append_json_string(out, text_);
      break;
    case ValueType::bytes:
      out += R"(,"type":"bytes","value":)";
      text::append_json_string(out, text_);
      out += R"(,"encoding":"base64")";
      break;
    case ValueType::ptr:
      out += R"(,"type":"ptr","value":)";
      {
        std::string hex;
        text::append_hex(hex, number_);
        text::append_json_string(out, hex);
      }
      break;
    case ValueType::none:
      break;
  }
  out += '}';
  return out;
}

Equality compare(const Value& lhs, const Value& rhs) {
  // "Unavailable | Never equal to anything", "Redacted | Never equal to anything".
  // Not even to an identical copy: absence is not evidence.
  if (lhs.state() == ValueState::unavailable || lhs.state() == ValueState::redacted ||
      rhs.state() == ValueState::unavailable || rhs.state() == ValueState::redacted) {
    return Equality::different;
  }

  // "Null | Equal only to null."
  if (lhs.state() == ValueState::null_value || rhs.state() == ValueState::null_value) {
    return lhs.state() == rhs.state() ? Equality::equal : Equality::different;
  }

  // Both observed. A declared type mismatch is a different observation, never a
  // converted comparison.
  if (lhs.type() != rhs.type()) {
    return Equality::different;
  }

  switch (lhs.type()) {
    case ValueType::ptr:
      // "Pointer | Excluded unless the manifest declares an identity rule." The
      // manifest-specific rule is the comparator's job; the model's default is exclusion.
      return Equality::excluded;
    case ValueType::u64:
      return lhs.as_u64() == rhs.as_u64() ? Equality::equal : Equality::different;
    case ValueType::i32:
      return lhs.as_i32() == rhs.as_i32() ? Equality::equal : Equality::different;
    case ValueType::f32:
      // Bits are authoritative (and make -0.0 vs +0.0, and NaN payloads, decidable).
      return lhs.f32_bits() == rhs.f32_bits() ? Equality::equal : Equality::different;
    case ValueType::boolean:
      return lhs.as_bool() == rhs.as_bool() ? Equality::equal : Equality::different;
    case ValueType::str:
      return lhs.as_str() == rhs.as_str() ? Equality::equal : Equality::different;
    case ValueType::bytes:
      return lhs.as_bytes_base64() == rhs.as_bytes_base64() ? Equality::equal : Equality::different;
    case ValueType::none:
      break;
  }
  return Equality::different;
}

} // namespace mh
