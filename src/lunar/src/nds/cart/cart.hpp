
// Copyright (C) 2022 fleroviux

#pragma once

#include <algorithm>
#include <lunar/integer.hpp>
#include <fstream>
#include <memory>
#include <string>

#include "common/scheduler.hpp"
#include "nds/arm7/dma/dma.hpp"
#include "nds/arm7/spi/spi_device.hpp"
#include "nds/arm9/dma/dma.hpp"
#include "nds/irq/irq.hpp"
#include "nds/exmemcnt.hpp"

namespace lunar::nds {

struct Cartridge {
  Cartridge(
    Scheduler& scheduler,
    IRQ& irq7,
    IRQ& irq9,
    DMA7& dma7,
    DMA9& dma9,
    EXMEMCNT& exmemcnt
  )   : scheduler(scheduler)
      , irq7(irq7)
      , irq9(irq9)
      , dma7(dma7)
      , dma9(dma9)
      , exmemcnt(exmemcnt) {
    Reset();
  }

  void Reset();
  void Load(std::string const& path, bool direct_boot);
  auto ReadSPI() -> u8;
  void WriteSPI(u8 value);
  auto ReadROM() -> u32;

  struct AUXSPICNT {
    AUXSPICNT(Cartridge& cart) : cart(cart) {}

    auto ReadByte (uint offset) -> u8;
    void WriteByte(uint offset, u8 value);

  private:
    friend struct lunar::nds::Cartridge;

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
    friend struct lunar::nds::Cartridge;

    // TODO: implement the remaining data fields.
    // http://problemkaputt.de/gbatek.htm#dscartridgeioports
    int  transfer_clock_rate = 0;
    bool data_ready = false;
    int  data_block_size = 0;
    bool busy = false;

    Cartridge& cart;
  } romctrl { *this };

  struct CARDCMD {
    auto ReadByte (uint offset) -> u8;
    void WriteByte(uint offset, u8 value);

  private:
    friend struct lunar::nds::Cartridge;

    u8 buffer[8] {0};
  } cardcmd;

private:
  // TODO: use a plausible chip ID based on the ROM size.
  static constexpr u32 kChipID = 0x1FC2;

  enum class DataMode {
    Unencrypted,
    SecureAreaLoad,
    MainDataLoad
  } data_mode = DataMode::Unencrypted;

  void OnCommandStart();
  void Encrypt64(u32* key_buffer, u32* ptr);
  void Decrypt64(u32* key_buffer, u32* ptr);
  void InitKeyCode(u32 game_id_code);

  // TODO: abstract the cartridge file away.
  bool loaded = false;
  std::fstream file;
  u32 file_mask = 0;

  struct {
    // Current index into the data buffer (before modulo data_count)
    int index = 0;

    // Number of requested words
    int count = 0;

    // Number of available words
    int data_count = 0;

    // Underlying transfer buffer
    u32 data[0x1000] {0};
  } transfer;

  // SPI backup data register
  u8 spidata = 0;

  u32 key1_buffer_lvl2[0x412];
  u32 key1_buffer_lvl3[0x412];

  Scheduler& scheduler;
  DMA7& dma7;
  DMA9& dma9;
  IRQ& irq7;
  IRQ& irq9;
  EXMEMCNT& exmemcnt;
  std::unique_ptr<SPIDevice> backup;
};

} // namespace lunar::nds
