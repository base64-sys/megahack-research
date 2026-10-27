// Schema and vocabulary constants for the trace contract.
//
// Source: notes/runtime-validation.md, §Trace format ("Every line carries
// "schema": "mh-trace/1"") and §Scenario manifest ("schema": "mh-scenario/1").
// These strings are the contract's, verbatim. A change to the meaning of an existing
// field requires a new schema string, so nothing here is allowed to drift silently.

#pragma once

#include <optional>
#include <string_view>

namespace mh::contract {

// notes/runtime-validation.md §Trace format.
inline constexpr std::string_view kTraceSchema = "mh-trace/1";

// notes/runtime-validation.md §Scenario manifest.
inline constexpr std::string_view kScenarioSchema = "mh-scenario/1";

// Observer identity: meta.environment.harness.{id,version}. The id is also the mod id
// of the observer (notes/runtime-validation.md §meta record, §Scenario manifest).
inline constexpr std::string_view kHarnessId = "megahack.trace";
inline constexpr std::string_view kHarnessVersion = "0.1.1";

// Record types written to one trace file, in order (§Trace format, table of type).
inline constexpr std::string_view kTypeMeta = "meta";
inline constexpr std::string_view kTypeEvent = "event";
inline constexpr std::string_view kTypeEnd = "end";

enum class RecordType { meta, event, end };

// The `type` field of a record (§Trace format). Returns std::nullopt for anything that
// is not one of the three contract strings; the caller decides whether that is invalid
// or forward-compatible.
std::optional<RecordType> parse_record_type(std::string_view text);

// The exact contract spelling of `type`.
std::string_view to_string(RecordType type) noexcept;

} // namespace mh::contract
