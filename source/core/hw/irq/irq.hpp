/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <common/integer.hpp>

namespace fauxDS::core {

/// ARM7 and ARM9 interrupt controller.
struct IRQ {
  enum class Source : u32 {
    VBlank = 1 << 0,
    HBlank = 1 << 1,
    VCount = 1 << 2,
    IPC_Sync = 1 << 16,
    IPC_SendEmpty = 1 << 17,
    IPC_ReceiveNotEmpty = 1 << 18,
    SPI = 1 << 23
  };

  IRQ();

  void Reset();
  void Raise(Source source);
  bool IsEnabled();
  bool HasPendingIRQ();

  /// Interrupt Master Enable
  struct IME {
    auto ReadByte (uint offset) -> u8;
    void WriteByte(uint offset, u8 value);

  private:
    friend struct fauxDS::core::IRQ;

    bool enabled = false;
  } ime;

  /// Interrupt Enable
  struct IE {
    auto ReadByte (uint offset) -> u8;
    void WriteByte(uint offset, u8 value);

  private:
    friend struct fauxDS::core::IRQ;

    u32 value = 0;
  } ie;

  /// Interrupt Flag / Acknowledge
  struct IF {
    auto ReadByte (uint offset) -> u8;
    void WriteByte(uint offset, u8 value);

  private:
    friend struct fauxDS::core::IRQ;

    u32 value = 0;
  } _if;
};

} // namespace fauxDS::core
