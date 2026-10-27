// Run context parsing. See include/mh/context.hpp for why this exists.

#include "mh/context.hpp"

#include <utility>

#include "mh/json_parse.hpp"

namespace mh {
namespace {

const json::Json* member(const json::Json& object, std::string_view key) {
    for (const auto& entry : object.members()) {
        if (entry.first == key) {
            return &entry.second;
        }
    }
    return nullptr;
}

// A digest is only a digest when the contract can read it back: 64 lowercase hex digits.
std::string hex64(const json::Json* node) {
    if (!node || node->kind() != json::Json::Kind::string) {
        return {};
    }
    const std::string_view text = node->string_value();
    if (text.size() != 64) {
        return {};
    }
    for (char c : text) {
        const bool is_hex = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
        if (!is_hex) {
            return {};
        }
    }
    return std::string(text);
}

std::string nested_hex(const json::Json& body, std::string_view object, std::string_view field) {
    const json::Json* node = member(body, object);
    if (!node || !node->is_object()) {
        return {};
    }
    return hex64(member(*node, field));
}

} // namespace

FingerprintInput RunContext::fingerprint_input() const {
    FingerprintInput input;
    input.scenario_sha256 = nested_hex(body, "scenario", "sha256");
    input.target_sha256 = nested_hex(body, "target", "sha256");
    input.level_sha256 = nested_hex(body, "level", "sha256");

    if (const json::Json* environment = member(body, "environment");
        environment && environment->is_object()) {
        if (const json::Json* harness = member(*environment, "harness");
            harness && harness->is_object()) {
            input.harness_sha256 = hex64(member(*harness, "sha256"));
        }
    }

    if (const json::Json* settings = member(body, "settings");
        settings && settings->is_object()) {
        if (const json::Json* tags = member(*settings, "tags"); tags && tags->is_array()) {
            input.settings_tags = *tags;
        }
    }

    input.repetition_index = repetition_index;
    return input;
}

ContextResult read_run_context(std::string_view text) {
    ContextResult result;
    const json::ParseResult parsed = json::parse(text);
    if (!parsed.value) {
        result.error = parsed.error ? parsed.error->message : std::string("invalid JSON");
        return result;
    }
    const json::Json& document = *parsed.value;
    if (!document.is_object()) {
        result.error = "run context is not an object";
        return result;
    }

    for (std::string_view key : {"target", "scenario", "environment", "settings", "level"}) {
        const json::Json* node = member(document, key);
        if (!node || !node->is_object()) {
            result.error =
                "run context member '" + std::string(key) + "' is missing or not an object";
            return result;
        }
    }

    const json::Json* repetition = member(document, "repetition_index");
    if (!repetition || repetition->kind() != json::Json::Kind::integer) {
        result.error = "run context member 'repetition_index' is missing or not an integer";
        return result;
    }
    if (repetition->integer_value() < 0) {
        result.error = "run context member 'repetition_index' is negative";
        return result;
    }

    RunContext context;
    context.body = document;
    context.repetition_index = repetition->integer_value();

    const FingerprintInput input = context.fingerprint_input();
    const std::pair<std::string_view, std::string> digests[] = {
        {"target.sha256", input.target_sha256},
        {"scenario.sha256", input.scenario_sha256},
        {"environment.harness.sha256", input.harness_sha256},
        {"level.sha256", input.level_sha256},
    };
    for (const auto& [name, digest] : digests) {
        if (digest.empty()) {
            result.error = "run context member '" + std::string(name) +
                           "' is missing, not a string, or not 64 lowercase hex digits";
            return result;
        }
    }
    const json::Json* settings = member(document, "settings");
    const json::Json* tags = member(*settings, "tags");
    if (!tags || !tags->is_array()) {
        result.error = "run context member 'settings.tags' is missing or not an array";
        return result;
    }

    result.context = std::move(context);
    return result;
}

} // namespace mh
