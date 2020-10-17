/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <common/integer.hpp>
#include <common/log.hpp>

namespace fauxDS::core {

struct Register {
  virtual auto GetWidth() const -> uint = 0;
  
  virtual auto Read(uint offset) -> u8 {
    LOG_WARN("Register: attempted to read write-only register.");
    return 0;
  }
  
  virtual void Write(uint offset, u8 value) {
    LOG_WARN("Register: attempted to write read-only register.");
  }
};

struct RegisterHalf : Register {
  auto GetWidth() const -> uint final { return sizeof(u16); }
};

struct RegisterWord : Register {
  auto GetWidth() const -> uint final { return sizeof(u32); }
};

struct RegisterSet {
  RegisterSet(uint capacity) : capacity(capacity) {
    views = std::make_unique<View[]>(capacity);
  }

  void Map(uint offset, Register& reg) {
    auto width = reg.GetWidth();

    for (uint i = 0; i < width; i++) {
      ASSERT(offset < capacity, "cannot map register to out-of-bounds offset.");
      ASSERT(views[offset].reg == nullptr ||
             views[offset].reg == &reg, "cannot map two registers to the same location.");
      views[offset++] = { &reg, i };
    }
  }

  void Map(uint offset, RegisterSet const& set) {
    for (uint i = 0; i < set.capacity; i++) {
      if (set.views[i].reg == nullptr)
        continue;
      ASSERT(offset < capacity, "cannot map register to out-of-bounds offset.");
      ASSERT(views[offset].reg == nullptr ||
             views[offset].reg == set.views[i].reg, "cannot map two registers to the same location.");
      views[offset++] = set.views[i];
    }
  }

  auto Read(uint offset) -> u8 {
    if (offset >= capacity) {
      LOG_WARN("RegisterSet: out-of-bounds read to offset 0x{0:08X}", offset);
      return 0;
    }

    auto const& view = views[offset];
    if (view.reg == nullptr) {
      LOG_WARN("RegisterSet: read to unmapped offset 0x{0:08X}", offset);
      return 0;
    }

    return view.reg->Read(view.offset);
  }

  void Write(uint offset, u8 value) {
    if (offset >= capacity) {
      LOG_WARN("RegisterSet: out-of-bounds write to offset 0x{0:08X} = 0x{1:02X}",
        offset, value);
      return;
    }

    auto const& view = views[offset];
    if (view.reg == nullptr) {
      LOG_WARN("RegisterSet: write to unmapped offset 0x{0:08X} = 0x{1:02X}",
        offset, value);
      return;
    }

    view.reg->Write(view.offset, value);
  }

private:
  uint capacity;

  struct View {
    Register* reg = nullptr;
    uint offset = 0;
  };

  std::unique_ptr<View[]> views;
};

} // namespace fauxDS::core
