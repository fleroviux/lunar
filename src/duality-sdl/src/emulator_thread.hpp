/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <atomic>
#include <core/core.hpp>
#include <util/framelimiter.hpp>
#include <thread>

struct EmulatorThread {
  EmulatorThread(Duality::core::Core& core);

  bool IsRunning() const;
  auto GetFPS() const -> float;
  void SetFastForward(bool enabled);
  void Start();
  void Stop();

private:
  Duality::core::Core& core;
  common::Framelimiter framelimiter;
  std::thread thread;
  std::atomic_bool running = false;
  float fps = 0.0;
};