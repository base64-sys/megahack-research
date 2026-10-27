// Pure record builders: values in, `json::Json` out.
//
// Why this unit exists (notes/trace-observer.md §What still blocks a comparison): the
// observer and the reconstruction are two mods that must emit the *same* record shapes.
// Two copies of a builder with one drifted key name would make a comparison lie without
// failing anything, so the shapes live here - in the portable core, where ctest pins them
// byte-for-byte against a captured line - and a mod only supplies the values.
//
// What is deliberately NOT here: anything that touches a platform, a filesystem, a mod or a
// clock. The caller resolves the address, reads the file, and takes the timestamp; this unit
// only decides what the record looks like.
//
// Envelope keys (`schema`, `type`, `run_id`, `run_fingerprint`, `seq`, `t_ns`, `frame`,
// `thread`, `capture`) belong to `TraceWriter` and are never produced here.

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "mh/json.hpp"

namespace mh::records {

// One hook site as the trace spells it. `va` is the resolved runtime address; `rva` is kept
// because the contract's key is RVA-based and a runtime address alone is not identity.
struct HookIdentity {
  std::string key;     // "GeometryDash.exe:rva:0x397f40"
  std::string method;  // "PlayerObject::pushButton"
  std::string module;  // "GeometryDash.exe"
  std::uint32_t rva = 0;
  std::uintptr_t va = 0;
};

// `"0x397F40"` and `"0x7FF6762E7F40"`: upper-case, no padding, as the captured records spell
// them.
std::string rva_text(std::uint32_t rva);
std::string va_text(std::uintptr_t va);

// `hook.target`: the implementation that owns a hook at this address. Fields Geode does not
// expose stay null rather than being invented; only `module` and `priority` are supplied.
json::Json hook_target(std::string_view module, std::int64_t priority);

// The whole `hook` object with the nine keys in the captured order.
json::Json hook_object(const HookIdentity& identity, json::Json target);

// `call`: identity, nesting, the receiver, and an honest `args`/`return`. `duration_ns` is
// only written when the call has finished, so an `enter` does not claim a duration.
json::Json call_object(std::string_view call_id, std::int64_t depth,
                       std::optional<std::string_view> parent_call_id, std::uint64_t self,
                       std::optional<std::int64_t> duration_ns = std::nullopt);

json::Json hook_event(std::string_view phase, const HookIdentity& identity, json::Json target,
                      json::Json call);

json::Json milestone_event(std::string_view name);

// A `save` record is an observed file state, never an inference: `exists` decides whether
// `size`/`sha256` are numbers or null.
json::Json save_event(std::string_view phase, bool exists, std::optional<std::int64_t> size,
                      std::optional<std::string> sha256, std::string local_path);

json::Json error_event(std::string_view code, std::string_view message, bool fatal,
                       std::optional<std::string_view> hook_key);

}  // namespace mh::records
