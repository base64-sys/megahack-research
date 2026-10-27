#include "mh/fingerprint.hpp"

#include <string>

#include "mh/sha256.hpp"

namespace mh {

json::Json fingerprint_payload(const FingerprintInput& input) {
  // The six members named by the contract, spelled as the contract spells them. Missing
  // members are not filled in with an empty string: an empty hash would silently group
  // runs that were never hashed.
  json::Json payload = json::Json::object();
  payload.set("scenario.sha256", json::Json::string(input.scenario_sha256));
  payload.set("target.sha256", json::Json::string(input.target_sha256));
  payload.set("harness.sha256", json::Json::string(input.harness_sha256));
  payload.set("settings.tags", input.settings_tags);
  payload.set("level.sha256", json::Json::string(input.level_sha256));
  payload.set("repetition_index", json::Json::integer(input.repetition_index));
  return payload;
}

std::string run_fingerprint(const FingerprintInput& input) {
  return sha256_hex(fingerprint_payload(input).to_canonical());
}

} // namespace mh
