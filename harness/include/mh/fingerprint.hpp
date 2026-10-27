// run_fingerprint: the deterministic identity used to group runs.
//
// Source: notes/runtime-validation.md §Common fields:
//
//   "run_fingerprint is SHA-256 over canonical JSON (UTF-8, sorted keys, no insignificant
//    whitespace) of:
//    {scenario.sha256, target.sha256, harness.sha256, settings.tags, level.sha256,
//     repetition_index}"
//
// "It is the deterministic identity used to group runs. run_id is deliberately random so
// two files cannot collide even if a runner restarts." Nothing run-specific (run_id,
// timestamps, thread ids, runtime bases) may enter it.

#pragma once

#include <cstdint>
#include <string>

#include "mh/json.hpp"

namespace mh {

struct FingerprintInput {
  std::string scenario_sha256;
  std::string target_sha256;
  std::string harness_sha256;
  std::string level_sha256;
  // The semantic tag list, not the settings file hash: §meta record says
  // "The semantic settings.tags list is the cross-variant identity."
  json::Json settings_tags = json::Json::array();
  std::int64_t repetition_index = 0;
};

// The canonicalised object that is hashed. Exposed so a test (and a reviewer) can see
// exactly what goes into the digest instead of inferring it.
json::Json fingerprint_payload(const FingerprintInput& input);

// Lower-case 64-hex SHA-256 of fingerprint_payload(input).to_canonical().
std::string run_fingerprint(const FingerprintInput& input);

} // namespace mh
