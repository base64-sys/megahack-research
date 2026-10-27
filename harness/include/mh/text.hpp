// Byte-level helpers shared by every writer in the trace core.
//
// One escaping implementation and one hex spelling, because the trace writer, the
// canonical writer and the value model must agree byte for byte: a second copy is how a
// round-trip quietly stops being a round-trip.

#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace mh::text {

// Upper case matches the contract's own value-object examples (0x3F000000, 0x1F4A2B00000).
inline constexpr char kUpperHex[] = "0123456789ABCDEF";

// "0x" + minimal upper-case hex digits (no zero padding, "0x0" for zero).
inline void append_hex(std::string& out, std::uint64_t value) {
  char digits[16];
  int count = 0;
  do {
    digits[count++] = kUpperHex[value & 0xF];
    value >>= 4;
  } while (value != 0);

  out += "0x";
  while (count > 0) {
    out += digits[--count];
  }
}

// JSON string body with the escapes RFC 8259 requires. Valid UTF-8 passes through
// untouched: the model never re-encodes or normalises, because a redacted string is the
// only player-data string a trace is allowed to carry.
inline void append_json_body(std::string& out, std::string_view text) {
  for (const char raw : text) {
    const auto byte = static_cast<unsigned char>(raw);
    switch (byte) {
      case '"':
        out += "\\\"";
        break;
      case '\\':
        out += "\\\\";
        break;
      case '\b':
        out += "\\b";
        break;
      case '\f':
        out += "\\f";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        if (byte < 0x20) {
          out += "\\u00";
          out += kUpperHex[(byte >> 4) & 0xF];
          out += kUpperHex[byte & 0xF];
        } else {
          out += raw;
        }
        break;
    }
  }
}

// Quoted JSON string.
inline void append_json_string(std::string& out, std::string_view text) {
  out += '"';
  append_json_body(out, text);
  out += '"';
}

} // namespace mh::text
