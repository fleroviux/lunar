/*
 * Copyright (C) 2020 fleroviux
 */

#include <algorithm>
#include <cstring>
#include <util/log.hpp>
#include <filesystem>

#include "backup/eeprom.hpp"
#include "backup/eeprom512b.hpp"
#include "backup/flash.hpp"
#include "cart.hpp"

namespace Duality::Core {

static constexpr int kCyclesPerByte[2] { 5, 8 };

void Cartridge::Reset() {
  if (file.is_open())
    file.close();
  loaded = false;
  // FIXME: properly reset AUXSPICNT and ROMCTRL.
  //auxspicnt = {};
  //romctrl = {};
  cardcmd = {};
  spidata = 0;
  backup = nullptr;
}

void Cartridge::Load(std::string const& path) {
  u32 file_size;
  auto save_path = std::filesystem::path{path}.replace_extension(".sav").string();

  if (file.is_open())
    file.close();
  loaded = false;

  file.open(path, std::ios::in | std::ios::binary);
  ASSERT(file.good(), "Cartridge: failed to load ROM: {0}", path);
  loaded = true;
  file.seekg(0, std::ios::end);
  file_size = file.tellg();
  file.seekg(0);

  // Generate power-of-two mask for ROM mirroring.
  for (int i = 0; i < 32; i++) {
    if (file_size <= (1 << i)) {
      file_mask = (1 << i) - 1;
      break;
    }
  }

  backup = std::make_unique<FLASH>(save_path, FLASH::Size::_1024K);
  // backup = std::make_unique<EEPROM>(save_path, EEPROM::Size::_128K, false);
  // backup = std::make_unique<EEPROM512B>(save_path);

  // Read ID code for KEY1 encryption.
  file.seekg(0xC);
  file.read((char*)&idcode, sizeof(u32));
}

void Cartridge::Encrypt64(u32* ptr) {
  u32 x = ptr[1];
  u32 y = ptr[0];

  for (int i = 0; i <= 0xF; i++) {
    u32 z = keybuf[i] ^ x;

    x = keybuf[0x048/4 + u8(z >> 24)];
    x = keybuf[0x448/4 + u8(z >> 16)] + x;
    x = keybuf[0x848/4 + u8(z >>  8)] ^ x;
    x = keybuf[0xC48/4 + u8(z >>  0)] + x;

    x ^= y;
    y = z;
  }

  ptr[0] = x ^ keybuf[16];
  ptr[1] = y ^ keybuf[17];
}

void Cartridge::Decrypt64(u32* ptr) {
  u32 x = ptr[1];
  u32 y = ptr[0];

  for (int i = 0x11; i >= 0x02; i--) {
    u32 z = keybuf[i] ^ x;

    x = keybuf[0x048/4 + u8(z >> 24)];
    x = keybuf[0x448/4 + u8(z >> 16)] + x;
    x = keybuf[0x848/4 + u8(z >>  8)] ^ x;
    x = keybuf[0xC48/4 + u8(z >>  0)] + x;

    x ^= y;
    y = z;
  }

  ptr[0] = x ^ keybuf[1];
  ptr[1] = y ^ keybuf[0];
}

// NOTE: our modulo is GBATEK's module divided by four.
void Cartridge::InitKeyCode(u32 idcode, int level, int modulo) {
  // TODO: pass the BIOS data from outside somehow?
  std::ifstream bios{"bios7.bin", std::ios::binary};
  bios.seekg(0x30);
  bios.read((char*)&keybuf[0], 0x1048);
  bios.close();

  u32 keycode[3];

  // TODO: move this into an external function?
  auto apply_keycode = [&]() {
    Encrypt64(&keycode[1]);
    Encrypt64(&keycode[0]);

    u32 scratch[2] {0, 0};

    for (int i = 0; i <= 0x44/4; i++) {
      // TODO: do not rely on builtins
      // TODO: optimize modulo with a AND-mask
      keybuf[i] ^= __builtin_bswap32(keycode[i % modulo]);
    }

    for (int i = 0; i <= 0x1040/4; i += 2)  {
      Encrypt64(scratch);
      keybuf[i + 0] = scratch[1];
      keybuf[i + 1] = scratch[0];
    }
  };

  keycode[0] = idcode;
  keycode[1] = idcode >> 1;
  keycode[2] = idcode << 1;

  if (level >= 1) apply_keycode();
  if (level >= 2) apply_keycode();

  if (level >= 3) {
    keycode[1] <<= 1;
    keycode[2] >>= 1;
    apply_keycode();
  }
}

static bool key1_encrypt = false;

void Cartridge::OnCommandStart() {
  transfer.index = 0;
  transfer.data_count = 0;
  romctrl.data_ready = false;

  if (romctrl.data_block_size == 0) {
    transfer.count = 0;
  } else if (romctrl.data_block_size == 7) {
    transfer.count = 1;
  } else {
    transfer.count = 0x40 << romctrl.data_block_size;
  }

  if (key1_encrypt) {
    u8 buffer[8];

    for (int i = 0; i < 8; i++) {
      buffer[i] = cardcmd.buffer[7 - i];
    }

    InitKeyCode(idcode, 2, 2);
    Decrypt64((u32*)&buffer[0]);

    for (int i = 0; i < 8; i++) {
      cardcmd.buffer[i] = buffer[7 - i];
    }
    
    LOG_TRACE("Cart: encrypted CMD: {:02X}-{:02X}-{:02X}-{:02X}-{:02X}-{:02X}-{:02X}-{:02X}",
      cardcmd.buffer[0], cardcmd.buffer[1], cardcmd.buffer[2], cardcmd.buffer[3], cardcmd.buffer[4], cardcmd.buffer[5], cardcmd.buffer[6], cardcmd.buffer[7]);
  
    if ((cardcmd.buffer[0] & 0xF0) == 0x40) {
      // Activate KEY2 Encryption Mode
      
      // ignored...?
    } else if ((cardcmd.buffer[0] & 0xF0) == 0x10) {
    // 2nd Get ROM Chip ID
      // TODO: what about the 0x910 dummy bytes? are these just internal?
      // TODO: this is duplicated from the other Get ROM Chip ID commands.
      transfer.data[0] = 0x1FC2;
      transfer.data_count = 1;
    } else if ((cardcmd.buffer[0] & 0xF0) == 0x20) {
      // Get Secure Area Block

      // TODO: make sure that we calculate the address correctly.
      u32 address = (cardcmd.buffer[2] & 0xF0) << 8;

      file.seekg(address);
      file.read((char*)&transfer.data[0], 4096);
      transfer.data_count = 1024;

      LOG_TRACE("Cart: secure area read: 0x{:08X}", address);

      if (address == 0x4000) {
        //LOG_TRACE("Cart: debug 0x{:08X}", transfer.data[0]);

        transfer.data[0] = 0x72636e65;
        transfer.data[1] = 0x6a624f70;

        //LOG_TRACE("Cart: foo {}\n", (char*)&transfer.data[0]);

        InitKeyCode(idcode, 3, 2);
        
        for (int i = 0; i < 512; i += 2) {
          Encrypt64(&transfer.data[i]);
        }

        InitKeyCode(idcode, 2, 2);
        Encrypt64(&transfer.data[0]);
      }

    } else if ((cardcmd.buffer[0] & 0xF0) == 0xA0) {
      // Enter Main Data Mode
      // TODO: properly handle this
      key1_encrypt = false;
    } else {
      ASSERT(false, "ded");
    }
  } else {
    //LOG_TRACE("Cart: cleartext CMD: {:02X}-{:02X}-{:02X}-{:02X}-{:02X}-{:02X}-{:02X}-{:02X}",
    //  cardcmd.buffer[0], cardcmd.buffer[1], cardcmd.buffer[2], cardcmd.buffer[3], cardcmd.buffer[4], cardcmd.buffer[5], cardcmd.buffer[6], cardcmd.buffer[7]);    

    switch (cardcmd.buffer[0]) {
      /// Get Cartridge Header
      case 0x00: {
        // TODO: check what the correct behavior for reading past 0x200 bytes is.
        file.seekg(0);
        file.read((char*)transfer.data, 0x200);
        std::memset(&transfer.data[0x80], 0xFF, 0xE00);
        transfer.data_count = 0x400;

        //InitKeyCode(idcode, 1, 2);
        //Encrypt64(&transfer.data[0x78/4]);
        break;
      }

      /// Activate KEY1 encryption
      case 0x3C: {
        key1_encrypt = true;
        LOG_TRACE("Cart: activated KEY1 encryption");
        break;
      }

      /// Dummy (read high-z bytes)
      case 0x9F: {
        transfer.data[0] = 0xFFFFFFFF;
        transfer.data_count = 1;
        break;
      }

      /// Get Data 
      case 0xB7: {
        u32 address;

        address  = cardcmd.buffer[1] << 24;
        address |= cardcmd.buffer[2] << 16;
        address |= cardcmd.buffer[3] <<  8;
        address |= cardcmd.buffer[4] <<  0;
        
        address &= file_mask;

        if (address <= 0x7FFF) {
          address = 0x8000 + (address & 0x1FF);
          LOG_WARN("Cartridge: attempted to read protected region.");
        }

        ASSERT(transfer.count <= 0x80, "Cartridge: command 0xB7: size greater than 0x200 is untested.");
        ASSERT((address & 0x1FF) == 0, "Cartridge: command 0xB7: address unaligned to 0x200 is untested.");

        transfer.data_count = std::min(0x80, transfer.count);

        u32 byte_len = transfer.data_count * sizeof(u32);
        u32 sector_a = address >> 12;
        u32 sector_b = (address + byte_len - 1) >> 12;

        file.seekg(address);

        if (sector_a != sector_b) {
          u32 size_a = 0x1000 - (address & 0xFFF);
          u32 size_b = byte_len - size_a;
          file.read((char*)transfer.data, size_a);
          file.seekg(address & ~0xFFF);
          file.read((char*)transfer.data + size_a, size_b);
        } else {
          file.read((char*)transfer.data, byte_len);
        }
        break;
      }

      /// 1st and 3rd Get ROM Chip ID
      // TODO: can we really treat 0x90 and 0xB8 as the same command?
      case 0x90:
      case 0xB8: {
        // TODO: use a plausible chip ID based on the ROM size.
        transfer.data[0] = 0x1FC2;
        transfer.data_count = 1;
        break;
      }

      default: {
        ASSERT(false, "Cartridge: unhandled command 0x{0:02X}", cardcmd.buffer[0]);
      }
    }
  }

  romctrl.busy = transfer.data_count != 0;

  if (romctrl.busy) {
    scheduler.Add(sizeof(u32) * kCyclesPerByte[romctrl.transfer_clock_rate], [this](int late) {
      romctrl.data_ready = true;

      dma7.Request(DMA7::Time::Slot1);
      dma9.Request(DMA9::Time::Slot1);
    });
  } else if (auxspicnt.enable_ready_irq) {
    irq7.Raise(IRQ::Source::Cart_DataReady);
    irq9.Raise(IRQ::Source::Cart_DataReady);
  }
}

auto Cartridge::ReadSPI() -> u8 {
  return spidata;
}

void Cartridge::WriteSPI(u8 value) {
  if (backup) {
    backup->Select();
    spidata = backup->Transfer(value);
    if (!auxspicnt.chipselect_hold) {
      backup->Deselect();
    }
  } else {
    spidata = 0xFF;
  }
}

auto Cartridge::ReadROM() -> u32 {
  u32 data = 0xFFFFFFFF;

  if (!romctrl.data_ready) {
    return data;
  }

  if (transfer.data_count != 0) {
    data = transfer.data[transfer.index++ % transfer.data_count];
  } else {
    transfer.index++;
  }

  romctrl.data_ready = false;

  if (transfer.index == transfer.count) {
    romctrl.busy = false;
    transfer.index = 0;
    transfer.count = 0;

    if (auxspicnt.enable_ready_irq) {
      irq7.Raise(IRQ::Source::Cart_DataReady);
      irq9.Raise(IRQ::Source::Cart_DataReady);
    }
  } else {
    scheduler.Add(sizeof(u32) * kCyclesPerByte[romctrl.transfer_clock_rate], [this](int late) {
      romctrl.data_ready = true;

      dma7.Request(DMA7::Time::Slot1);
      dma9.Request(DMA9::Time::Slot1);
    });
  }

  return data;
}

auto Cartridge::AUXSPICNT::ReadByte(uint offset) -> u8 {
  switch (offset) {
    case 0:
      return baudrate | (busy ? 128 : 0);
    case 1:
      return (select_spi ? 32 : 0) |
             (enable_ready_irq ? 64 : 0) |
             (enable_slot ? 128 : 0);
  }

  UNREACHABLE;
}

void Cartridge::AUXSPICNT::WriteByte(uint offset, u8 value) {
  switch (offset) {
    case 0:
      baudrate = value & 3;
      chipselect_hold = value & 64;
      break;
    case 1:
      select_spi = value & 32;
      enable_ready_irq = value & 64;
      enable_slot = value & 128;
      break;
    default:
      UNREACHABLE;
  }
}

auto Cartridge::ROMCTRL::ReadByte(uint offset) -> u8 {
  switch (offset) {
    case 0:
    case 1:
      return 0;
    case 2:
      return data_ready ? 0x80 : 0;
    case 3:
      return data_block_size | (transfer_clock_rate << 3) | (busy ? 0x80 : 0);
  }

  UNREACHABLE;
}

void Cartridge::ROMCTRL::WriteByte(uint offset, u8 value) {
  switch (offset) {
    case 0:
    case 1:
    case 2:
      break;
    case 3:
      data_block_size = value & 7;
      transfer_clock_rate = (value >> 3) & 1;
      if ((value & 0x80) && !busy) {
        busy = true;
        cart.scheduler.Add(sizeof(u64) * kCyclesPerByte[transfer_clock_rate], [this](int late) {
          cart.OnCommandStart();
        });
      }
      break;
    default:
      UNREACHABLE;
  }
}

auto Cartridge::CARDCMD::ReadByte(uint offset) -> u8 {
  if (offset >= 8)
    UNREACHABLE;
  return buffer[offset];
}

void Cartridge::CARDCMD::WriteByte(uint offset, u8 value) {
  if (offset >= 8)
    UNREACHABLE;
  buffer[offset] = value;
}


} // namespace Duality::Core
