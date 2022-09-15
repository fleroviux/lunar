/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <lunar/log.hpp>
#include <string.h>

#include "wifi.hpp"

namespace lunar::nds {

void WIFI::Reset() {
  memset(mmio, 0, sizeof(mmio));
  memset(bb_regs, 0, sizeof(bb_regs));
}

auto WIFI::ReadByteIO(u32 address) -> u8 {
  // hardcode W_POWERSTATE to break out of an infinite-loop.
  if (address == 0x0480'803D) {
    return 2;
  }

  auto offset = address - 0x0480'4000;
  if (offset >= sizeof(mmio)) {
    UNREACHABLE;
  }
  return mmio[offset];
}

void WIFI::WriteByteIO(u32 address, u8 value) {
  // handle BB register transfer
  if (address == 0x0480'8159) {
    auto direction = value >> 4;
    auto index = mmio[0x4158];
    
    if (index >= sizeof(bb_regs)) {
      return;
    }

    if (direction == 5) {
      // latch W_BB_WRITE into BB register
      bb_regs[index] = mmio[0x415A];
    }

    if (direction == 6) {
      // latch BB register into W_BB_READ
      mmio[0x415C] = bb_regs[index];
    }
  }

  auto offset = address - 0x0480'4000;
  if (offset >= sizeof(mmio)) {
    UNREACHABLE;
  }
  mmio[offset] = value;
}

} // namespace lunar::nds
