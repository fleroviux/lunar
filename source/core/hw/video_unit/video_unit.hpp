/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <common/integer.hpp>
#include <common/log.hpp>
#include <core/scheduler.hpp>

namespace fauxDS::core {

/// Graphics subsystem which contains two 2D PPUs (A and B) and a 3D GPU.
struct VideoUnit {
  VideoUnit(Scheduler* scheduler);

  void Reset();

  /// Graphics status and IRQ control.
  struct DISPSTAT {
    auto ReadByte(uint offset) -> u8 {
      switch (offset) {
        case 0:
          return (vblank.flag ? 1 : 0) |
                 (hblank.flag ? 2 : 0) |
                 (vcount.flag ? 4 : 0) |
                 (vblank.enable_irq ?  8 : 0) |
                 (hblank.enable_irq ? 16 : 0) |
                 (vcount.enable_irq ? 32 : 0) |
                 ((vcount_setting >> 1) & 128);
        case 1:
          return vcount_setting & 0xFF;
      }

      ASSERT_UNREACHABLE;
    }

    void WriteByte(uint offset, u8 value) {
      switch (offset) {
        case 0:
          vblank.enable_irq = value &  8;
          hblank.enable_irq = value & 16;
          vcount.enable_irq = value & 32;
          vcount_setting = (vcount_setting & 0xFF) | ((value << 1) & 0x100);
          break;
        case 1:
          vcount_setting = (vcount_setting & 0x100) | value;
          break;
      }

      ASSERT_UNREACHABLE;
    }

  private:
    friend struct fauxDS::core::VideoUnit;

    struct {
      bool flag = false;
      bool enable_irq = false;
    } vblank = {}, hblank = {}, vcount = {};

    u16 vcount_setting = 0;
  } dispstat;

  /// Currently rendered scanline.
  /// NOTE: this register reportedly is writable.
  struct VCOUNT {
    auto ReadByte(uint offset) -> u8 {
      switch (offset) {
        case 0:
          return value & 0xFF;
        case 1:
          return (value >> 8) & 1;
      }

      ASSERT_UNREACHABLE;
    }

  private:
    friend struct fauxDS::core::VideoUnit;
    u16 value = 0;
  } vcount;

private:
  Scheduler* scheduler;
  Scheduler::Event* state_event = nullptr;
};

} // namespace fauxDS::core
