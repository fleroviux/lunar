
// Copyright (C) 2022 fleroviux

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <lunar/log.hpp>

#include "backup/eeprom.hpp"
#include "backup/eeprom512b.hpp"
#include "backup/flash.hpp"
#include "cart.hpp"

namespace lunar::nds {

static constexpr int kCyclesPerByte[2] { 5, 8 };

void Cartridge::Reset() {
  if (file.is_open())
    file.close();
  loaded = false;
  // TODO: properly reset AUXSPICNT and ROMCTRL.
  //auxspicnt = {};
  //romctrl = {};
  cardcmd = {};
  spidata = 0;
  backup = nullptr;
}

void Cartridge::Load(std::string const& path, bool direct_boot) {
  u32 file_size;
  auto save_path = std::filesystem::path{path}.replace_extension(".sav").string();

  if (file.is_open())
    file.close();
  loaded = false;

  file.open(path, std::ios::in | std::ios::binary);
  ASSERT(file.good(), "Cart: failed to load ROM: {0}", path);
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
  u32 game_id_code;
  file.seekg(0xC);
  file.read((char*)&game_id_code, sizeof(u32));
  InitKeyCode(game_id_code);

  data_mode = direct_boot ? DataMode::MainDataLoad : DataMode::Unencrypted;
}

void Cartridge::OnCommandStart() {
  u8* cmd = &cardcmd.buffer[0];
  bool unknown_command = false;

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

  if (data_mode == DataMode::Unencrypted) {
    switch (cardcmd.buffer[0]) {
      case 0x9F: {
        // Dummy (read high-z bytes)
        transfer.data[0] = 0xFFFFFFFF;
        transfer.data_count = 1;
        break;
      }
      case 0x00: {
        // Get Cartridge Header
        // TODO: check what the correct behavior for reading past 0x200 bytes is.
        file.seekg(0);
        file.read((char*)transfer.data, 0x200);
        std::memset(&transfer.data[0x80], 0xFF, 0xE00);
        transfer.data_count = 0x400;
        break;
      }
      case 0x90: {
        // 1st Get ROM Chip ID
        transfer.data[0] = kChipID;
        transfer.data_count = 1;
        break;
      }
      case 0x3C: {
        // Activate KEY1 encryption
        data_mode = DataMode::SecureAreaLoad;
        break;
      }
      default: {
        unknown_command = true;
        break;
      }
    }
  } else if (data_mode == DataMode::SecureAreaLoad) {
    u8 command[8];

    for (int i = 0; i < 8; i++) {
      command[i] = cardcmd.buffer[7 - i];
    }

    Decrypt64(key1_buffer_lvl2, (u32*)&command[0]);

    for (int i = 0; i < 4; i++) {
      std::swap(command[i], command[7 - i]);
    }

    switch (command[0] & 0xF0) {
      case 0x40: {
        // Activate KEY2 Encryption Mode
        LOG_WARN("Cart: unhandled 'Activate KEY2 encryption' command");
        break;
      }
      case 0x10: {
        // 2nd Get ROM Chip ID
        transfer.data[0] = kChipID;
        transfer.data_count = 1;
        break;
      }
      case 0x20: {
        // Get Secure Area Block
        u32 address = (command[2] & 0xF0) << 8;

        file.seekg(address);
        file.read((char*)&transfer.data[0], 4096);
        transfer.data_count = 1024;

        if (address == 0x4000) {
          transfer.data[0] = 0x72636e65; // encr
          transfer.data[1] = 0x6a624f79; // yObj

          for (int i = 0; i < 512; i += 2) {
            Encrypt64(key1_buffer_lvl3, &transfer.data[i]);
          }

          Encrypt64(key1_buffer_lvl2, &transfer.data[0]);
        }
        break;
      }
      case 0xA0: {
        // Enter Main Data Mode
        data_mode = DataMode::MainDataLoad;
        break;
      }
      default: {
        unknown_command = true;
        cmd = &command[0];
        break;
      }
    }
  } else if (data_mode == DataMode::MainDataLoad) {
    switch (cardcmd.buffer[0]) {
      case 0xB7: {
        // Get Data
        u32 address;

        address  = cardcmd.buffer[1] << 24;
        address |= cardcmd.buffer[2] << 16;
        address |= cardcmd.buffer[3] <<  8;
        address |= cardcmd.buffer[4] <<  0;
        
        address &= file_mask;

        if (address <= 0x7FFF) {
          address = 0x8000 + (address & 0x1FF);
          LOG_WARN("Cart: attempted to read protected region.");
        }

        ASSERT(transfer.count <= 0x80, "Cart: command 0xB7: size greater than 0x200 is untested.");
        ASSERT((address & 0x1FF) == 0, "Cart: command 0xB7: address unaligned to 0x200 is untested.");

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
      case 0xB8: {
        // 3rd Get ROM Chip ID
        transfer.data[0] = kChipID;
        transfer.data_count = 1;
        break;
      }
      default: {
        unknown_command = true;
        break;
      }
    }
  }

  if (unknown_command) {
    ASSERT(false, "Cart: unhandled command (mode={}): {:02X}-{:02X}-{:02X}-{:02X}-{:02X}-{:02X}-{:02X}-{:02X}",
      (int)data_mode, cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], cmd[6], cmd[7]);
  }

  romctrl.busy = transfer.data_count != 0;

  if (romctrl.busy) {
    scheduler.Add(sizeof(u32) * kCyclesPerByte[romctrl.transfer_clock_rate], [this](int late) {
      romctrl.data_ready = true;

      if (exmemcnt.nds_slot_access == EXMEMCNT::CPU::ARM7) {
        dma7.Request(DMA7::Time::Slot1);
      } else {
        dma9.Request(DMA9::Time::Slot1);
      }
    });
  } else if (auxspicnt.enable_ready_irq) {
    if (exmemcnt.nds_slot_access == EXMEMCNT::CPU::ARM7) {
      irq7.Raise(IRQ::Source::Cart_DataReady);
    } else {
      irq9.Raise(IRQ::Source::Cart_DataReady);
    }
  }
}

void Cartridge::Encrypt64(u32* key_buffer, u32* ptr) {
  u32 x = ptr[1];
  u32 y = ptr[0];

  for (int i = 0; i <= 0xF; i++) {
    u32 z = key_buffer[i] ^ x;

    x = key_buffer[0x012 + u8(z >> 24)];
    x = key_buffer[0x112 + u8(z >> 16)] + x;
    x = key_buffer[0x212 + u8(z >>  8)] ^ x;
    x = key_buffer[0x312 + u8(z >>  0)] + x;

    x ^= y;
    y  = z;
  }

  ptr[0] = x ^ key_buffer[16];
  ptr[1] = y ^ key_buffer[17];
}

void Cartridge::Decrypt64(u32* key_buffer, u32* ptr) {
  u32 x = ptr[1];
  u32 y = ptr[0];

  for (int i = 0x11; i >= 0x02; i--) {
    u32 z = key_buffer[i] ^ x;

    x = key_buffer[0x012 + u8(z >> 24)];
    x = key_buffer[0x112 + u8(z >> 16)] + x;
    x = key_buffer[0x212 + u8(z >>  8)] ^ x;
    x = key_buffer[0x312 + u8(z >>  0)] + x;

    x ^= y;
    y  = z;
  }

  ptr[0] = x ^ key_buffer[1];
  ptr[1] = y ^ key_buffer[0];
}

void Cartridge::InitKeyCode(u32 game_id_code) {
  u32 keycode[3];

  auto apply_keycode = [&](u32* key_buffer_dst, u32* key_buffer_src) {
    u32 scratch[2] = {0, 0};

    Encrypt64(key_buffer_src, &keycode[1]);
    Encrypt64(key_buffer_src, &keycode[0]);

    for (int i = 0; i <= 0x11; i++) {
      // TODO: do not rely on builtins
      key_buffer_dst[i] = key_buffer_src[i] ^ __builtin_bswap32(keycode[i & 1]);
    }

    for (int i = 0x12; i <= 0x411; i++) {
      key_buffer_dst[i] = key_buffer_src[i];
    }

    for (int i = 0; i <= 0x410; i += 2)  {
      Encrypt64(key_buffer_dst, scratch);
      key_buffer_dst[i + 0] = scratch[1];
      key_buffer_dst[i + 1] = scratch[0];
    }
  };

  // TODO: pass the BIOS data from outside somehow?
  std::ifstream bios{"bios7.bin", std::ios::binary};
  bios.seekg(0x30);
  bios.read((char*)&key1_buffer_lvl2[0], 0x1048);
  bios.close();

  keycode[0] = game_id_code;
  keycode[1] = game_id_code >> 1;
  keycode[2] = game_id_code << 1;
  apply_keycode(key1_buffer_lvl2, key1_buffer_lvl2);
  apply_keycode(key1_buffer_lvl2, key1_buffer_lvl2);

  keycode[1] <<= 1;
  keycode[2] >>= 1;
  apply_keycode(key1_buffer_lvl3, key1_buffer_lvl2);
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
      if (exmemcnt.nds_slot_access == EXMEMCNT::CPU::ARM7) {
        irq7.Raise(IRQ::Source::Cart_DataReady);
      } else {
        irq9.Raise(IRQ::Source::Cart_DataReady);
      }
    }
  } else {
    scheduler.Add(sizeof(u32) * kCyclesPerByte[romctrl.transfer_clock_rate], [this](int late) {
      romctrl.data_ready = true;

      if (exmemcnt.nds_slot_access == EXMEMCNT::CPU::ARM7) {
        dma7.Request(DMA7::Time::Slot1);
      } else {
        dma9.Request(DMA9::Time::Slot1);
      }
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


} // namespace lunar::nds
