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
// mods, settings, level, clock) plus repetition_index, plus one runner directive - `timing` -
// which is *removed* before the body is spliced into meta. The meta record's member set is
// fixed by the contract, and a capture bound is an instruction to the observer, not an
// observation about the run.

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "mh/fingerprint.hpp"
#include "mh/json.hpp"
#include "mh/window.hpp"

namespace mh {

struct RunContext {
  // Spliced into meta as-is, minus what the writer generates (schema, type, run_id,
  // run_fingerprint, seq, t_ns, capture) and minus the runner-only `timing` member.
  json::Json body = json::Json::object();
  std::int64_t repetition_index = 0;

  // The capture window the manifest declared, if any. Unset means the capture is unbounded,
  // which is what every scenario without `timing.window` gets; a frame count without an
  // anchor is not carried, because it cannot be enforced meaningfully.
  std::optional<CaptureWindow> window;

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
// non-negative integer. `mods` and `clock` are optional, as in the meta record. `timing` is
// optional and is never part of `body`; when present its `window` must name an anchor
// (`level_started`) and a positive `max_frames`.
ContextResult read_run_context(std::string_view text);

} // namespace mh
