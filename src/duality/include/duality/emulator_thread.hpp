/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <atomic>
#include <core/core.hpp>
#include <duality/frame_limiter.hpp>
#include <thread>

namespace Duality {

struct EmulatorThread {
  EmulatorThread(core::Core& core);

  bool IsRunning() const;
  auto GetFPS() const -> float;
  void SetFastForward(bool enabled);
  void Start();
  void Stop();

private:
  float fps = 0.0;
  core::Core& core;
  FrameLimiter frame_limiter;

  std::thread thread;
  std::atomic_bool running = false;
};

} // namespace Duality
