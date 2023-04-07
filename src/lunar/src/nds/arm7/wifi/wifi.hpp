/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <atom/integer.hpp>

namespace lunar::nds {

class WIFI {
  public:
    WIFI() {
      Reset();
    }

    void Reset();
    auto ReadByteIO(u32 address) -> u8;
    void WriteByteIO(u32 address, u8 value);

  private:
    /* Wifi RAM and stubbed IO registers.
     * TODO: figure out which registers precisely need to be stubbed
     * and only stub those.
     */
    u8 mmio[0x42F8]{};

    // Baseband chip internal registers.
    u8 bb_regs[0x69]{};
};

} // namespace lunar::nds
