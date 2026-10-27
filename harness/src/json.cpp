// JSON tree: construction and the two renderings (document order, canonical order).

#include "mh/json.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

#include "mh/text.hpp"

namespace mh::json {
namespace {

// Shortest decimal that round-trips to the same double; 0.5 stays "0.5", 1.0 becomes
// "1". A canonical form must not depend on stream formatting state or locale.
std::string format_real(double value) {
  char buffer[32];
  const auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
  if (result.ec != std::errc{}) {
    throw std::runtime_error("json: cannot format real value");
  }
  return std::string(buffer, result.ptr);
}

} // namespace

Json Json::null() {
  return Json(Kind::null);
}

Json Json::boolean(bool value) {
  Json node(Kind::boolean);
  node.bool_ = value;
  return node;
}

Json Json::integer(std::int64_t value) {
  Json node(Kind::integer);
  node.integer_ = value;
  return node;
}

Json Json::real(double value) {
  if (!std::isfinite(value)) {
    throw std::invalid_argument("json: real value must be finite");
  }
  Json node(Kind::real);
  node.real_ = value;
  return node;
}

Json Json::string(std::string_view value) {
  Json node(Kind::string);
  node.text_ = std::string{value};
  return node;
}

Json Json::array() {
  return Json(Kind::array);
}

Json Json::object() {
  return Json(Kind::object);
}

void Json::set(std::string key, Json value) {
  if (kind_ != Kind::object) {
    throw std::invalid_argument("json: set() requires an object");
  }
  for (const auto& member : members_) {
    if (member.first == key) {
      throw std::invalid_argument("json: duplicate object key " + key);
    }
  }
  members_.emplace_back(std::move(key), std::move(value));
}

void Json::push(Json value) {
  if (kind_ != Kind::array) {
    throw std::invalid_argument("json: push() requires an array");
  }
  items_.push_back(std::move(value));
}

void Json::write_members(std::string& out, bool canonical) const {
  // Canonical order is UTF-8 byte order. std::string's operator< is exactly that and is
  // locale-independent, so the order does not depend on the machine the trace was hashed
  // on.
  std::vector<std::size_t> order(members_.size());
  std::iota(order.begin(), order.end(), std::size_t{0});
  if (canonical) {
    std::sort(order.begin(), order.end(), [this](std::size_t lhs, std::size_t rhs) {
      return members_[lhs].first < members_[rhs].first;
    });
  }

  out += '{';
  bool first = true;
  for (const std::size_t index : order) {
    if (!first) {
      out += ',';
    }
    first = false;
    text::append_json_string(out, members_[index].first);
    out += ':';
    members_[index].second.write_to(out, canonical);
  }
  out += '}';
}

void Json::write_to(std::string& out, bool canonical) const {
  switch (kind_) {
    case Kind::null:
      out += "null";
      return;
    case Kind::boolean:
      out += bool_ ? "true" : "false";
      return;
    case Kind::integer:
      out += std::to_string(integer_);
      return;
    case Kind::real:
      out += format_real(real_);
      return;
    case Kind::string:
      text::append_json_string(out, text_);
      return;
    case Kind::array:
      out += '[';
      for (std::size_t index = 0; index < items_.size(); ++index) {
        if (index != 0) {
          out += ',';
        }
        items_[index].write_to(out, canonical);
      }
      out += ']';
      return;
    case Kind::object:
      write_members(out, canonical);
      return;
  }
}

std::string Json::to_json() const {
  std::string out;
  write_to(out, /*canonical=*/false);
  return out;
}

std::string Json::to_canonical() const {
  std::string out;
  write_to(out, /*canonical=*/true);
  return out;
}

} // namespace mh::json
