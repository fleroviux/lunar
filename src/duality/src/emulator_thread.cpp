/*
 * Copyright (C) 2021 fleroviux
 */

#include <duality/emulator_thread.hpp>

namespace Duality {

EmulatorThread::EmulatorThread(Duality::core::Core& core)
    : core(core) {
  frame_limiter.Reset(59.8983);
}

bool EmulatorThread::IsRunning() const {
  return running;
}

auto EmulatorThread::GetFPS() const -> float {
  return fps;
}

void EmulatorThread::SetFastForward(bool enabled) {
  frame_limiter.SetFastForward(enabled);
}

void EmulatorThread::Start() {
  if (running) {
    return;
  }
  running = true;
  thread = std::thread{[this]() {
    // 355 dots-per-line * 263 lines-per-frame * 6 cycles-per-dot = 560190
    static constexpr int kCyclesPerFrame = 560190;

    frame_limiter.Reset();

    while (running) {
      frame_limiter.Run([this]() {
        core.Run(kCyclesPerFrame);
      }, [this](float fps) {
        this->fps = fps;
      });
    }
  }};
}

void EmulatorThread::Stop() {
  if (!running) {
    return;
  }
  running = false;
  thread.join();
}

} // namespace Duality
