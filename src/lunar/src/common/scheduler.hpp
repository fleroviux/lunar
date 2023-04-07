/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <atom/integer.hpp>
#include <functional>
#include <limits>

namespace lunar {

struct Scheduler {
  Scheduler();
 ~Scheduler();

  template<class T>
  using EventMethod = void (T::*)(int);

  struct Event {
    std::function<void(int)> callback;
  private:
    friend class Scheduler;
    int handle;
    u64 timestamp;
  };

  auto GetTimestampNow() const -> u64 {
    return timestamp_now;
  }

  auto GetTimestampTarget() const -> u64 {
    if (heap_size == 0) {
      return std::numeric_limits<u64>::max();
    }
    return heap[0]->timestamp;
  }

  auto GetRemainingCycleCount() const -> int {
    return int(GetTimestampTarget() - GetTimestampNow());
  }

  void AddCycles(int cycles) {
    timestamp_now += cycles;
  }

  void Reset();
  void Step();
  auto Add(u64 delay, std::function<void(int)> callback) -> Event*;
  void Cancel(Event* event) { Remove(event->handle); }

  template<class T>
  auto Add(std::uint64_t delay, T* object, EventMethod<T> method) -> Event* {
    return Add(delay, [object, method](int cycles_late) {
      (object->*method)(cycles_late);
    });
  }

private:
  static constexpr int kMaxEvents = 64;

  constexpr int Parent(int n) { return (n - 1) / 2; }
  constexpr int LeftChild(int n) { return n * 2 + 1; }
  constexpr int RightChild(int n) { return n * 2 + 2; }

  void Remove(int n);
  void Swap(int i, int j);
  void Heapify(int n);

  int heap_size;
  Event* heap[kMaxEvents];
  u64 timestamp_now;
};

} // namespace lunar
