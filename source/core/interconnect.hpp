/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <common/integer.hpp>
#include <common/log.hpp>
#include <core/hw/video_unit/video_unit.hpp>
#include <string.h>

#include "scheduler.hpp"

namespace fauxDS::core {

struct Interconnect {
  Interconnect() : video_unit(&scheduler) { Reset(); }

  void Reset() {
    memset(ewram, 0, sizeof(ewram));
    memset(swram.data, 0, sizeof(swram));
    video_unit.Reset();
    // For direct boot only - map all of SWRAM to the ARM7.
    //wramcnt.Write(0, 3);
  }

  u8 ewram[0x400000];

  struct SWRAM {
    u8 data[0x8000];

    struct Alloc {
      u8* data = nullptr;
      u32 mask = 0;
    } arm9 = {}, arm7 = {};
  } swram;

  Scheduler scheduler;
  VideoUnit video_unit;

  /*struct WRAMCNT : RegisterByte {
    WRAMCNT(SWRAM& swram) : swram(swram) {}

    auto Read(uint offset) -> u8 override {
      return value;
    }

    void Write(uint offset, u8 value) override {
      this->value = value & 3;
      switch (this->value) {
        case 0:
          swram.arm9 = { swram.data, 0x7FFF };
          swram.arm7 = { nullptr, 0 };
          break;
        case 1:
          swram.arm9 = { &swram.data[0x4000], 0x3FFF };
          swram.arm7 = { &swram.data[0x0000], 0x3FFF };
          break;
        case 2:
          swram.arm9 = { &swram.data[0x0000], 0x3FFF };
          swram.arm7 = { &swram.data[0x4000], 0x3FFF };
          break;
        case 3:
          swram.arm9 = { nullptr, 0 };
          swram.arm7 = { swram.data, 0x7FFF };
          break;
      }
    }

  private:
    int value = 0;
    SWRAM& swram;
  } wramcnt;*/

  struct KeyInput {
    bool a = false;
    bool b = false;
    bool select = false;
    bool start = false;
    bool right = false;
    bool left = false;
    bool up = false;
    bool down = false;
    bool r = false;
    bool l = false;

    auto ReadByte(uint offset) -> u8 {
      switch (offset) {
        case 0:
          return (a      ? 0 :   1) |
                 (b      ? 0 :   2) |
                 (select ? 0 :   4) |
                 (start  ? 0 :   8) |
                 (right  ? 0 :  16) |
                 (left   ? 0 :  32) |
                 (up     ? 0 :  64) |
                 (down   ? 0 : 128);
        case 1:
          return (r ? 0 : 1) |
                 (l ? 0 : 2);
      }

      UNREACHABLE;
    }
  } keyinput = {};
};

} // namespace fauxDS::core
