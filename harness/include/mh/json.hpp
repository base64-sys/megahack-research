// A small JSON tree with two renderings.
//
// Canonical JSON is defined by the contract only for run_fingerprint's input
// (notes/runtime-validation.md §Common fields: "UTF-8, sorted keys, no insignificant
// whitespace"). The distinction matters:
//
//   to_json()       insertion order, no insignificant whitespace — what a trace line
//                   looks like, and what makes parse → write a round-trip.
//   to_canonical()  keys sorted by UTF-8 byte order — what a hash is computed over.
//
// Hash the second, never the first: a document whose keys were written in a different
// order is the same document, and must fingerprint identically.

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mh::json {

class Json {
 public:
  enum class Kind { null, boolean, integer, real, string, array, object };

  static Json null();
  static Json boolean(bool value);
  static Json integer(std::int64_t value);
  // Throws std::invalid_argument for NaN/Infinity: JSON has no literal for them, and a
  // fingerprint must never substitute a different number.
  static Json real(double value);
  static Json string(std::string_view value);
  static Json array();
  static Json object();

  Kind kind() const noexcept { return kind_; }
  bool is_object() const noexcept { return kind_ == Kind::object; }
  bool is_array() const noexcept { return kind_ == Kind::array; }

  // Precondition: kind() is the matching kind. Read access is deliberately narrow: a
  // value's meaning comes from the trace schema, not from probing the tree.
  std::string_view string_value() const noexcept { return text_; }
  std::int64_t integer_value() const noexcept { return integer_; }
  double real_value() const noexcept { return real_; }
  bool boolean_value() const noexcept { return bool_; }

  // Object member. Throws std::invalid_argument when the key already exists (a document
  // with duplicate keys has no single interpretation) or when this is not an object.
  void set(std::string key, Json value);
  // Array element. Throws std::invalid_argument when this is not an array.
  void push(Json value);

  const std::vector<Json>& items() const noexcept { return items_; }
  const std::vector<std::pair<std::string, Json>>& members() const noexcept { return members_; }

  std::string to_json() const;
  std::string to_canonical() const;

 private:
  explicit Json(Kind kind) : kind_(kind) {}

  void write_to(std::string& out, bool canonical) const;
  void write_members(std::string& out, bool canonical) const;

  Kind kind_;
  bool bool_ = false;
  std::int64_t integer_ = 0;
  double real_ = 0.0;
  std::string text_;
  std::vector<Json> items_;
  std::vector<std::pair<std::string, Json>> members_;
};

} // namespace mh::json
