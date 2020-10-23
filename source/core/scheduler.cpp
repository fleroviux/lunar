/*
 * Copyright (C) 2020 fleroviux
 */

#include "scheduler.hpp"

namespace fauxDS::core {

Scheduler::Scheduler() {
  for (int i = 0; i < kMaxEvents; i++) {
    heap[i] = new Event();
    heap[i]->handle = i;
  }
  Reset();
}

Scheduler::~Scheduler() {
  for (int i = 0; i < kMaxEvents; i++) {
    delete heap[i];
  }
}

void Scheduler::Reset() {
  heap_size = 0;
  timestamp_now = 0;
}

void Scheduler::Step() {
  auto now = GetTimestampNow();
  while (heap_size > 0 && heap[0]->timestamp <= now) {
    auto event = heap[0];
    event->callback(int(now - event->timestamp));
    // NOTE: we cannot just pass zero because the callback may mess with the event queue.
    Remove(event->handle);
  }
}

auto Scheduler::Add(u64 delay, std::function<void(int)> callback) -> Event* {
  int n = heap_size++;
  int p = Parent(n);

  ASSERT(heap_size <= kMaxEvents, "exceeded maximum number of scheduler events.");

  auto event = heap[n];
  event->timestamp = GetTimestampNow() + delay;
  event->callback = callback;

  while (n != 0 && heap[p]->timestamp > heap[n]->timestamp) {
    Swap(n, p);
    n = p;
    p = Parent(n);
  }

  return event;
}

void Scheduler::Remove(int n) {
  Swap(n, --heap_size);

  int p = Parent(n);
  if (n != 0 && heap[p]->timestamp > heap[n]->timestamp) {
    do {
      Swap(n, p);
      n = p;
      p = Parent(n);
    } while (n != 0 && heap[p]->timestamp > heap[n]->timestamp);
  } else {
    Heapify(n);
  }
}

void Scheduler::Swap(int i, int j) {
  auto tmp = heap[i];
  heap[i] = heap[j];
  heap[j] = tmp;
  heap[i]->handle = i;
  heap[j]->handle = j;
}

void Scheduler::Heapify(int n) {
  int l = LeftChild(n);
  int r = RightChild(n);

  if (l < heap_size && heap[l]->timestamp < heap[n]->timestamp) {
    Swap(l, n);
    Heapify(l);
  }

  if (r < heap_size && heap[r]->timestamp < heap[n]->timestamp) {
    Swap(r, n);
    Heapify(r);
  }
}

} // namespace fauxDS::core
