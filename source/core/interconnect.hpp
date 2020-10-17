/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <string.h>

#include "register.hpp"

namespace fauxDS::core {

struct Interconnect {
  Interconnect() { Reset(); }

  void Reset() {
    memset(swram, 0, sizeof(swram));
  }

  struct FakeDISPSTAT : RegisterHalf {
    int vblank_flag = 0;

    auto Read(uint offset) -> u8 override {
      if (offset == 0)
        return (vblank_flag ^= 1);
      return 0;
    }

    void Write(uint offset, u8 value) {}
  } fake_dispstat = {};

  struct KeyInput : RegisterHalf {
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

    auto Read(uint offset) -> u8 override {
      return (a      ? 0 :   1) |
             (b      ? 0 :   2) |
             (select ? 0 :   4) |
             (start  ? 0 :   8) |
             (right  ? 0 :  16) |
             (left   ? 0 :  32) |
             (up     ? 0 :  64) |
             (down   ? 0 : 128) |
             (r      ? 0 : 256) |
             (l      ? 0 : 512);
    }
  } keyinput = {};

  u8 swram[0x8000];
};

} // namespace fauxDS::core
