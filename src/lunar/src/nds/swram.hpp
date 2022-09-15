/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <array>
#include <functional>
#include <lunar/integer.hpp>

namespace lunar::nds {

struct SWRAM {
  using Callback = std::function<void(void)>;

  void Reset() {
    control.Reset();
    data.fill(0xFF);
  }

  void AddCallback(Callback callback) {
    control.AddCallback(callback);
  }

  std::array<u8, 0x8000> data;

  struct Alloc {
    u8* data = nullptr;
    u32 mask = 0;
  } arm9 = {}, arm7 = {};

  struct WRAMCNT {
    WRAMCNT(SWRAM& swram) : swram(swram) {}

    void Reset();
    void AddCallback(Callback callback);
    auto ReadByte() -> u8;
    void WriteByte(u8 value);

  private:
    int value = 0;
    SWRAM& swram;
    std::vector<Callback> callbacks;
  } control{*this};
};

} // namespace lunar::nds