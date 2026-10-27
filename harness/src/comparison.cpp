#include "mh/comparison.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstddef>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

#include "mh/text.hpp"

namespace mh {
namespace {

const json::Json* member(const json::Json& object, std::string_view key) {
  for (const auto& entry : object.members()) {
    if (entry.first == key) {
      return &entry.second;
    }
  }
  return nullptr;
}

std::optional<std::string> string_member(const json::Json& object, std::string_view key) {
  const json::Json* node = member(object, key);
  if (node == nullptr || node->kind() != json::Json::Kind::string) {
    return std::nullopt;
  }
  return std::string{node->string_value()};
}

// Hex in value objects is written by this tree's own writer (mh::text::append_hex), so the
// reader accepts either case: the number is the observation, not its spelling.
std::optional<std::uint64_t> hex_member(const json::Json& object, std::string_view key) {
  const std::optional<std::string> text = string_member(object, key);
  if (!text.has_value() || text->size() < 3 || (*text)[0] != '0' ||
      ((*text)[1] != 'x' && (*text)[1] != 'X')) {
    return std::nullopt;
  }
  std::uint64_t value = 0;
  for (std::size_t index = 2; index < text->size(); ++index) {
    const char c = (*text)[index];
    const int digit = (c >= '0' && c <= '9')   ? c - '0'
                      : (c >= 'a' && c <= 'f') ? c - 'a' + 10
                      : (c >= 'A' && c <= 'F') ? c - 'A' + 10
                                               : -1;
    if (digit < 0) {
      return std::nullopt;
    }
    value = (value << 4) | static_cast<std::uint64_t>(digit);
  }
  return value;
}

bool tolerance_is_usable(const Tolerance& tolerance) {
  // "A tolerance without a `why` is invalid."
  return !tolerance.why.empty();
}

bool within_tolerance(double lhs, double rhs, const Tolerance& tolerance) {
  // abs(a-b) <= abs_tol + rel_tol * max(abs(a), abs(b))
  const double difference = std::fabs(lhs - rhs);
  const double limit =
      tolerance.abs_tolerance + tolerance.rel_tolerance * std::max(std::fabs(lhs), std::fabs(rhs));
  return difference <= limit;
}

} // namespace

std::optional<Value> value_from_json(const json::Json& node) {
  if (!node.is_object()) {
    return std::nullopt;
  }
  const std::optional<std::string> state = string_member(node, "state");
  if (!state.has_value()) {
    return std::nullopt;
  }
  if (*state == "null") {
    return Value::null_value();
  }
  if (*state == "unavailable" || *state == "redacted") {
    const std::string reason = string_member(node, "reason").value_or("");
    return *state == "unavailable" ? Value::unavailable(reason) : Value::redacted(reason);
  }
  if (*state != "observed") {
    return std::nullopt;
  }

  const std::optional<std::string> type = string_member(node, "type");
  if (!type.has_value()) {
    return std::nullopt;
  }
  if (*type == "u64") {
    const std::optional<std::uint64_t> value = hex_member(node, "value");
    return value.has_value() ? std::optional<Value>{Value::observed_u64(*value)} : std::nullopt;
  }
  if (*type == "ptr") {
    const std::optional<std::uint64_t> value = hex_member(node, "value");
    return value.has_value() ? std::optional<Value>{Value::observed_ptr(*value)} : std::nullopt;
  }
  if (*type == "f32") {
    const std::optional<std::uint64_t> bits = hex_member(node, "bits");
    if (!bits.has_value() || *bits > 0xFFFFFFFFull) {
      return std::nullopt;
    }
    return Value::observed_f32_bits(static_cast<std::uint32_t>(*bits));
  }
  if (*type == "i32") {
    const json::Json* value = member(node, "value");
    if (value == nullptr || value->kind() != json::Json::Kind::integer) {
      return std::nullopt;
    }
    return Value::observed_i32(static_cast<std::int32_t>(value->integer_value()));
  }
  if (*type == "bool") {
    const json::Json* value = member(node, "value");
    if (value == nullptr || value->kind() != json::Json::Kind::boolean) {
      return std::nullopt;
    }
    return Value::observed_bool(value->boolean_value());
  }
  if (*type == "str" || *type == "bytes") {
    const std::optional<std::string> value = string_member(node, "value");
    if (!value.has_value()) {
      return std::nullopt;
    }
    return *type == "str" ? std::optional<Value>{Value::observed_str(*value)}
                          : std::optional<Value>{Value::observed_bytes(*value)};
  }
  return std::nullopt;
}

std::string_view to_string(ValueVerdict verdict) noexcept {
  switch (verdict) {
    case ValueVerdict::equal:
      return "equal";
    case ValueVerdict::different:
      return "different";
    case ValueVerdict::excluded:
      return "excluded";
    case ValueVerdict::blocked:
      return "blocked";
  }
  return {};
}

std::ostream& operator<<(std::ostream& out, ValueVerdict verdict) {
  return out << to_string(verdict);
}

ValueComparison compare_value_json(const json::Json& release, const json::Json& reconstruction,
                                   const std::optional<Tolerance>& tolerance) {
  const std::optional<Value> lhs = value_from_json(release);
  const std::optional<Value> rhs = value_from_json(reconstruction);
  if (!lhs.has_value() || !rhs.has_value()) {
    // Not a value object at all: the trace cannot establish either side of the claim.
    return ValueComparison{ValueVerdict::blocked, "malformed value object"};
  }

  // §6: "Capture unavailable | Blocked" — unavailable and redacted establish nothing.
  if (!lhs->is_observed() || !rhs->is_observed()) {
    if (lhs->state() == ValueState::unavailable || rhs->state() == ValueState::unavailable ||
        lhs->state() == ValueState::redacted || rhs->state() == ValueState::redacted) {
      return ValueComparison{ValueVerdict::blocked, "a side is unavailable or redacted"};
    }
  }

  // Float with a declared, explained tolerance: §5 compares the decoded value, not the bits.
  if (tolerance.has_value() && tolerance_is_usable(*tolerance) && lhs->is_observed() &&
      rhs->is_observed() && lhs->type() == ValueType::f32 && rhs->type() == ValueType::f32) {
    const bool close =
        within_tolerance(static_cast<double>(lhs->f32_value()),
                         static_cast<double>(rhs->f32_value()), *tolerance);
    return ValueComparison{close ? ValueVerdict::equal : ValueVerdict::different,
                           close ? "within tolerance" : "outside tolerance"};
  }

  switch (compare(*lhs, *rhs)) {
    case Equality::equal:
      return ValueComparison{ValueVerdict::equal, "exact"};
    case Equality::excluded:
      return ValueComparison{ValueVerdict::excluded, "pointer excluded from equality"};
    case Equality::different:
      break;
  }
  return ValueComparison{ValueVerdict::different, "values differ"};
}

OrderingResult check_order_group(const std::vector<OrderEvent>& events,
                                 const std::vector<std::string>& group) {
  OrderingResult result;
  result.satisfied = true;

  std::size_t cursor = 0;
  for (const std::string& name : group) {
    bool found = false;
    for (std::size_t index = cursor; index < events.size(); ++index) {
      // §4: "If the harness cannot prove a thread is `main`, its events are not eligible to
      // satisfy an ordered group."
      if (events[index].thread_role != "main") {
        continue;
      }
      if (events[index].milestone_name == name || events[index].hook_key == name) {
        cursor = index + 1;
        found = true;
        break;
      }
    }
    if (!found) {
      result.satisfied = false;
      result.missing.push_back(name);
    }
  }
  return result;
}

std::string_view to_string(ComparisonStatus status) noexcept {
  switch (status) {
    case ComparisonStatus::pass:
      return "pass";
    case ComparisonStatus::pass_with_warnings:
      return "pass_with_warnings";
    case ComparisonStatus::fail:
      return "fail";
    case ComparisonStatus::blocked:
      return "blocked";
    case ComparisonStatus::unstable:
      return "unstable";
    case ComparisonStatus::invalid:
      return "invalid";
  }
  return {};
}

std::ostream& operator<<(std::ostream& out, ComparisonStatus status) {
  return out << to_string(status);
}

ComparisonStatus finalize_status(GateStatus gate, const std::vector<Finding>& findings,
                                 bool unstable) {
  if (gate == GateStatus::invalid) {
    return ComparisonStatus::invalid;
  }
  if (gate == GateStatus::blocked) {
    return ComparisonStatus::blocked;
  }
  // §8: "If a required event, payload, or order differs within a variant, the scenario is
  // `unstable`; do not proceed to a cross-variant claim."
  if (unstable) {
    return ComparisonStatus::unstable;
  }

  bool warned = false;
  for (const Finding& finding : findings) {
    if (finding.severity == ComparisonStatus::fail ||
        finding.severity == ComparisonStatus::blocked ||
        finding.severity == ComparisonStatus::invalid) {
      return finding.severity;
    }
    if (finding.severity == ComparisonStatus::pass_with_warnings) {
      warned = true;
    }
  }
  return warned ? ComparisonStatus::pass_with_warnings : ComparisonStatus::pass;
}

} // namespace mh
