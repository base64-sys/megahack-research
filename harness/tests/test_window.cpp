// Capture window: which records a bounded capture may write (notes/deterministic-capture.md).

#include "mh/window.hpp"

#include <cstdint>
#include <optional>
#include <string>

#include "test_runner.hpp"

namespace {

constexpr std::int64_t kAnchorFrame = 32460; // b6fae021's level_started frame

std::optional<std::int64_t> frame(std::int64_t value) { return std::optional<std::int64_t>(value); }

} // namespace

MH_TEST(window_absent_admits_everything) {
    mh::CaptureWindow window;
    MH_CHECK(!window.declared());
    MH_CHECK(!window.closed());
    MH_CHECK(window.admit(false, frame(10)));
    MH_CHECK(window.admit(false, std::nullopt));
    MH_CHECK(!window.opened());
}

MH_TEST(window_drops_records_before_the_anchor) {
    mh::CaptureWindow window("level_started", 240);
    MH_CHECK(window.declared());
    MH_CHECK(window.anchor() == "level_started");
    MH_CHECK_EQ(window.max_frames(), std::int64_t{240});

    // Menu hooks fire long before the level exists; they are outside the run.
    MH_CHECK(!window.admit(false, frame(186)));
    MH_CHECK(!window.opened());
    MH_CHECK(window.admit(true, frame(kAnchorFrame)));
    MH_CHECK(window.opened());
    MH_CHECK(window.admit(false, frame(kAnchorFrame + 1)));
    MH_CHECK(!window.closed());
}

MH_TEST(window_captures_exactly_max_frames) {
    mh::CaptureWindow window("level_started", 4);
    MH_CHECK(window.admit(true, frame(kAnchorFrame)));
    for (std::int64_t offset = 1; offset < 4; ++offset) {
        MH_CHECK(window.admit(false, frame(kAnchorFrame + offset)));
        MH_CHECK(!window.closed());
    }
    // The fifth frame is the first one past the bound, and it closes the window.
    MH_CHECK(!window.admit(false, frame(kAnchorFrame + 4)));
    MH_CHECK(window.closed());
    // Everything after that stays out, even a frame that would otherwise fit.
    MH_CHECK(!window.admit(false, frame(kAnchorFrame + 5)));
    MH_CHECK(!window.admit(false, frame(kAnchorFrame + 1)));
}

MH_TEST(window_that_never_sees_its_anchor_never_closes) {
    mh::CaptureWindow window("level_started", 4);
    MH_CHECK(!window.admit(false, frame(100)));
    MH_CHECK(!window.opened());
    MH_CHECK(!window.closed());
}

MH_TEST(window_without_an_anchor_frame_stays_open) {
    // The anchor milestone written before the engine has a frame counter cannot bound
    // anything: a bound with nothing to count from would be a guess, so the window stays
    // open and the run is reported as incomplete instead.
    mh::CaptureWindow window("level_started", 4);
    MH_CHECK(window.admit(true, std::nullopt));
    MH_CHECK(window.opened());
    MH_CHECK(window.admit(false, std::nullopt));
    MH_CHECK(window.admit(false, frame(1000000)));
    MH_CHECK(!window.closed());
}

MH_TEST(window_records_after_the_bound_with_no_frame_are_dropped) {
    // Once closed, a record without a frame is still outside the capture.
    mh::CaptureWindow window("level_started", 2);
    MH_CHECK(window.admit(true, frame(kAnchorFrame)));
    MH_CHECK(window.admit(false, frame(kAnchorFrame + 1)));
    MH_CHECK(!window.admit(false, frame(kAnchorFrame + 2)));
    MH_CHECK(window.closed());
    MH_CHECK(!window.admit(false, std::nullopt));
}
