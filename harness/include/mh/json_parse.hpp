// Strict JSON parser (RFC 8259), producing the same tree the writer renders.
//
// There is no third-party dependency in this tree, and a trace cannot be validated
// without a parser. The reader is deliberately strict where the contract is strict:
//
//   - notes/runtime-validation.md §Trace format: "one complete JSON object per line, no
//     comments, no BOM." A BOM is therefore a parse error, not something to strip.
//   - Duplicate object keys are rejected: two values for one key have no single
//     interpretation, and the canonical writer would have to drop one silently.
//   - Documents preserve key order, so a parsed line can be written back unchanged; the
//     canonical (sorted) rendering is a separate call.
//
// Numbers with a fraction or an exponent become `real`; a plain integer literal that fits
// in int64 becomes `integer`. An integer literal that does not fit becomes `real` rather
// than being truncated.

#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

#include "mh/json.hpp"

namespace mh::json {

struct ParseError {
  std::size_t offset = 0;
  std::string message;
};

struct ParseResult {
  std::optional<Json> value;
  std::optional<ParseError> error;

  bool ok() const noexcept { return value.has_value(); }
};

// Parses exactly one JSON value with optional surrounding whitespace. Trailing non-space
// characters are an error.
ParseResult parse(std::string_view text);

} // namespace mh::json
