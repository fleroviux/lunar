/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <util/integer.hpp>
#include <fstream>
#include <string>

#include "backup.hpp"
#include "hw/dma/dma7.hpp"
#include "hw/dma/dma9.hpp"
#include "hw/irq/irq.hpp"

namespace Duality::core {

struct Cartridge {
  Cartridge(IRQ& irq7, IRQ& irq9, DMA7& dma7, DMA9& dma9)
      : irq7(irq7)
      , irq9(irq9)
      , dma7(dma7)
      , dma9(dma9) {
    Reset();
  }

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
    friend struct Duality::core::Cartridge;

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
    friend struct Duality::core::Cartridge;

    // TODO: implement the remaining data fields.
    // http://problemkaputt.de/gbatek.htm#dscartridgeioports
    int data_block_size = 0;

    Cartridge& cart;
  } romctrl { *this };

  struct CARDCMD {
    auto ReadByte (uint offset) -> u8;
    void WriteByte(uint offset, u8 value);

  private:
    friend struct Duality::core::Cartridge;

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

  /// SPI backup data register
  u8 spidata = 0;

  DMA7& dma7;
  DMA9& dma9;
  IRQ& irq7;
  IRQ& irq9;
  Backup backup;
};

} // namespace Duality::core
