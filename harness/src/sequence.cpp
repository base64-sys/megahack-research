#include "mh/sequence.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <numeric>
#include <optional>
#include <ostream>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mh {
namespace {

struct RawStep {
  AlignmentVerdict verdict;
  std::optional<std::size_t> release_index;
  std::optional<std::size_t> reconstruction_index;
};

// "A mismatched pair is a changed event": an adjacent missing/extra pair is one
// substitution, not two absences.
std::vector<AlignedPair> coalesce(const std::vector<RawStep>& raw) {
  std::vector<AlignedPair> pairs;
  std::size_t index = 0;
  while (index < raw.size()) {
    const RawStep& current = raw[index];
    const bool has_next = index + 1 < raw.size();

    if (has_next && current.verdict == AlignmentVerdict::missing &&
        raw[index + 1].verdict == AlignmentVerdict::extra) {
      pairs.push_back(AlignedPair{AlignmentVerdict::changed, current.release_index,
                                  raw[index + 1].reconstruction_index});
      index += 2;
      continue;
    }
    if (has_next && current.verdict == AlignmentVerdict::extra &&
        raw[index + 1].verdict == AlignmentVerdict::missing) {
      pairs.push_back(AlignedPair{AlignmentVerdict::changed, raw[index + 1].release_index,
                                  current.reconstruction_index});
      index += 2;
      continue;
    }
    pairs.push_back(AlignedPair{current.verdict, current.release_index,
                               current.reconstruction_index});
    ++index;
  }
  return pairs;
}

void count(SequenceAlignment& alignment) {
  for (const AlignedPair& pair : alignment.pairs) {
    switch (pair.verdict) {
      case AlignmentVerdict::matched:
        ++alignment.matched;
        break;
      case AlignmentVerdict::changed:
        ++alignment.changed;
        break;
      case AlignmentVerdict::missing:
        ++alignment.missing;
        break;
      case AlignmentVerdict::extra:
        ++alignment.extra;
        break;
    }
  }
}

} // namespace

std::string_view to_string(AlignmentVerdict verdict) noexcept {
  switch (verdict) {
    case AlignmentVerdict::matched:
      return "matched";
    case AlignmentVerdict::changed:
      return "changed";
    case AlignmentVerdict::missing:
      return "missing";
    case AlignmentVerdict::extra:
      return "extra";
  }
  return {};
}

std::ostream& operator<<(std::ostream& out, AlignmentVerdict verdict) {
  return out << to_string(verdict);
}

SequenceAlignment align_sequences(std::size_t release_size, std::size_t reconstruction_size,
                                  const PayloadEqual& equal) {
  // Suffix LCS table: lcs[i][j] is the longest common subsequence of release[i..) and
  // reconstruction[j..). Walking forward then lets the tie-breaks be expressed as "prefer
  // the earlier release event", which is what the contract asks for.
  std::vector<std::vector<std::int64_t>> lcs(
      release_size + 1, std::vector<std::int64_t>(reconstruction_size + 1, 0));
  for (std::size_t i = release_size; i-- > 0;) {
    for (std::size_t j = reconstruction_size; j-- > 0;) {
      if (equal(i, j)) {
        lcs[i][j] = lcs[i + 1][j + 1] + 1;
      } else {
        lcs[i][j] = std::max(lcs[i + 1][j], lcs[i][j + 1]);
      }
    }
  }

  std::vector<RawStep> raw;
  std::size_t i = 0;
  std::size_t j = 0;
  while (i < release_size || j < reconstruction_size) {
    if (i < release_size && j < reconstruction_size && equal(i, j) &&
        lcs[i][j] == lcs[i + 1][j + 1] + 1) {
      // 1. Prefer an exact payload match.
      raw.push_back(RawStep{AlignmentVerdict::matched, i, j});
      ++i;
      ++j;
      continue;
    }

    const bool can_skip_release = i < release_size;
    const bool can_skip_reconstruction = j < reconstruction_size;

    if (can_skip_release && can_skip_reconstruction) {
      const std::int64_t skip_release = lcs[i + 1][j];
      const std::int64_t skip_reconstruction = lcs[i][j + 1];
      if (skip_release >= skip_reconstruction) {
        // 2. Then the earlier release event.
        raw.push_back(RawStep{AlignmentVerdict::missing, i, std::nullopt});
        ++i;
      } else {
        // 3. Then the earlier reconstruction event.
        raw.push_back(RawStep{AlignmentVerdict::extra, std::nullopt, j});
        ++j;
      }
      continue;
    }
    if (can_skip_release) {
      raw.push_back(RawStep{AlignmentVerdict::missing, i, std::nullopt});
      ++i;
    } else {
      raw.push_back(RawStep{AlignmentVerdict::extra, std::nullopt, j});
      ++j;
    }
  }

  SequenceAlignment alignment;
  alignment.pairs = coalesce(raw);
  count(alignment);
  return alignment;
}

SequenceAlignment align_sequences(const std::vector<std::string>& release,
                                  const std::vector<std::string>& reconstruction) {
  return align_sequences(
      release.size(), reconstruction.size(),
      [&release, &reconstruction](std::size_t i, std::size_t j) {
        return release[i] == reconstruction[j];
      });
}

std::vector<MinCountFinding> check_min_counts(
    const std::vector<RequiredKeyCount>& required,
    const std::map<std::string, std::int64_t>& release_counts,
    const std::map<std::string, std::int64_t>& reconstruction_counts) {
  std::vector<MinCountFinding> findings;
  for (const RequiredKeyCount& requirement : required) {
    MinCountFinding finding;
    finding.key = requirement.key;
    finding.required = requirement.min_count;

    const auto release = release_counts.find(requirement.key);
    if (release != release_counts.end()) {
      finding.release_count = release->second;
    }
    const auto reconstruction = reconstruction_counts.find(requirement.key);
    if (reconstruction != reconstruction_counts.end()) {
      finding.reconstruction_count = reconstruction->second;
    }

    if (finding.release_count < requirement.min_count ||
        finding.reconstruction_count < requirement.min_count) {
      findings.push_back(std::move(finding));
    }
  }
  return findings;
}

std::vector<KeyAlignment> align_by_key(
    const std::map<std::string, std::vector<std::string>>& release,
    const std::map<std::string, std::vector<std::string>>& reconstruction) {
  std::set<std::string> keys;
  for (const auto& entry : release) {
    keys.insert(entry.first);
  }
  for (const auto& entry : reconstruction) {
    keys.insert(entry.first);
  }

  std::vector<KeyAlignment> alignments;
  for (const std::string& key : keys) {
    const std::vector<std::string> empty;
    const auto release_entry = release.find(key);
    const auto reconstruction_entry = reconstruction.find(key);
    const std::vector<std::string>& release_payloads =
        release_entry == release.end() ? empty : release_entry->second;
    const std::vector<std::string>& reconstruction_payloads =
        reconstruction_entry == reconstruction.end() ? empty : reconstruction_entry->second;

    alignments.push_back(KeyAlignment{key,
                                     align_sequences(release_payloads, reconstruction_payloads)});
  }
  return alignments;
}

} // namespace mh
