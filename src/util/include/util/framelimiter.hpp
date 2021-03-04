/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <chrono>
#include <functional>
#include <thread>

namespace common {

struct Framelimiter {
  Framelimiter(float fps = 60.0) {
    Reset(fps);
  }

  void Reset() {
    Reset(frames_per_second);
  }

  void Reset(float fps) {
    Unbounded(false);
    frames_per_second = fps;
    frame_duration = kMillisecondsPerSecond / fps;
    accumulated_error = 0;
    frame_count = 0;
    timestamp_previous_fps_update = std::chrono::steady_clock::now();
  }

  void Unbounded(bool value) {
    unbounded = value;
  }

  void Run(std::function<void(void)> frame_advance, std::function<void(float)> update_fps) {
    auto timestamp_frame_start = std::chrono::steady_clock::now();

    frame_advance();
    
    auto timestamp_frame_end = std::chrono::steady_clock::now();

    frame_count++;

    auto time_since_last_fps_update = std::chrono::duration_cast<std::chrono::milliseconds>(
      timestamp_frame_end - timestamp_previous_fps_update
    ).count();

    if (time_since_last_fps_update >= kMillisecondsPerSecond) {
      update_fps(frame_count * 1000.0 / time_since_last_fps_update);
      frame_count = 0;
      timestamp_previous_fps_update = timestamp_frame_end;
    }

    if (!unbounded) {
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - timestamp_frame_start
      ).count();

      // NOTE: we need to cast the variables to integers seperately because
      // we don't want the fractional parts to accumulate and overflow into the integer part.
      auto delay = int(frame_duration) + int(accumulated_error) - elapsed;

      std::this_thread::sleep_for(std::chrono::milliseconds(delay));

      accumulated_error -= int(accumulated_error);
      accumulated_error -= std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - timestamp_frame_start
      ).count() - frame_duration;
    }
  }

private:
  static constexpr int kMillisecondsPerSecond = 1000;

  int frame_count = 0;

  float frames_per_second;
  float frame_duration;
  float accumulated_error = 0.0;

  bool unbounded = false;

  std::chrono::time_point<std::chrono::steady_clock> timestamp_previous_fps_update;
};

} // namespace common
