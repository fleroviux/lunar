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
    frame_count = 0;
    frame_duration = int(kMicrosecondsPerSecond / fps);
    frames_per_second = fps;
    unbounded = false;
    timestamp_target = std::chrono::steady_clock::now();
    timestamp_fps_update = std::chrono::steady_clock::now();
  }

  void Unbounded(bool value) {
    if (unbounded != value) {
      unbounded = value;
      if (!value) {
        timestamp_target = std::chrono::steady_clock::now();
      }
    }
  }

  void Run(std::function<void(void)> frame_advance, std::function<void(float)> update_fps) {
    timestamp_target += std::chrono::microseconds(frame_duration);

    frame_advance();
    frame_count++;
    
    auto now = std::chrono::steady_clock::now(); 
    auto fps_update_delta = std::chrono::duration_cast<std::chrono::milliseconds>(
      now - timestamp_fps_update).count();

    if (fps_update_delta >= kMillisecondsPerSecond) {
      update_fps(frame_count * float(kMillisecondsPerSecond) / fps_update_delta);
      frame_count = 0;
      timestamp_fps_update = std::chrono::steady_clock::now();
    }

    if (!unbounded) {
      std::this_thread::sleep_until(timestamp_target);
    }
  }

private:
  static constexpr int kMillisecondsPerSecond = 1000;
  static constexpr int kMicrosecondsPerSecond = 1000000;

  int frame_count = 0;
  int frame_duration;
  float frames_per_second;
  bool unbounded = false;

  std::chrono::time_point<std::chrono::steady_clock> timestamp_target;
  std::chrono::time_point<std::chrono::steady_clock> timestamp_fps_update;
};

} // namespace common
