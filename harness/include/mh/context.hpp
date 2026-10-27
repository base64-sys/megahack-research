// Run context: the facts the runner hands the observer before the game starts.
//
// Source: notes/runtime-validation.md A`meta record` (the members named there are the ones
// this unit requires), A`Common fields` (run_fingerprint is computed from six members, one of
// which - repetition_index - is not part of the meta record itself).
//
// Why a file rather than the runner editing the trace afterwards: a meta record is written
// once, at load, by the harness. Rewriting it after the run would turn a contemporaneous
// record into a reconstruction, and the contract's whole point is to compare what was
// observed. So the runner writes the context *before* launch, the observer splices it into
// meta, and the context file is renamed to `<name>.consumed` so a stale one cannot silently
// apply to a second run.
//
// The context is not a new schema: it is the meta body (target, scenario, environment,
// mods, settings, level, clock) plus repetition_index.

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "mh/fingerprint.hpp"
#include "mh/json.hpp"

namespace mh {

struct RunContext {
  // Spliced into meta as-is, minus what the writer generates (schema, type, run_id,
  // run_fingerprint, seq, t_ns, capture).
  json::Json body = json::Json::object();
  std::int64_t repetition_index = 0;

  // The six members the fingerprint is defined over, read from `body`.
  FingerprintInput fingerprint_input() const;
};

struct ContextResult {
  std::optional<RunContext> context;
  std::string error; // empty when context has a value

  bool ok() const noexcept { return context.has_value(); }
};

// Requires: an object; `target`, `scenario`, `environment`, `settings` and `level` members
// (the same set the trace reader requires of a meta record); `repetition_index` as a
// non-negative integer. `mods` and `clock` are optional, as in the meta record.
ContextResult read_run_context(std::string_view text);

} // namespace mh
