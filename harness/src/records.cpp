#include "mh/records.hpp"

#include <cstdio>
#include <utility>

#include "mh/json_parse.hpp"
#include "mh/value.hpp"

namespace mh::records {
namespace {

json::Json object() { return json::Json::object(); }

// The value model owns its serialisation; parsing it back keeps one spelling of a value
// object in the repository instead of a second, hand-written one here. Moving this into the
// core means the observer and any future emitter cannot disagree about a value object.
json::Json value_json(const Value& value) {
  auto parsed = json::parse(value.to_json());
  if (parsed.value) {
    return *parsed.value;
  }
  return json::Json::null();
}

}  // namespace

std::string rva_text(std::uint32_t rva) {
  char buffer[16];
  std::snprintf(buffer, sizeof(buffer), "0x%X", rva);
  return std::string(buffer);
}

std::string va_text(std::uintptr_t va) {
  char buffer[32];
  std::snprintf(buffer, sizeof(buffer), "0x%llX", static_cast<unsigned long long>(va));
  return std::string(buffer);
}

json::Json hook_target(std::string_view module, std::int64_t priority) {
  json::Json target = object();
  target.set("module", json::Json::string(module));
  target.set("callback_va", json::Json::null());
  target.set("callback_rva", json::Json::null());
  target.set("registration_va", json::Json::null());
  target.set("tag", json::Json::null());
  target.set("tag_label", json::Json::null());
  target.set("order_raw", json::Json::null());
  target.set("order_signed", json::Json::null());
  target.set("priority", json::Json::integer(priority));
  return target;
}

json::Json hook_object(const HookIdentity& identity, json::Json target) {
  json::Json hook = object();
  hook.set("observer", json::Json::string("gd_method"));
  hook.set("key", json::Json::string(identity.key));
  hook.set("module", json::Json::string(identity.module));
  hook.set("method", json::Json::string(identity.method));
  hook.set("resolution", json::Json::string("rva"));
  hook.set("rva", json::Json::string(rva_text(identity.rva)));
  hook.set("va", json::Json::string(va_text(identity.va)));
  hook.set("target", std::move(target));
  hook.set("convention", json::Json::string("unknown"));
  return hook;
}

json::Json call_object(std::string_view call_id, std::int64_t depth,
                       std::optional<std::string_view> parent_call_id, std::uint64_t self,
                       std::optional<std::int64_t> duration_ns) {
  json::Json call = object();
  call.set("call_id", json::Json::string(call_id));
  call.set("depth", json::Json::integer(depth));
  call.set("parent_call_id", parent_call_id.has_value()
                                 ? json::Json::string(*parent_call_id)
                                 : json::Json::null());
  call.set("this", value_json(Value::observed_ptr(self)));
  call.set("args",
           value_json(Value::unavailable("arguments are not captured by this hook")));
  call.set("return", value_json(Value::null_value()));
  if (duration_ns.has_value()) {
    call.set("duration_ns", json::Json::integer(*duration_ns));
  }
  return call;
}

json::Json hook_event(std::string_view phase, const HookIdentity& identity, json::Json target,
                      json::Json call) {
  json::Json body = object();
  body.set("kind", json::Json::string("hook"));
  body.set("phase", json::Json::string(phase));
  body.set("hook", hook_object(identity, std::move(target)));
  body.set("call", std::move(call));
  return body;
}

json::Json milestone_event(std::string_view name, const Value& data) {
  json::Json body = object();
  body.set("kind", json::Json::string("milestone"));
  body.set("name", json::Json::string(name));
  // `data` is a value object, so "nothing observed" is the model's own null value
  // (`{"state":"null"}`), not a JSON null. The captured trace is the authority here: a JSON
  // null would be a second spelling of the same claim.
  body.set("data", value_json(data));
  return body;
}

json::Json milestone_event(std::string_view name) { return milestone_event(name, Value::null_value()); }

json::Json save_event(std::string_view phase, bool exists, std::optional<std::int64_t> size,
                      std::optional<std::string> sha256, std::string local_path) {
  json::Json body = object();
  body.set("kind", json::Json::string("save"));
  body.set("store", json::Json::string("gd_save"));
  body.set("phase", json::Json::string(phase));
  body.set("source", json::Json::string("filesystem"));
  body.set("path_template", json::Json::string("<gd_save>/CCLocalLevels.dat"));
  body.set("exists", json::Json::boolean(exists));
  body.set("size", (exists && size.has_value()) ? json::Json::integer(*size)
                                                : json::Json::null());
  body.set("sha256", (exists && sha256.has_value()) ? json::Json::string(*sha256)
                                                    : json::Json::null());
  body.set("keys", json::Json::null());
  body.set("local_path", json::Json::string(local_path));
  return body;
}

json::Json error_event(std::string_view code, std::string_view message, bool fatal,
                       std::optional<std::string_view> hook_key) {
  json::Json body = object();
  body.set("kind", json::Json::string("error"));
  body.set("code", json::Json::string(code));
  body.set("message", json::Json::string(message));
  body.set("fatal", json::Json::boolean(fatal));
  body.set("hook_key", hook_key.has_value() ? json::Json::string(*hook_key)
                                            : json::Json::null());
  return body;
}

}  // namespace mh::records
