/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <chrono>
#include <functional>
#include <thread>

namespace lunar {

struct FrameLimiter {
  FrameLimiter(float fps = 60.0) {
    Reset(fps);
  }

  void Reset();
  void Reset(float fps);
  auto GetFastForward() const -> bool;
  void SetFastForward(bool value);

  void Run(
    std::function<void(void)> frame_advance,
    std::function<void(float)> update_fps
  );

private:
  static constexpr int kMillisecondsPerSecond = 1000;
  static constexpr int kMicrosecondsPerSecond = 1000000;

  int frame_count = 0;
  int frame_duration;
  float frames_per_second;
  bool fast_forward = false;

  std::chrono::time_point<std::chrono::steady_clock> timestamp_target;
  std::chrono::time_point<std::chrono::steady_clock> timestamp_fps_update;
};

} // namespace lunar
