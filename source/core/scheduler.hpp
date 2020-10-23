/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <common/integer.hpp>
#include <common/log.hpp>
#include <functional>

namespace fauxDS::core {

class Scheduler {
public:
  Scheduler();
 ~Scheduler();

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
    ASSERT(heap_size != 0, "cannot calculate target when scheduler is empty.");
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

} // namespace fauxDS::core
