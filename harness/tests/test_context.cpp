// Run context: what the runner hands the observer before launch (harness/OBSERVER-PROGRESS.md O4b).

#include "mh/context.hpp"

#include "mh/fingerprint.hpp"
#include "test_runner.hpp"

namespace {

constexpr char kDigestA[] = "1111111111111111111111111111111111111111111111111111111111111111";
constexpr char kDigestB[] = "2222222222222222222222222222222222222222222222222222222222222222";
constexpr char kDigestC[] = "3333333333333333333333333333333333333333333333333333333333333333";
constexpr char kDigestD[] = "4444444444444444444444444444444444444444444444444444444444444444";

std::string context_text(std::string_view repetition = "0", std::string_view tags = "[]",
                         std::string_view level_digest = kDigestD,
                         std::string_view timing = "") {
    std::string text = R"({"target":{"role":"release","artifact":"a.dll","sha256":")";
    text += kDigestA;
    text += R"(","size":1,"image_base":"0x180000000"},"scenario":{"id":"s","sha256":")";
    text += kDigestB;
    text += R"("},"environment":{"os":"Windows","arch":"x86_64",)";
    text += R"("gd":{"version":null,"sha256":null},"geode":{"version":"5.11.2","sha256":null},)";
    text += R"("harness":{"id":"megahack.trace","version":"0.1.0","sha256":")";
    text += kDigestC;
    text += R"("}},"settings":{"reset":"defaults","tags":)";
    text += tags;
    text += R"(,"snapshot_sha256":null},"level":{"fixture":"f.gmd","sha256":")";
    text += level_digest;
    text += R"(","source":"generated"},"repetition_index":)";
    text += repetition;
    if (!timing.empty()) {
        text += ",\"timing\":";
        text += timing;
    }
    text += "}";
    return text;
}

bool has_member(const mh::json::Json& body, std::string_view key) {
    for (const auto& entry : body.members()) {
        if (entry.first == key) {
            return true;
        }
    }
    return false;
}

} // namespace

MH_TEST(context_accepts_a_complete_document) {
    const auto result = mh::read_run_context(context_text());
    MH_CHECK(result.ok());
    MH_CHECK_EQ(result.context->repetition_index, std::int64_t{0});
    MH_CHECK(result.context->body.is_object());
}

MH_TEST(context_requires_the_meta_members) {
    for (std::string_view key : {"target", "scenario", "environment", "settings", "level"}) {
        std::string text = context_text();
        // Drop the member by renaming its key, which keeps the rest of the document valid JSON.
        const std::string needle = "\"" + std::string(key) + "\":{";
        const auto position = text.find(needle);
        MH_CHECK(position != std::string::npos);
        text.replace(position, needle.size(), "\"renamed_" + std::string(key) + "\":{");
        const auto result = mh::read_run_context(text);
        MH_CHECK(!result.ok());
        MH_CHECK(result.error.find(key) != std::string::npos);
    }
}

MH_TEST(context_requires_a_non_negative_repetition_index) {
    MH_CHECK(!mh::read_run_context(context_text("-1")).ok());
    MH_CHECK(!mh::read_run_context(context_text("1.5")).ok());
    MH_CHECK(mh::read_run_context(context_text("3")).context->repetition_index == 3);
}

MH_TEST(context_rejects_a_digest_that_is_not_64_lowercase_hex) {
    const std::string short_digest = context_text("0", "[]", "abcd");
    const auto result = mh::read_run_context(short_digest);
    MH_CHECK(!result.ok());
    MH_CHECK(result.error.find("level.sha256") != std::string::npos);

    std::string upper = context_text();
    upper.replace(upper.find(kDigestD), 64, "44444444444444444444444444444444444444444444444444444444444AAAAA");
    MH_CHECK(!mh::read_run_context(upper).ok());
}

MH_TEST(context_rejects_a_non_object_and_broken_json) {
    MH_CHECK(!mh::read_run_context("[]").ok());
    MH_CHECK(!mh::read_run_context("{").ok());
    MH_CHECK(!mh::read_run_context("{\"target\":1}").ok());
}

MH_TEST(context_requires_settings_tags_to_be_an_array) {
    MH_CHECK(!mh::read_run_context(context_text("0", "{}")).ok());
}

MH_TEST(context_fingerprint_input_reads_the_six_members) {
    const auto result = mh::read_run_context(
        context_text("2", R"([{"tag":"0x1E2","label":"HACK_SAFE_MODE","value":true}])"));
    MH_CHECK(result.ok());

    const mh::FingerprintInput input = result.context->fingerprint_input();
    MH_CHECK_EQ(input.scenario_sha256, std::string(kDigestB));
    MH_CHECK_EQ(input.target_sha256, std::string(kDigestA));
    MH_CHECK_EQ(input.harness_sha256, std::string(kDigestC));
    MH_CHECK_EQ(input.level_sha256, std::string(kDigestD));
    MH_CHECK_EQ(input.repetition_index, std::int64_t{2});
    MH_CHECK_EQ(input.settings_tags.items().size(), std::size_t{1});

    // The fingerprint is the core's, not a second implementation: the digest must equal what
    // run_fingerprint computes for the same six members.
    MH_CHECK_EQ(mh::run_fingerprint(input), mh::run_fingerprint(result.context->fingerprint_input()));
}

MH_TEST(context_fingerprint_changes_with_the_repetition_index) {
    const auto first = mh::read_run_context(context_text("0"));
    const auto second = mh::read_run_context(context_text("1"));
    MH_CHECK(first.ok() && second.ok());
    MH_CHECK(mh::run_fingerprint(first.context->fingerprint_input()) !=
             mh::run_fingerprint(second.context->fingerprint_input()));
}

MH_TEST(context_carries_a_declared_capture_window) {
    const auto result = mh::read_run_context(context_text(
        "0", "[]", kDigestD,
        R"({"window":{"anchor":"level_started","max_frames":600},"stop_on":[]})"));
    MH_CHECK(result.ok());
    MH_CHECK(result.context->window.has_value());
    MH_CHECK(result.context->window->declared());
    MH_CHECK_EQ(result.context->window->anchor(), std::string("level_started"));
    MH_CHECK_EQ(result.context->window->max_frames(), std::int64_t{600});

    // A runner directive is not an observation: it must not reach the meta record.
    MH_CHECK(!has_member(result.context->body, "timing"));
    // The six fingerprint inputs are unchanged by it.
    MH_CHECK(result.context->fingerprint_input().level_sha256 == std::string(kDigestD));
}

MH_TEST(context_without_timing_is_unbounded) {
    const auto result = mh::read_run_context(context_text());
    MH_CHECK(result.ok());
    MH_CHECK(!result.context->window.has_value());
}

MH_TEST(context_ignores_a_frame_count_without_an_anchor) {
    // `timing.max_frames` alone is the pre-existing, unenforceable spelling: nothing in the
    // trace says which frame it counts from. It is dropped, not guessed at.
    const auto result = mh::read_run_context(context_text(
        "0", "[]", kDigestD, R"({"max_frames":240,"stop_on":["player_died"]})"));
    MH_CHECK(result.ok());
    MH_CHECK(!result.context->window.has_value());
    MH_CHECK(!has_member(result.context->body, "timing"));
}

MH_TEST(context_rejects_a_malformed_capture_window) {
    const std::string cases[] = {
        R"("window")",
        R"({"window":{"max_frames":600}})",
        R"({"window":{"anchor":"attempt_started","max_frames":600}})",
        R"({"window":{"anchor":1,"max_frames":600}})",
        R"({"window":{"anchor":"level_started"}})",
        R"({"window":{"anchor":"level_started","max_frames":0}})",
        R"({"window":{"anchor":"level_started","max_frames":-5}})",
        R"({"window":{"anchor":"level_started","max_frames":"600"}})",
    };
    for (const std::string& timing : cases) {
        const auto result = mh::read_run_context(context_text("0", "[]", kDigestD, timing));
        MH_CHECK(!result.ok());
        MH_CHECK(result.error.find("timing") != std::string::npos);
    }
}
