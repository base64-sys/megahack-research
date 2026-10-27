// Contract vocabulary that needs a definition rather than a constant.
//
// Source: notes/runtime-validation.md §Trace format (record type table). The three
// spellings are the contract's; parsing is exact, because a typo in `type` is a broken
// trace, not a newer dialect.

#include "mh/contract.hpp"

namespace mh::contract {

std::optional<RecordType> parse_record_type(std::string_view text) {
  if (text == kTypeMeta) {
    return RecordType::meta;
  }
  if (text == kTypeEvent) {
    return RecordType::event;
  }
  if (text == kTypeEnd) {
    return RecordType::end;
  }
  return std::nullopt;
}

std::string_view to_string(RecordType type) noexcept {
  switch (type) {
    case RecordType::meta:
      return kTypeMeta;
    case RecordType::event:
      return kTypeEvent;
    case RecordType::end:
      return kTypeEnd;
  }
  return {};
}

} // namespace mh::contract
