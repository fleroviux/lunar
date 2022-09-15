/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <atomic>
#include <functional>
#include <lunar/core.hpp>
#include <platform/frame_limiter.hpp>
#include <thread> 

namespace lunar {

struct EmulatorThread {
  EmulatorThread(std::unique_ptr<CoreBase>& core);
 ~EmulatorThread();

  bool IsRunning() const;
  bool IsPaused() const;
  void SetPause(bool value);
  bool GetFastForward() const;
  void SetFastForward(bool enabled);
  void SetFrameRateCallback(std::function<void(float)> callback);
  void SetPerFrameCallback(std::function<void()> callback);
  void Start();
  void Stop();

private:
  std::unique_ptr<CoreBase>& core;
  FrameLimiter frame_limiter;
  std::thread thread;
  std::atomic_bool running = false;
  bool paused = false;
  std::function<void(float)> frame_rate_cb = [](float) {};
  std::function<void()> per_frame_cb = []() {};
};

} // namespace lunar