// Item 9 — per-key sequence alignment.
//
// Contract: notes/runtime-validation.md §Comparison rules 3. The four classifications are
// the contract's: a match, a changed event, a gap on the release side and a gap on the
// reconstruction side; the tie-breaks are "preferring an exact payload match, then the
// earlier release event, then the earlier reconstruction event".

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "mh/sequence.hpp"
#include "test_runner.hpp"

namespace {

using mh::AlignmentVerdict;
using mh::SequenceAlignment;

std::string describe(const SequenceAlignment& alignment) {
  std::string text;
  for (const mh::AlignedPair& pair : alignment.pairs) {
    text += to_string(pair.verdict);
    text += '(';
    text += pair.release_index.has_value() ? std::to_string(*pair.release_index) : "-";
    text += ',';
    text += pair.reconstruction_index.has_value() ? std::to_string(*pair.reconstruction_index)
                                                 : "-";
    text += ')';
  }
  return text;
}

} // namespace

MH_TEST(alignment_of_identical_sequences_is_all_matched) {
  const SequenceAlignment alignment = mh::align_sequences({"a", "b", "c"}, {"a", "b", "c"});

  MH_CHECK_EQ(alignment.matched, std::int64_t{3});
  MH_CHECK_EQ(alignment.changed, std::int64_t{0});
  MH_CHECK_EQ(alignment.missing, std::int64_t{0});
  MH_CHECK_EQ(alignment.extra, std::int64_t{0});
  MH_CHECK(alignment.clean());
  MH_CHECK_EQ(describe(alignment), std::string{"matched(0,0)matched(1,1)matched(2,2)"});
}

MH_TEST(alignment_reports_a_deleted_event_as_missing) {
  const SequenceAlignment alignment = mh::align_sequences({"a", "b", "c"}, {"a", "c"});

  // §6: "Missing in release or reconstruction" is one row; this is the release-side event
  // that the reconstruction never produced.
  MH_CHECK_EQ(alignment.missing, std::int64_t{1});
  MH_CHECK_EQ(alignment.extra, std::int64_t{0});
  MH_CHECK_EQ(describe(alignment), std::string{"matched(0,0)missing(1,-)matched(2,1)"});
}

MH_TEST(alignment_reports_an_inserted_event_as_extra) {
  const SequenceAlignment alignment = mh::align_sequences({"a", "c"}, {"a", "b", "c"});

  MH_CHECK_EQ(alignment.extra, std::int64_t{1});
  MH_CHECK_EQ(alignment.missing, std::int64_t{0});
  MH_CHECK_EQ(describe(alignment), std::string{"matched(0,0)extra(-,1)matched(1,2)"});
}

MH_TEST(alignment_reports_a_substitution_as_changed) {
  // "A gap is a missing event; a mismatched pair is a changed event." A same-length
  // difference is a changed pair, not a missing plus an extra.
  const SequenceAlignment alignment = mh::align_sequences({"a", "b", "c"}, {"a", "X", "c"});

  MH_CHECK_EQ(alignment.changed, std::int64_t{1});
  MH_CHECK_EQ(alignment.missing, std::int64_t{0});
  MH_CHECK_EQ(alignment.extra, std::int64_t{0});
  MH_CHECK_EQ(describe(alignment), std::string{"matched(0,0)changed(1,1)matched(2,2)"});
}

MH_TEST(alignment_prefers_a_match_over_a_gap_that_scores_the_same) {
  // release [x], reconstruction [y, x]: matching x with the later event is the only way to
  // get an LCS of 1, and the contract prefers the match.
  const SequenceAlignment alignment = mh::align_sequences({"x"}, {"y", "x"});

  MH_CHECK_EQ(describe(alignment), std::string{"extra(-,0)matched(0,1)"});
}

MH_TEST(alignment_prefers_the_earlier_release_event_on_a_tie) {
  // release [A, B], reconstruction [B]: LCS is 1 either way, so the tie-break decides.
  // "then the earlier release event" means A is reported first, as the gap, and B matches.
  const SequenceAlignment alignment = mh::align_sequences({"A", "B"}, {"B"});

  MH_CHECK_EQ(describe(alignment), std::string{"missing(0,-)matched(1,0)"});
}

MH_TEST(alignment_keeps_indices_strictly_increasing) {
  const SequenceAlignment alignment =
      mh::align_sequences({"a", "b", "c", "d"}, {"a", "c", "d", "e"});

  std::size_t last_release = 0;
  std::size_t last_reconstruction = 0;
  bool first = true;
  for (const mh::AlignedPair& pair : alignment.pairs) {
    if (pair.release_index.has_value()) {
      MH_CHECK(first || *pair.release_index > last_release);
      last_release = *pair.release_index;
    }
    if (pair.reconstruction_index.has_value()) {
      MH_CHECK(first || *pair.reconstruction_index > last_reconstruction);
      last_reconstruction = *pair.reconstruction_index;
    }
    first = false;
  }
}

MH_TEST(alignment_of_duplicate_payloads_matches_one_and_reports_the_rest) {
  const SequenceAlignment alignment = mh::align_sequences({"x", "x"}, {"x"});

  MH_CHECK_EQ(alignment.matched, std::int64_t{1});
  MH_CHECK_EQ(alignment.missing, std::int64_t{1});
  MH_CHECK_EQ(alignment.changed, std::int64_t{0});
}

MH_TEST(alignment_of_an_empty_side_reports_everything) {
  const SequenceAlignment empty_release = mh::align_sequences({}, {"a", "b"});
  MH_CHECK_EQ(empty_release.extra, std::int64_t{2});
  MH_CHECK_EQ(empty_release.missing, std::int64_t{0});

  const SequenceAlignment empty_reconstruction = mh::align_sequences({"a", "b"}, {});
  MH_CHECK_EQ(empty_reconstruction.missing, std::int64_t{2});
  MH_CHECK_EQ(empty_reconstruction.extra, std::int64_t{0});

  const SequenceAlignment both_empty = mh::align_sequences({}, {});
  MH_CHECK(both_empty.pairs.empty());
  MH_CHECK(both_empty.clean());
}

MH_TEST(alignment_is_deterministic) {
  const std::vector<std::string> release = {"a", "b", "c", "d", "e"};
  const std::vector<std::string> reconstruction = {"b", "c", "x", "e"};

  MH_CHECK_EQ(describe(mh::align_sequences(release, reconstruction)),
              describe(mh::align_sequences(release, reconstruction)));
}

MH_TEST(min_count_is_enforced_on_both_sides) {
  const std::vector<mh::RequiredKeyCount> required = {
      {"gov|hook|a", 1},
      {"gov|hook|b", 2},
      {"gov|hook|c", 1},
  };
  const std::map<std::string, std::int64_t> release = {
      {"gov|hook|a", 1},
      {"gov|hook|b", 2},
      {"gov|hook|c", 0},
  };
  const std::map<std::string, std::int64_t> reconstruction = {
      {"gov|hook|a", 1},
      {"gov|hook|b", 1},
      {"gov|hook|c", 3},
  };

  const std::vector<mh::MinCountFinding> findings =
      mh::check_min_counts(required, release, reconstruction);

  MH_CHECK_EQ(findings.size(), std::size_t{2});
  // b is short on the reconstruction side, c has zero on the release side: "A required key
  // with zero events is missing, not a vacuous match."
  MH_CHECK_EQ(findings[0].key, std::string{"gov|hook|b"});
  MH_CHECK_EQ(findings[0].release_count, std::int64_t{2});
  MH_CHECK_EQ(findings[0].reconstruction_count, std::int64_t{1});
  MH_CHECK_EQ(findings[1].key, std::string{"gov|hook|c"});
  MH_CHECK_EQ(findings[1].release_count, std::int64_t{0});
}

MH_TEST(min_count_accepts_a_key_absent_from_both_count_maps_only_when_not_required) {
  const std::vector<mh::RequiredKeyCount> required = {{"gov|hook|a", 1}};
  const std::map<std::string, std::int64_t> release;
  const std::map<std::string, std::int64_t> reconstruction;

  const std::vector<mh::MinCountFinding> findings =
      mh::check_min_counts(required, release, reconstruction);
  MH_CHECK_EQ(findings.size(), std::size_t{1});
  MH_CHECK_EQ(findings[0].required, std::int64_t{1});
}

MH_TEST(align_by_key_covers_keys_present_on_only_one_side) {
  const std::map<std::string, std::vector<std::string>> release = {
      {"milestone|level_started", {"a"}},
      {"save|hackpro|read|filesystem", {"s"}},
  };
  const std::map<std::string, std::vector<std::string>> reconstruction = {
      {"milestone|level_started", {"a"}},
  };

  const std::vector<mh::KeyAlignment> alignments =
      mh::align_by_key(release, reconstruction);

  MH_CHECK_EQ(alignments.size(), std::size_t{2});
  // Keys are reported in sorted order, so the output is stable across runs.
  MH_CHECK_EQ(alignments[0].key, std::string{"milestone|level_started"});
  MH_CHECK(alignments[0].alignment.clean());
  MH_CHECK_EQ(alignments[1].key, std::string{"save|hackpro|read|filesystem"});
  MH_CHECK_EQ(alignments[1].alignment.missing, std::int64_t{1});
}

MH_TEST(align_by_key_gives_an_alignment_for_a_key_only_on_the_reconstruction_side) {
  const std::map<std::string, std::vector<std::string>> release;
  const std::map<std::string, std::vector<std::string>> reconstruction = {
      {"milestone|run_end", {"x", "y"}},
  };

  const std::vector<mh::KeyAlignment> alignments =
      mh::align_by_key(release, reconstruction);
  MH_CHECK_EQ(alignments.size(), std::size_t{1});
  MH_CHECK_EQ(alignments[0].alignment.extra, std::int64_t{2});
}
