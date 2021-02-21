/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <util/integer.hpp>

#include "hw/irq/irq.hpp"
#include "scheduler.hpp"

namespace Duality::core {

struct Timer {
  Timer(Scheduler& scheduler, IRQ& irq)
      : scheduler(scheduler), irq(irq) {
    Reset();
  }

  void Reset();
  auto Read (uint chan_id, uint offset) -> u8;
  void Write(uint chan_id, uint offset, u8 value);

private:
  enum Registers {
    REG_TMXCNT_L = 0,
    REG_TMXCNT_H = 2
  };

  struct Channel {
    uint id;
    u16 reload = 0;
    u32 counter = 0;

    struct Control {
      int frequency = 0;
      bool cascade = false;
      bool interrupt = false;
      bool enable = false;
    } control = {};

    bool running = false;
    int shift;
    int mask;
    u64 timestamp_started;
    Scheduler::Event* event = nullptr;
    std::function<void(int)> event_cb;
  } channels[4];

  Scheduler& scheduler;
  IRQ& irq;

  auto GetCounterDeltaSinceLastUpdate(Channel const& channel) -> u32;
  void StartChannel(Channel& channel, int cycles_late);
  void StopChannel(Channel& channel);
  void OnOverflow(Channel& channel);
};

} // namespace Duality::core