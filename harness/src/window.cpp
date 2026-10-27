// Capture window gating. See include/mh/window.hpp for why this is not in the observer.

#include "mh/window.hpp"

#include <utility>

namespace mh {

CaptureWindow::CaptureWindow(std::string anchor, std::int64_t max_frames)
    : declared_(true), anchor_(std::move(anchor)), max_frames_(max_frames) {}

bool CaptureWindow::admit(bool is_anchor, std::optional<std::int64_t> frame) noexcept {
    if (!declared_) {
        return true;
    }
    if (closed_) {
        return false;
    }
    if (!opened_) {
        if (!is_anchor) {
            return false;
        }
        opened_ = true;
        if (frame.has_value()) {
            anchor_frame_ = *frame;
        }
        return true;
    }
    if (!frame.has_value() || anchor_frame_ < 0) {
        // No counter to compare against: admit, but do not pretend the bound is being held.
        // A window that never learns its anchor frame never closes, and the run is reported
        // as incomplete rather than as bounded.
        return true;
    }
    if (*frame - anchor_frame_ >= max_frames_) {
        closed_ = true;
        return false;
    }
    return true;
}

} // namespace mh
