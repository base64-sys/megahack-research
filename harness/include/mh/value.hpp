// The value model: one tagged object per captured value.
//
// Source: notes/runtime-validation.md §Value objects. The rules that shape this header:
//
//   - "Every captured value is a tagged object, so an unavailable value cannot be
//     confused with a real null."
//   - "Floats always carry `bits`; `value` is not used for equality when `bits` is
//     present."
//   - "Pointers are recorded for correlation but are excluded from equality because heap
//     and module bases vary."
//   - §Comparison rules 5: unavailable and redacted are never equal to anything, null is
//     equal only to null.
//
// There is deliberately no untyped constructor and no default constructor: a Value is
// always in one of the four states, and the observed ones always carry a type.

#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace mh {

// `observed` carries a type and a payload; the other three are the "not a value"
// states from the contract's table.
enum class ValueState { observed, null_value, unavailable, redacted };

// `none` is only legal for the three non-observed states.
enum class ValueType { none, u64, i32, f32, boolean, str, bytes, ptr };

// Comparison outcomes, from §Comparison rules 5: a pointer is "excluded", which is not
// the same answer as "different" (an excluded field cannot make a run fail).
enum class Equality { equal, different, excluded };

class Value {
 public:
  Value(const Value&) = default;
  Value(Value&&) = default;
  Value& operator=(const Value&) = default;
  Value& operator=(Value&&) = default;

  // -- observed payloads ------------------------------------------------------
  static Value observed_u64(std::uint64_t value);
  static Value observed_i32(std::int32_t value);
  // The bits are the observation. `value` in JSON is generated from them.
  static Value observed_f32_bits(std::uint32_t bits);
  static Value observed_bool(bool value);
  static Value observed_str(std::string_view value);
  // Base64 is the wire spelling of opaque bytes (§Value objects).
  static Value observed_bytes(std::string_view base64);
  static Value observed_ptr(std::uint64_t address);

  // -- non-values -------------------------------------------------------------
  static Value null_value();
  static Value unavailable(std::string reason);
  static Value redacted(std::string reason);

  // -- inspection -------------------------------------------------------------
  ValueState state() const noexcept { return state_; }
  ValueType type() const noexcept { return type_; }
  bool is_observed() const noexcept { return state_ == ValueState::observed; }

  // Precondition on each accessor: type() is the matching type.
  std::uint64_t as_u64() const noexcept { return number_; }
  std::int32_t as_i32() const noexcept { return static_cast<std::int32_t>(number_); }
  std::uint32_t f32_bits() const noexcept { return static_cast<std::uint32_t>(number_); }
  float f32_value() const noexcept;
  bool as_bool() const noexcept { return number_ != 0; }
  std::string_view as_str() const noexcept { return text_; }
  std::string_view as_bytes_base64() const noexcept { return text_; }
  std::uint64_t as_ptr() const noexcept { return number_; }
  // Reason for unavailable/redacted; empty otherwise.
  std::string_view reason() const noexcept { return text_; }

  // Canonical object for the trace, in the contract's own field order:
  // {"state":...,"type":...,"value":...,"bits":...}. No insignificant whitespace.
  std::string to_json() const;

 private:
  Value(ValueState state, ValueType type, std::uint64_t number, std::string text);

  ValueState state_;
  ValueType type_;
  std::uint64_t number_ = 0;
  std::string text_;
};

// §Comparison rules 5. Both observed values of the same type compare exactly, except
// f32 (bits) and ptr (excluded). `unavailable` and `redacted` never compare equal, not
// even to themselves: an absent observation is not evidence of equality.
Equality compare(const Value& lhs, const Value& rhs);

} // namespace mh
