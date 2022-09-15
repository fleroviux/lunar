/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "swram.hpp"

namespace lunar::nds {

void SWRAM::WRAMCNT::Reset() {
  WriteByte(3);
}

void SWRAM::WRAMCNT::AddCallback(Callback callback) {
  callbacks.push_back(callback);
}

auto SWRAM::WRAMCNT::ReadByte() -> u8 {
  return value;
}

void SWRAM::WRAMCNT::WriteByte(u8 value) {
  this->value = value & 3;

  switch (this->value) {
    case 0: {
      swram.arm9 = { &swram.data[0], 0x7FFF };
      swram.arm7 = { nullptr, 0 };
      break;
    }
    case 1: {
      swram.arm9 = { &swram.data[0x4000], 0x3FFF };
      swram.arm7 = { &swram.data[0x0000], 0x3FFF };
      break;
    }
    case 2: {
      swram.arm9 = { &swram.data[0x0000], 0x3FFF };
      swram.arm7 = { &swram.data[0x4000], 0x3FFF };
      break;
    }
    case 3: {
      swram.arm9 = { nullptr, 0 };
      swram.arm7 = { &swram.data[0], 0x7FFF };
      break;
    }
  }

  for (auto const& callback : callbacks) callback();
}

} // namespace lunar::nds
