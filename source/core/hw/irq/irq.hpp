/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <common/integer.hpp>

namespace fauxDS::core {

/// ARM7 and ARM9 interrupt controller.
struct IRQ {
  IRQ();

  void Reset();

  /// Interrupt Master Enable
  struct IME {
    auto ReadByte (uint offset) -> u8;
    void WriteByte(uint offset, u8 value);

  private:
    friend class fauxDS::core::IRQ;

    u32 value = 0;
  } ime = {};

  /// Interrupt Enable
  struct IE {
    auto ReadByte (uint offset) -> u8;
    void WriteByte(uint offset, u8 value);
    
  private:
    friend class fauxDS::core::IRQ;

    u32 value = 0;
  } ie = {};

  /// Interrupt Flag / Acknowledge
  struct IF {
    auto ReadByte (uint offset) -> u8;
    void WriteByte(uint offset, u8 value);
    
  private:
    friend class fauxDS::core::IRQ;

    u32 value = 0;
  } _if = {};
};

} // namespace fauxDS::core
