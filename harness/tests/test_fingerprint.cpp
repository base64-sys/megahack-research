// Item 3 — run_fingerprint.
//
// Contract: notes/runtime-validation.md §Common fields — "run_fingerprint is SHA-256 over
// canonical JSON (UTF-8, sorted keys, no insignificant whitespace) of:
// {scenario.sha256, target.sha256, harness.sha256, settings.tags, level.sha256,
// repetition_index}". It is the deterministic identity used to group runs, which is why
// run_id (random) is not part of it.

#include <string>

#include "mh/fingerprint.hpp"
#include "mh/sha256.hpp"
#include "test_runner.hpp"

namespace {

mh::FingerprintInput example_input() {
  mh::FingerprintInput input;
  input.scenario_sha256 = std::string(64, '0');
  input.target_sha256 = "1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92";
  input.harness_sha256 = std::string(64, '0');
  input.level_sha256 = std::string(64, '0');
  input.settings_tags = mh::json::Json::array();
  mh::json::Json tag = mh::json::Json::object();
  tag.set("tag", mh::json::Json::string("0x1E2"));
  tag.set("value", mh::json::Json::boolean(true));
  input.settings_tags.push(tag);
  input.repetition_index = 0;
  return input;
}

} // namespace

MH_TEST(fingerprint_payload_is_the_contract_shaped_object) {
  // The six members the contract names, spelled as it spells them, canonicalised. There
  // is no run_id, no scenario id and no settings file hash: those either differ per run
  // or are not part of the identity.
  const std::string zeros(64, '0');
  const std::string expected = R"J({"harness.sha256":")J" + zeros +
                               R"J(","level.sha256":")J" + zeros +
                               R"J(","repetition_index":0,"scenario.sha256":")J" + zeros +
                               R"J(","settings.tags":[{"tag":"0x1E2","value":true}],)J" +
                               R"J("target.sha256":")J" +
                               "1635c3391e853d68eb718965a756fd411ec6a876c23c0b92db508366e1107a92" +
                               R"J("})J";
  MH_CHECK_EQ(mh::fingerprint_payload(example_input()).to_canonical(), expected);
}

MH_TEST(fingerprint_is_sha256_of_the_canonical_payload) {
  const mh::FingerprintInput input = example_input();
  const std::string canonical = mh::fingerprint_payload(input).to_canonical();

  // Internal consistency with the writer...
  MH_CHECK_EQ(mh::run_fingerprint(input), mh::sha256_hex(canonical));
  // ...and an independently computed digest of that canonical text, so the test does not
  // only agree with itself. Digest source: Python `hashlib.sha256` over
  // `json.dumps(..., sort_keys=True, separators=(",", ":"))` of the same object.
  MH_CHECK_EQ(mh::run_fingerprint(input),
              std::string{"c8439d351208d3f96d3a0b404ad1b313158c92f8695e4abf19c6f800134300bc"});
}

MH_TEST(fingerprint_is_stable_for_identical_input) {
  MH_CHECK_EQ(mh::run_fingerprint(example_input()), mh::run_fingerprint(example_input()));
}

MH_TEST(fingerprint_changes_when_any_member_changes) {
  const std::string baseline = mh::run_fingerprint(example_input());

  mh::FingerprintInput other = example_input();
  other.repetition_index = 1;
  MH_CHECK(mh::run_fingerprint(other) != baseline);

  other = example_input();
  other.scenario_sha256 = std::string(64, '1');
  MH_CHECK(mh::run_fingerprint(other) != baseline);

  other = example_input();
  other.target_sha256 = std::string(64, '1');
  MH_CHECK(mh::run_fingerprint(other) != baseline);

  other = example_input();
  other.harness_sha256 = std::string(64, '1');
  MH_CHECK(mh::run_fingerprint(other) != baseline);

  other = example_input();
  other.level_sha256 = std::string(64, '1');
  MH_CHECK(mh::run_fingerprint(other) != baseline);

  other = example_input();
  other.settings_tags = mh::json::Json::array();
  MH_CHECK(mh::run_fingerprint(other) != baseline);
}

MH_TEST(fingerprint_tag_order_matters) {
  // settings.tags is a list, not a set: the semantic settings list has an order and two
  // differently ordered lists are different inputs until the comparator says otherwise.
  mh::FingerprintInput forward = example_input();
  forward.settings_tags = mh::json::Json::array();
  mh::json::Json a = mh::json::Json::object();
  a.set("tag", mh::json::Json::string("0x1E2"));
  mh::json::Json b = mh::json::Json::object();
  b.set("tag", mh::json::Json::string("0x405"));
  forward.settings_tags.push(a);
  forward.settings_tags.push(b);

  mh::FingerprintInput reverse = forward;
  reverse.settings_tags = mh::json::Json::array();
  reverse.settings_tags.push(b);
  reverse.settings_tags.push(a);

  MH_CHECK(mh::run_fingerprint(forward) != mh::run_fingerprint(reverse));
}
