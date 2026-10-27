#include "mh/json_parse.hpp"

#include <charconv>
#include <cstdint>
#include <optional>
#include <string>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <utility>

namespace mh::json {
namespace {

// Deep enough for a trace record, shallow enough that a hostile or corrupt document
// cannot exhaust the stack.
constexpr int kMaxDepth = 128;

struct ParseFailure {
  std::size_t offset;
  std::string message;
};

class Parser {
 public:
  explicit Parser(std::string_view text) : text_(text) {}

  ParseResult run() {
    try {
      skip_whitespace();
      Json value = parse_value(0);
      skip_whitespace();
      if (position_ != text_.size()) {
        fail("trailing characters after the document");
      }
      ParseResult result;
      result.value = std::move(value);
      return result;
    } catch (const ParseFailure& failure) {
      ParseResult result;
      result.error = ParseError{failure.offset, failure.message};
      return result;
    }
  }

 private:
  [[noreturn]] void fail(std::string message) const {
    throw ParseFailure{position_, std::move(message)};
  }

  bool at_end() const { return position_ >= text_.size(); }
  char peek() const { return at_end() ? '\0' : text_[position_]; }

  void skip_whitespace() {
    while (!at_end()) {
      const char c = text_[position_];
      if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
        ++position_;
      } else {
        return;
      }
    }
  }

  void expect(std::string_view literal) {
    for (const char expected : literal) {
      if (at_end() || text_[position_] != expected) {
        fail("expected " + std::string{literal});
      }
      ++position_;
    }
  }

  Json parse_value(int depth) {
    if (depth > kMaxDepth) {
      fail("nesting too deep");
    }
    if (at_end()) {
      fail("unexpected end of document");
    }
    switch (peek()) {
      case '{':
        return parse_object(depth);
      case '[':
        return parse_array(depth);
      case '"':
        return Json::string(parse_string());
      case 't':
        expect("true");
        return Json::boolean(true);
      case 'f':
        expect("false");
        return Json::boolean(false);
      case 'n':
        expect("null");
        return Json::null();
      default:
        return parse_number();
    }
  }

  Json parse_object(int depth) {
    ++position_;  // '{'
    Json object = Json::object();
    skip_whitespace();
    if (peek() == '}') {
      ++position_;
      return object;
    }

    while (true) {
      skip_whitespace();
      if (peek() != '"') {
        fail("object key must be a string");
      }
      std::string key = parse_string();
      skip_whitespace();
      if (peek() != ':') {
        fail("expected ':' after object key");
      }
      ++position_;
      skip_whitespace();

      const std::size_t key_offset = position_;
      Json member = parse_value(depth + 1);
      try {
        object.set(std::move(key), std::move(member));
      } catch (const std::invalid_argument&) {
        position_ = key_offset;
        fail("duplicate object key");
      }

      skip_whitespace();
      if (peek() == ',') {
        ++position_;
        continue;
      }
      if (peek() == '}') {
        ++position_;
        return object;
      }
      fail("expected ',' or '}' in object");
    }
  }

  Json parse_array(int depth) {
    ++position_;  // '['
    Json array = Json::array();
    skip_whitespace();
    if (peek() == ']') {
      ++position_;
      return array;
    }

    while (true) {
      skip_whitespace();
      array.push(parse_value(depth + 1));
      skip_whitespace();
      if (peek() == ',') {
        ++position_;
        continue;
      }
      if (peek() == ']') {
        ++position_;
        return array;
      }
      fail("expected ',' or ']' in array");
    }
  }

  static void append_utf8(std::string& out, std::uint32_t codepoint) {
    if (codepoint < 0x80) {
      out += static_cast<char>(codepoint);
    } else if (codepoint < 0x800) {
      out += static_cast<char>(0xC0 | (codepoint >> 6));
      out += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else if (codepoint < 0x10000) {
      out += static_cast<char>(0xE0 | (codepoint >> 12));
      out += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
      out += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else {
      out += static_cast<char>(0xF0 | (codepoint >> 18));
      out += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
      out += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
      out += static_cast<char>(0x80 | (codepoint & 0x3F));
    }
  }

  std::uint32_t parse_hex4() {
    std::uint32_t value = 0;
    for (int i = 0; i < 4; ++i) {
      if (at_end()) {
        fail("truncated \\u escape");
      }
      const char c = text_[position_++];
      value <<= 4;
      if (c >= '0' && c <= '9') {
        value |= static_cast<std::uint32_t>(c - '0');
      } else if (c >= 'a' && c <= 'f') {
        value |= static_cast<std::uint32_t>(c - 'a' + 10);
      } else if (c >= 'A' && c <= 'F') {
        value |= static_cast<std::uint32_t>(c - 'A' + 10);
      } else {
        fail("invalid hex digit in \\u escape");
      }
    }
    return value;
  }

  std::string parse_string() {
    ++position_;  // opening quote
    std::string out;
    while (true) {
      if (at_end()) {
        fail("unterminated string");
      }
      const auto c = static_cast<unsigned char>(text_[position_]);
      if (c == '"') {
        ++position_;
        return out;
      }
      if (c == '\\') {
        ++position_;
        if (at_end()) {
          fail("unterminated escape");
        }
        const char escape = text_[position_++];
        switch (escape) {
          case '"':
          case '\\':
          case '/':
            out += escape;
            break;
          case 'b':
            out += '\b';
            break;
          case 'f':
            out += '\f';
            break;
          case 'n':
            out += '\n';
            break;
          case 'r':
            out += '\r';
            break;
          case 't':
            out += '\t';
            break;
          case 'u': {
            const std::uint32_t first = parse_hex4();
            if (first >= 0xD800 && first <= 0xDBFF) {
              // A high surrogate must be followed by a low surrogate; a lone one is not
              // a character and must not be smuggled into the plan as bytes.
              if (at_end() || text_[position_] != '\\' || position_ + 1 >= text_.size() ||
                  text_[position_ + 1] != 'u') {
                fail("high surrogate without a low surrogate");
              }
              position_ += 2;
              const std::uint32_t second = parse_hex4();
              if (second < 0xDC00 || second > 0xDFFF) {
                fail("invalid low surrogate");
              }
              append_utf8(out, 0x10000 + ((first - 0xD800) << 10) + (second - 0xDC00));
            } else if (first >= 0xDC00 && first <= 0xDFFF) {
              fail("lone low surrogate");
            } else {
              append_utf8(out, first);
            }
            break;
          }
          default:
            fail("invalid escape sequence");
        }
        continue;
      }
      if (c < 0x20) {
        fail("unescaped control character in string");
      }
      out += static_cast<char>(c);
      ++position_;
    }
  }

  Json parse_number() {
    const std::size_t start = position_;
    if (peek() == '-') {
      ++position_;
    }
    if (at_end() || peek() < '0' || peek() > '9') {
      fail("expected a number");
    }
    if (peek() == '0') {
      ++position_;
      if (!at_end() && peek() >= '0' && peek() <= '9') {
        fail("leading zero in number");
      }
    } else {
      while (!at_end() && peek() >= '0' && peek() <= '9') {
        ++position_;
      }
    }

    bool is_real = false;
    if (peek() == '.') {
      is_real = true;
      ++position_;
      if (at_end() || peek() < '0' || peek() > '9') {
        fail("fraction without digits");
      }
      while (!at_end() && peek() >= '0' && peek() <= '9') {
        ++position_;
      }
    }
    if (peek() == 'e' || peek() == 'E') {
      is_real = true;
      ++position_;
      if (peek() == '+' || peek() == '-') {
        ++position_;
      }
      if (at_end() || peek() < '0' || peek() > '9') {
        fail("exponent without digits");
      }
      while (!at_end() && peek() >= '0' && peek() <= '9') {
        ++position_;
      }
    }

    const std::string_view literal = text_.substr(start, position_ - start);
    if (!is_real) {
      std::int64_t integer = 0;
      const auto result =
          std::from_chars(literal.data(), literal.data() + literal.size(), integer);
      if (result.ec == std::errc{} && result.ptr == literal.data() + literal.size()) {
        return Json::integer(integer);
      }
      // Out of int64 range: keep the value as a real rather than truncating it.
    }

    double real = 0.0;
    const auto result = std::from_chars(literal.data(), literal.data() + literal.size(), real);
    if (result.ec == std::errc::result_out_of_range) {
      fail("number out of range");
    }
    if (result.ec != std::errc{} || result.ptr != literal.data() + literal.size()) {
      fail("invalid number");
    }
    return Json::real(real);
  }

  std::string_view text_;
  std::size_t position_ = 0;
};

} // namespace

ParseResult parse(std::string_view text) {
  Parser parser(text);
  return parser.run();
}

} // namespace mh::json
