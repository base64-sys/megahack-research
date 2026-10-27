// Capture window: which frames of a run a trace is allowed to record.
//
// Source: notes/runtime-validation.md A`Timing and stopping` ("`max_frames` is a hard bound")
// and notes/deterministic-capture.md, which records why a bare frame count is not enough.
//
// The observer's `frame` is the engine's own counter, absolute since launch
// (`CCDirector::getTotalFrames`), and `level_started` lands at a different frame on every
// launch: 32,460 and 37,963 in two release runs, 11,541 and 46,872 in two reconstruction runs
// (traces b6fae021, c06b599f, 418de514, 8cd6075c, 2026-10-27). A bound counted from launch
// therefore cuts the level short by however long the operator spent in the menu, and the two
// runs remain incomparable. A bound needs an anchor, and this unit is the anchor.
//
// Why the core and not the observer: the rule decides which records exist, so it has to be
// testable without the game. It holds no platform state and never reads a clock.

#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace mh {

// A window opens at a named milestone and closes after `max_frames` frames counted from the
// frame that milestone carried, so exactly `max_frames` frames are captured.
//
// A default-constructed window declares nothing and admits every record: that is the
// behaviour every capture had before a manifest could declare a window, and it is kept so an
// already-captured scenario does not silently change meaning when it is re-run.
class CaptureWindow {
 public:
  CaptureWindow() = default;
  CaptureWindow(std::string anchor, std::int64_t max_frames);

  bool declared() const noexcept { return declared_; }
  const std::string& anchor() const noexcept { return anchor_; }
  std::int64_t max_frames() const noexcept { return max_frames_; }

  // Whether a record may be written. `is_anchor` marks the record that opens the window (the
  // anchor milestone); `frame` is the record's own frame, which is nullopt before the engine
  // has a director.
  //
  // Returns false for a record before the anchor, for one at or after the bound, and for
  // every record after the window has closed. The caller drops those; nothing here mutates
  // the record.
  bool admit(bool is_anchor, std::optional<std::int64_t> frame) noexcept;

  bool opened() const noexcept { return opened_; }
  // True once the frame bound has stopped the capture. A window that never sees its anchor
  // (the level never started) stays open, so the caller can report an incomplete run rather
  // than claim it finished.
  bool closed() const noexcept { return closed_; }

 private:
  bool declared_ = false;
  std::string anchor_;
  std::int64_t max_frames_ = 0;
  bool opened_ = false;
  bool closed_ = false;
  // -1 when the anchor record carried no frame: the window is then open but unbounded,
  // because a bound with nothing to count from would be a guess.
  std::int64_t anchor_frame_ = -1;
};

} // namespace mh
