// Per-key sequence alignment.
//
// Source: notes/runtime-validation.md §Comparison rules 3:
//
//   - "Before alignment, enforce each required hook's `min_count` on **both** sides. A
//     required key with zero events is missing, not a vacuous match."
//   - "Then compare the ordered payload sequences with a deterministic sequence alignment
//     (longest common subsequence / Needleman-Wunsch)."
//   - "A match requires equal canonical payloads within the declared tolerances. A gap is a
//     missing event; a mismatched pair is a changed event."
//   - "Tie-break by preferring an exact payload match, then the earlier release event, then
//     the earlier reconstruction event."
//   - "Per-key matching is intentional: it tolerates unrelated interleaving from
//     render/audio threads without pretending the global order is identical."
//
// Equality of payloads is a caller-supplied predicate, so value comparison with the
// manifest's tolerances (item 10) plugs in without this unit re-implementing it.
//
// Terminology follows "missing in release or reconstruction": `missing` means present in
// the release only, `extra` means present in the reconstruction only.

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mh {

enum class AlignmentVerdict { matched, changed, missing, extra };
std::string_view to_string(AlignmentVerdict verdict) noexcept;
std::ostream& operator<<(std::ostream& out, AlignmentVerdict verdict);

struct AlignedPair {
  AlignmentVerdict verdict = AlignmentVerdict::matched;
  // `std::nullopt` where the side has no event (missing/extra).
  std::optional<std::size_t> release_index;
  std::optional<std::size_t> reconstruction_index;
};

struct SequenceAlignment {
  std::vector<AlignedPair> pairs;
  std::int64_t matched = 0;
  std::int64_t changed = 0;
  std::int64_t missing = 0;  // only in the release
  std::int64_t extra = 0;    // only in the reconstruction

  bool clean() const noexcept { return changed == 0 && missing == 0 && extra == 0; }
};

using PayloadEqual = std::function<bool(std::size_t release_index,
                                        std::size_t reconstruction_index)>;

// Longest-common-subsequence alignment with the contract's tie-breaks. Adjacent
// missing/extra pairs are coalesced into `changed` pairs, because "a mismatched pair is a
// changed event" rather than two absences.
SequenceAlignment align_sequences(std::size_t release_size, std::size_t reconstruction_size,
                                  const PayloadEqual& equal);

// Convenience for callers whose payloads are already canonical strings.
SequenceAlignment align_sequences(const std::vector<std::string>& release,
                                  const std::vector<std::string>& reconstruction);

// One required key and its minimum occurrence count on each side.
struct RequiredKeyCount {
  std::string key;
  std::int64_t min_count = 1;
};

struct MinCountFinding {
  std::string key;
  std::int64_t required = 0;
  std::int64_t release_count = 0;
  std::int64_t reconstruction_count = 0;
};

// §Comparison rules 3, first sentence: every required key must reach its min_count on both
// sides before any alignment is trusted.
std::vector<MinCountFinding> check_min_counts(
    const std::vector<RequiredKeyCount>& required,
    const std::map<std::string, std::int64_t>& release_counts,
    const std::map<std::string, std::int64_t>& reconstruction_counts);

struct KeyAlignment {
  std::string key;
  SequenceAlignment alignment;
};

// Aligns every key present on either side, in key order, so a key that exists only in one
// variant is reported rather than silently skipped.
std::vector<KeyAlignment> align_by_key(
    const std::map<std::string, std::vector<std::string>>& release,
    const std::map<std::string, std::vector<std::string>>& reconstruction);

} // namespace mh
