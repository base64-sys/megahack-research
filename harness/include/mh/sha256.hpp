// SHA-256 (FIPS 180-4), own implementation: the harness takes no third-party
// dependency, and a hash is not something to take on trust.
//
// Used for run_fingerprint (notes/runtime-validation.md §Common fields) and, later, for
// the trace sidecar hashes in `end.artifacts`.

#pragma once

#include <string>
#include <string_view>

namespace mh {

// Lower-case, 64 hex characters.
std::string sha256_hex(std::string_view bytes);

} // namespace mh
