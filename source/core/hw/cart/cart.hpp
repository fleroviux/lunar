/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <common/integer.hpp>
#include <core/hw/irq/irq.hpp>
#include <fstream>
#include <string>

namespace fauxDS::core {

struct Cartridge {
  Cartridge(IRQ& irq7, IRQ& irq9) : irq7(irq7), irq9(irq9) { Reset(); }

  void Reset();
  void Load(std::string const& path);
  auto ReadSPI() -> u8;
  void WriteSPI(u8 value);
  auto ReadROM() -> u32;

  struct AUXSPICNT {
    AUXSPICNT(Cartridge& cart) : cart(cart) {}

    auto ReadByte (uint offset) -> u8;
    void WriteByte(uint offset, u8 value);

  private:
    friend struct fauxDS::core::Cartridge;

    int  baudrate = 0;
    bool chipselect_hold = false;
    bool busy = false;
    bool select_spi = false;
    bool enable_ready_irq = false;
    bool enable_slot = false;

    Cartridge& cart;
  } auxspicnt { *this };

  struct ROMCTRL {
    ROMCTRL(Cartridge& cart) : cart(cart) {}

    auto ReadByte (uint offset) -> u8;
    void WriteByte(uint offset, u8 value);

  private:
    friend struct fauxDS::core::Cartridge;

    // TODO: implement the remaining data fields.
    // http://problemkaputt.de/gbatek.htm#dscartridgeioports
    int data_block_size = 0;

    Cartridge& cart;
  } romctrl { *this };

  struct CARDCMD {
    auto ReadByte (uint offset) -> u8;
    void WriteByte(uint offset, u8 value);

  private:
    friend struct fauxDS::core::Cartridge;

    u8 buffer[8] {0};
  } cardcmd;

private:
  void OnCommandStart();

  // TODO: abstract the cartridge file away.
  bool loaded = false;
  std::fstream file;
  u32 file_mask = 0;

  struct {
    /// Current index into the data buffer (before modulo data_count)
    int index = 0;

    /// Number of requested words
    int count = 0;

    /// Number of available words
    int data_count = 0;

    /// Underlying transfer buffer
    u32 data[0x1000] {0};
  } transfer;

  IRQ& irq7;
  IRQ& irq9;
};

} // namespace fauxDS::core
