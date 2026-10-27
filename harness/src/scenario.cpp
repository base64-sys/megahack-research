#include "mh/scenario.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "mh/contract.hpp"
#include "mh/json_parse.hpp"
#include "mh/sha256.hpp"

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

bool is_object(const json::Json* node) {
  return node != nullptr && node->kind() == json::Json::Kind::object;
}

bool is_array(const json::Json* node) {
  return node != nullptr && node->kind() == json::Json::Kind::array;
}

std::optional<std::string> string_member(const json::Json& object, std::string_view key) {
  const json::Json* node = member(object, key);
  if (node == nullptr || node->kind() != json::Json::Kind::string) {
    return std::nullopt;
  }
  return std::string{node->string_value()};
}

std::optional<std::int64_t> integer_member(const json::Json& object, std::string_view key) {
  const json::Json* node = member(object, key);
  if (node == nullptr || node->kind() != json::Json::Kind::integer) {
    return std::nullopt;
  }
  return node->integer_value();
}

std::optional<double> number_member(const json::Json& object, std::string_view key) {
  const json::Json* node = member(object, key);
  if (node == nullptr) {
    return std::nullopt;
  }
  if (node->kind() == json::Json::Kind::real) {
    return node->real_value();
  }
  if (node->kind() == json::Json::Kind::integer) {
    return static_cast<double>(node->integer_value());
  }
  return std::nullopt;
}

void add_issue(ScenarioDocument& document, ScenarioIssueSeverity severity,
               ScenarioIssueKind kind, std::string path, std::string detail) {
  ScenarioIssue issue;
  issue.severity = severity;
  issue.kind = kind;
  issue.path = std::move(path);
  issue.detail = std::move(detail);
  document.issues.push_back(std::move(issue));
}

bool is_lower_hex_64(std::string_view text) {
  if (text.size() != 64) {
    return false;
  }
  for (const char c : text) {
    const bool digit = c >= '0' && c <= '9';
    const bool lower = c >= 'a' && c <= 'f';
    if (!digit && !lower) {
      return false;
    }
  }
  return true;
}

// "<module>:rva:<lowercase 0x hex RVA>" or "<module>:name:<exact method string>"
bool is_hook_key(std::string_view key) {
  const std::size_t rva_marker = key.find(":rva:0x");
  if (rva_marker != std::string_view::npos) {
    std::string_view digits = key.substr(rva_marker + 7);
    if (rva_marker == 0 || digits.empty()) {
      return false;
    }
    for (const char c : digits) {
      const bool digit = c >= '0' && c <= '9';
      const bool lower = c >= 'a' && c <= 'f';
      if (!digit && !lower) {
        return false;
      }
    }
    return true;
  }
  const std::size_t name_marker = key.find(":name:");
  return name_marker != std::string_view::npos && name_marker > 0 &&
         name_marker + 6 < key.size();
}

bool is_milestone(std::string_view name) {
  // §Expected milestones and hooks: "The minimum vocabulary is level_started,
  // attempt_started, input_sent, player_died, level_completed, save_requested,
  // save_completed, quit_requested, and run_end."
  static constexpr std::string_view kVocabulary[] = {
      "level_started",  "attempt_started", "input_sent",     "player_died", "level_completed",
      "save_requested", "save_completed",  "quit_requested", "run_end",
  };
  for (const std::string_view candidate : kVocabulary) {
    if (candidate == name) {
      return true;
    }
  }
  return false;
}

bool is_settings_tag(std::string_view tag) {
  // The trace's own examples spell tags as upper-case hex: "0x1E2", "0x405".
  if (tag.size() < 3 || tag[0] != '0' || tag[1] != 'x') {
    return false;
  }
  for (std::size_t index = 2; index < tag.size(); ++index) {
    const char c = tag[index];
    const bool digit = c >= '0' && c <= '9';
    const bool upper = c >= 'A' && c <= 'F';
    if (!digit && !upper) {
      return false;
    }
  }
  return true;
}

void validate_hook_list(ScenarioDocument& document, const json::Json& hooks,
                        std::string_view field, std::vector<HookSpec>& out) {
  const json::Json* list = member(hooks, field);
  if (!is_array(list)) {
    add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::missing_key,
              "hooks." + std::string{field}, "expected an array");
    return;
  }
  std::size_t index = 0;
  for (const json::Json& entry : list->items()) {
    const std::string path = "hooks." + std::string{field} + "[" + std::to_string(index) + "]";
    ++index;
    if (!is_object(&entry)) {
      add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::bad_type, path,
                "hook entry must be an object");
      continue;
    }

    HookSpec spec;
    const std::optional<std::string> key = string_member(entry, "key");
    if (!key.has_value() || !is_hook_key(*key)) {
      add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::bad_hook_key, path,
                "hook key must be <module>:rva:<lowercase 0x hex> or <module>:name:<method>");
    } else {
      spec.key = *key;
    }

    const json::Json* phases = member(entry, "phases");
    if (!is_array(phases) || phases->items().empty()) {
      add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::bad_hook_phase,
                path + ".phases", "expected a non-empty array of enter|exit");
    } else {
      bool ok = true;
      for (const json::Json& phase : phases->items()) {
        if (phase.kind() != json::Json::Kind::string ||
            (phase.string_value() != "enter" && phase.string_value() != "exit")) {
          ok = false;
        } else {
          spec.phases.emplace_back(phase.string_value());
        }
      }
      if (!ok) {
        add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::bad_hook_phase,
                  path + ".phases", "phases must be enter and/or exit");
      }
    }

    const std::optional<std::int64_t> min_count = integer_member(entry, "min_count");
    if (!min_count.has_value() || *min_count < 1) {
      add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::bad_type,
                path + ".min_count", "min_count must be an integer >= 1");
    } else {
      spec.min_count = *min_count;
    }

    out.push_back(std::move(spec));
  }
}

void validate_milestone_list(ScenarioDocument& document, const json::Json& milestones,
                             std::string_view field, std::vector<std::string>& out) {
  const json::Json* list = member(milestones, field);
  if (!is_array(list)) {
    add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::missing_key,
              "milestones." + std::string{field}, "expected an array");
    return;
  }
  for (const json::Json& entry : list->items()) {
    if (entry.kind() != json::Json::Kind::string || !is_milestone(entry.string_value())) {
      add_issue(document, ScenarioIssueSeverity::error,
                ScenarioIssueKind::unknown_milestone,
                "milestones." + std::string{field},
                "unknown milestone " +
                    (entry.kind() == json::Json::Kind::string
                         ? std::string{entry.string_value()}
                         : std::string{"<non-string>"}));
      continue;
    }
    out.emplace_back(entry.string_value());
  }
}

} // namespace

std::string_view to_string(ScenarioStatus status) noexcept {
  switch (status) {
    case ScenarioStatus::draft:
      return "draft";
    case ScenarioStatus::approved:
      return "approved";
    case ScenarioStatus::retired:
      return "retired";
  }
  return {};
}

std::optional<ScenarioStatus> parse_scenario_status(std::string_view text) {
  if (text == "draft") {
    return ScenarioStatus::draft;
  }
  if (text == "approved") {
    return ScenarioStatus::approved;
  }
  if (text == "retired") {
    return ScenarioStatus::retired;
  }
  return std::nullopt;
}

std::string_view to_string(ScenarioIssueKind kind) noexcept {
  switch (kind) {
    case ScenarioIssueKind::parse_error:
      return "parse_error";
    case ScenarioIssueKind::missing_key:
      return "missing_key";
    case ScenarioIssueKind::bad_type:
      return "bad_type";
    case ScenarioIssueKind::wrong_schema:
      return "wrong_schema";
    case ScenarioIssueKind::unknown_status:
      return "unknown_status";
    case ScenarioIssueKind::bad_sha256:
      return "bad_sha256";
    case ScenarioIssueKind::bad_level_source:
      return "bad_level_source";
    case ScenarioIssueKind::live_profile:
      return "live_profile";
    case ScenarioIssueKind::bad_driver:
      return "bad_driver";
    case ScenarioIssueKind::bad_action:
      return "bad_action";
    case ScenarioIssueKind::unknown_milestone:
      return "unknown_milestone";
    case ScenarioIssueKind::bad_hook_key:
      return "bad_hook_key";
    case ScenarioIssueKind::bad_hook_phase:
      return "bad_hook_phase";
    case ScenarioIssueKind::bad_tolerance:
      return "bad_tolerance";
    case ScenarioIssueKind::bad_settings_tag:
      return "bad_settings_tag";
    case ScenarioIssueKind::observer_not_listed:
      return "observer_not_listed";
    case ScenarioIssueKind::too_few_repetitions:
      return "too_few_repetitions";
  }
  return {};
}

std::ostream& operator<<(std::ostream& out, ScenarioIssueKind kind) {
  return out << to_string(kind);
}

bool ScenarioDocument::has_errors() const noexcept {
  for (const ScenarioIssue& issue : issues) {
    if (issue.severity == ScenarioIssueSeverity::error) {
      return true;
    }
  }
  return false;
}

bool ScenarioDocument::valid() const noexcept { return scenario.has_value() && !has_errors(); }

bool ScenarioDocument::approved_for_evidence() const noexcept {
  return valid() && scenario->status == ScenarioStatus::approved;
}

ScenarioDocument read_scenario(std::string_view json_text) {
  ScenarioDocument document;
  document.sha256 = sha256_hex(json_text);

  const json::ParseResult parsed = json::parse(json_text);
  if (!parsed.ok()) {
    add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::parse_error, "",
              "manifest: " + parsed.error->message + " at offset " +
                  std::to_string(parsed.error->offset));
    return document;
  }
  if (!parsed.value->is_object()) {
    add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::parse_error, "",
              "manifest must be a JSON object");
    return document;
  }

  const json::Json& root = *parsed.value;
  Scenario scenario;

  const char* const kRequiredTop[] = {
      "schema",          "id",     "status",   "description", "baseline",
      "runtime_baseline", "targets", "harness", "mods",        "level",
      "profile",         "settings", "input",  "timing",      "milestones",
      "hooks",           "repetitions", "comparison",
  };
  for (const char* const key : kRequiredTop) {
    if (member(root, key) == nullptr) {
      add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::missing_key, key,
                "missing required manifest key");
    }
  }

  const std::optional<std::string> schema = string_member(root, "schema");
  if (schema.has_value() && *schema != contract::kScenarioSchema) {
    add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::wrong_schema,
              "schema", "expected " + std::string{contract::kScenarioSchema});
  }

  scenario.id = string_member(root, "id").value_or("");
  if (scenario.id.empty()) {
    add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::missing_key, "id",
              "id must be a non-empty string");
  }
  scenario.description = string_member(root, "description").value_or("");

  const std::optional<std::string> status = string_member(root, "status");
  const std::optional<ScenarioStatus> parsed_status =
      status.has_value() ? parse_scenario_status(*status) : std::nullopt;
  if (!parsed_status.has_value()) {
    add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::unknown_status,
              "status", "status must be draft, approved or retired");
  } else {
    scenario.status = *parsed_status;
  }

  // -- level ---------------------------------------------------------------
  const json::Json* level = member(root, "level");
  if (is_object(level)) {
    scenario.level_fixture = string_member(*level, "fixture").value_or("");
    scenario.level_sha256 = string_member(*level, "sha256").value_or("");
    scenario.level_source = string_member(*level, "source").value_or("");

    if (!is_lower_hex_64(scenario.level_sha256)) {
      add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::bad_sha256,
                "level.sha256", "expected 64 lower-case hex characters");
    }
    // "A user-created level is not allowed, even if hashed. A downloaded level is not
    // allowed."
    if (scenario.level_source != "generated" && scenario.level_source != "local-fixture") {
      add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::bad_level_source,
                "level.source",
                "source must be generated or local-fixture (user-level is not allowed)");
    }
    if (scenario.level_fixture.empty()) {
      add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::missing_key,
                "level.fixture", "fixture must name a local fixture");
    }
  }

  // -- profile -------------------------------------------------------------
  const json::Json* profile = member(root, "profile");
  if (is_object(profile)) {
    scenario.profile_isolation = string_member(*profile, "isolation").value_or("");
    if (scenario.profile_isolation.empty()) {
      add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::missing_key,
                "profile.isolation", "the runner must record profile isolation");
    } else if (scenario.profile_isolation != "disposable") {
      // "a run against a live profile is not eligible for M3 evidence."
      add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::live_profile,
                "profile.isolation", "only a disposable profile is eligible for M3 evidence");
    }
  }

  // -- harness and targets -------------------------------------------------
  const json::Json* harness = member(root, "harness");
  if (is_object(harness)) {
    scenario.harness_id = string_member(*harness, "id").value_or("");
    scenario.harness_version = string_member(*harness, "version").value_or("");
    scenario.harness_sha256 = string_member(*harness, "sha256").value_or("");
    if (!is_lower_hex_64(scenario.harness_sha256)) {
      add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::bad_sha256,
                "harness.sha256", "expected 64 lower-case hex characters");
    }
  }

  const json::Json* targets = member(root, "targets");
  if (is_object(targets)) {
    const json::Json* release = member(*targets, "release");
    const json::Json* reconstruction = member(*targets, "reconstruction");
    if (!is_object(release) || !is_object(reconstruction)) {
      add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::missing_key,
                "targets", "release and reconstruction targets are both required");
    } else {
      scenario.release_artifact = string_member(*release, "artifact").value_or("");
      scenario.release_sha256 = string_member(*release, "sha256").value_or("");
      scenario.reconstruction_artifact = string_member(*reconstruction, "artifact").value_or("");
      scenario.reconstruction_sha256 =
          string_member(*reconstruction, "sha256").value_or("");
      if (!is_lower_hex_64(scenario.release_sha256)) {
        add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::bad_sha256,
                  "targets.release.sha256", "expected 64 lower-case hex characters");
      }
      if (!is_lower_hex_64(scenario.reconstruction_sha256)) {
        add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::bad_sha256,
                  "targets.reconstruction.sha256", "expected 64 lower-case hex characters");
      }
    }
  }

  // -- mods ----------------------------------------------------------------
  const json::Json* mods = member(root, "mods");
  if (is_object(mods)) {
    const json::Json* required = member(*mods, "required");
    if (!is_array(required)) {
      add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::missing_key,
                "mods.required", "expected an array of mod ids");
    } else {
      for (const json::Json& entry : required->items()) {
        if (entry.kind() == json::Json::Kind::string) {
          scenario.required_mods.emplace_back(entry.string_value());
        }
      }
      // "mods.required lists the target and observer."
      const std::string observer = scenario.harness_id;
      bool listed = false;
      for (const std::string& id : scenario.required_mods) {
        if (id == observer) {
          listed = true;
        }
      }
      if (!listed) {
        add_issue(document, ScenarioIssueSeverity::error,
                  ScenarioIssueKind::observer_not_listed, "mods.required",
                  "the observer mod must be listed as required");
      }
    }

    const std::optional<std::string> other_mods = string_member(*mods, "other_mods");
    if (!other_mods.has_value() || (*other_mods != "forbidden" && *other_mods != "allowed")) {
      add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::bad_type,
                "mods.other_mods", "other_mods must be forbidden or allowed");
    }
  }

  // -- settings ------------------------------------------------------------
  const json::Json* settings = member(root, "settings");
  if (is_object(settings)) {
    const json::Json* tags = member(*settings, "tags");
    if (!is_array(tags)) {
      add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::missing_key,
                "settings.tags", "expected an array");
    } else {
      for (const json::Json& tag : tags->items()) {
        if (!is_object(&tag)) {
          add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::bad_settings_tag,
                    "settings.tags", "tag entry must be an object");
          continue;
        }
        const std::optional<std::string> name = string_member(tag, "tag");
        if (!name.has_value() || !is_settings_tag(*name)) {
          add_issue(document, ScenarioIssueSeverity::error,
                    ScenarioIssueKind::bad_settings_tag, "settings.tags",
                    "tag must be an upper-case hex string such as 0x1E2");
        }
        const json::Json* value = member(tag, "value");
        if (value == nullptr ||
            (value->kind() != json::Json::Kind::boolean &&
             value->kind() != json::Json::Kind::integer)) {
          add_issue(document, ScenarioIssueSeverity::error,
                    ScenarioIssueKind::bad_settings_tag, "settings.tags.value",
                    "value must be a boolean or an integer");
        }
      }
      scenario.settings_tags = *tags;
    }
  }

  // -- input ---------------------------------------------------------------
  const json::Json* input = member(root, "input");
  if (is_object(input)) {
    const std::optional<std::string> driver = string_member(*input, "driver");
    if (!driver.has_value() || (*driver != "harness" && *driver != "os")) {
      add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::bad_driver,
                "input.driver", "driver must be harness or os");
    }
    const json::Json* actions = member(*input, "actions");
    if (!is_array(actions)) {
      add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::missing_key,
                "input.actions", "expected an array");
    } else {
      std::size_t index = 0;
      for (const json::Json& action : actions->items()) {
        const std::string path = "input.actions[" + std::to_string(index) + "]";
        ++index;
        if (!is_object(&action)) {
          add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::bad_action,
                    path, "action must be an object");
          continue;
        }
        const std::optional<std::int64_t> at_frame = integer_member(action, "at_frame");
        if (!at_frame.has_value() || *at_frame < 0) {
          add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::bad_action,
                    path + ".at_frame", "at_frame must be an integer >= 0");
        }
        const std::optional<std::string> name = string_member(action, "action");
        if (!name.has_value() ||
            (*name != "press" && *name != "release" && *name != "quit" && *name != "wait")) {
          add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::bad_action,
                    path + ".action", "action must be press, release, quit or wait");
        }
        const json::Json* key = member(action, "key");
        if (key == nullptr) {
          add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::bad_action,
                    path + ".key", "key must be space, mouse_left or null");
        } else if (key->kind() == json::Json::Kind::string) {
          if (key->string_value() != "space" && key->string_value() != "mouse_left") {
            add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::bad_action,
                      path + ".key", "key must be space or mouse_left");
          }
        } else if (key->kind() != json::Json::Kind::null) {
          add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::bad_action,
                    path + ".key", "key must be space, mouse_left or null");
        }
        if (name.has_value() && *name == "quit" && key->kind() != json::Json::Kind::null) {
          add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::bad_action,
                    path + ".key", "quit takes a null key");
        }
      }
    }
  }

  // -- timing --------------------------------------------------------------
  const json::Json* timing = member(root, "timing");
  if (is_object(timing)) {
    const std::optional<std::int64_t> max_frames = integer_member(*timing, "max_frames");
    if (!max_frames.has_value() || *max_frames <= 0) {
      add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::bad_type,
                "timing.max_frames", "max_frames must be a positive integer");
    }
    const std::optional<std::int64_t> timeout = integer_member(*timing, "timeout_ms");
    if (!timeout.has_value() || *timeout <= 0) {
      add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::bad_type,
                "timing.timeout_ms", "timeout_ms must be a positive integer");
    }
    const std::optional<std::int64_t> fps = integer_member(*timing, "fps_cap");
    if (!fps.has_value() || *fps <= 0) {
      add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::bad_type,
                "timing.fps_cap", "fps_cap must be a positive integer");
    }
  }

  // -- milestones and hooks ------------------------------------------------
  const json::Json* milestones = member(root, "milestones");
  if (is_object(milestones)) {
    validate_milestone_list(document, *milestones, "required", scenario.required_milestones);
    std::vector<std::string> optional;
    validate_milestone_list(document, *milestones, "optional", optional);
  }

  const json::Json* hooks = member(root, "hooks");
  if (is_object(hooks)) {
    validate_hook_list(document, *hooks, "required", scenario.required_hooks);
    validate_hook_list(document, *hooks, "optional", scenario.optional_hooks);
  }

  // -- repetitions ---------------------------------------------------------
  const std::optional<std::int64_t> repetitions = integer_member(root, "repetitions");
  if (!repetitions.has_value() || *repetitions < 1) {
    add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::bad_type,
              "repetitions", "repetitions must be an integer >= 1");
  } else {
    scenario.repetitions = *repetitions;
    if (*repetitions < 2) {
      // §Comparability gate: "At least two runs per variant".
      add_issue(document, ScenarioIssueSeverity::warning,
                ScenarioIssueKind::too_few_repetitions, "repetitions",
                "the comparability gate needs at least two runs per variant");
    }
  }

  // -- comparison ----------------------------------------------------------
  const json::Json* comparison = member(root, "comparison");
  if (is_object(comparison)) {
    const json::Json* groups = member(*comparison, "order_groups");
    if (!is_array(groups)) {
      add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::missing_key,
                "comparison.order_groups", "expected an array of arrays");
    } else {
      for (const json::Json& group : groups->items()) {
        if (!is_array(&group)) {
          add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::bad_type,
                    "comparison.order_groups", "each group must be an array");
          continue;
        }
        std::vector<std::string> names;
        for (const json::Json& name : group.items()) {
          if (name.kind() != json::Json::Kind::string) {
            add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::bad_type,
                      "comparison.order_groups", "group entries must be strings");
            continue;
          }
          // A group names milestones and hook-enter keys; anything else cannot be ordered.
          if (!is_milestone(name.string_value()) && !is_hook_key(name.string_value())) {
            add_issue(document, ScenarioIssueSeverity::error,
                      ScenarioIssueKind::unknown_milestone, "comparison.order_groups",
                      "unknown event name " + std::string{name.string_value()});
            continue;
          }
          names.emplace_back(name.string_value());
        }
        scenario.order_groups.push_back(std::move(names));
      }
    }

    const json::Json* tolerances = member(*comparison, "tolerances");
    if (!is_object(tolerances)) {
      add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::missing_key,
                "comparison.tolerances", "expected an object keyed by JSON Pointer");
    } else {
      for (const auto& entry : tolerances->members()) {
        const std::string path = "comparison.tolerances" + entry.first;
        if (entry.first.empty() || entry.first[0] != '/') {
          add_issue(document, ScenarioIssueSeverity::error,
                    ScenarioIssueKind::bad_tolerance, path,
                    "tolerance key must be a JSON Pointer starting with '/'");
        }
        if (!is_object(&entry.second)) {
          add_issue(document, ScenarioIssueSeverity::error,
                    ScenarioIssueKind::bad_tolerance, path, "tolerance must be an object");
          continue;
        }
        Tolerance tolerance;
        const std::optional<double> abs = number_member(entry.second, "abs");
        const std::optional<double> rel = number_member(entry.second, "rel");
        if (abs.has_value()) {
          tolerance.abs_tolerance = *abs;
        }
        if (rel.has_value()) {
          tolerance.rel_tolerance = *rel;
        }
        if (!abs.has_value() && !rel.has_value()) {
          add_issue(document, ScenarioIssueSeverity::error,
                    ScenarioIssueKind::bad_tolerance, path,
                    "tolerance needs abs and/or rel");
        }
        const std::optional<std::string> why = string_member(entry.second, "why");
        if (!why.has_value() || why->empty()) {
          // "A tolerance without a `why` is invalid."
          add_issue(document, ScenarioIssueSeverity::error,
                    ScenarioIssueKind::bad_tolerance, path, "tolerance needs a why");
        } else {
          tolerance.why = *why;
        }
        scenario.tolerances.emplace_back(entry.first, tolerance);
      }
    }

    const json::Json* volatile_paths = member(*comparison, "volatile");
    if (!is_array(volatile_paths)) {
      add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::missing_key,
                "comparison.volatile", "expected an array of field paths");
    } else {
      for (const json::Json& path : volatile_paths->items()) {
        if (path.kind() != json::Json::Kind::string) {
          add_issue(document, ScenarioIssueSeverity::error, ScenarioIssueKind::bad_type,
                    "comparison.volatile", "entries must be strings");
          continue;
        }
        scenario.volatile_paths.emplace_back(path.string_value());
      }
    }
  }

  document.scenario = std::move(scenario);
  return document;
}

} // namespace mh
