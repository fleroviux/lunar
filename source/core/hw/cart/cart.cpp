/*
 * Copyright (C) 2020 fleroviux
 */

#include <algorithm>
#include <common/log.hpp>

#include "cart.hpp"

namespace fauxDS::core {

void Cartridge::Reset() {
  if (file.is_open())
    file.close();
  loaded = false;
  // FIXME: properly reset AUXSPICNT and ROMCTRL.
  //auxspicnt = {};
  //romctrl = {};
  cardcmd = {};
  spidata = 0;
}

void Cartridge::Load(std::string const& path) {
  u32 file_size;

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
}

void Cartridge::OnCommandStart() {
  transfer.index = 0;
  transfer.data_count = 0;

  if (romctrl.data_block_size == 0) {
    transfer.count = 0;
  } else if (romctrl.data_block_size == 7) {
    transfer.count = 1;
  } else {
    transfer.count = 0x40 << romctrl.data_block_size;
  }

  switch (cardcmd.buffer[0]) {
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

    /// 3rd Get ROM Chip ID
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

  if (auxspicnt.enable_ready_irq && transfer.data_count != 0) {
    // TODO: only raise the IRQ on the CPU which has rights to the cartridge bus.
    irq7.Raise(IRQ::Source::Cart_DataReady);
    irq9.Raise(IRQ::Source::Cart_DataReady);
  } 
}

auto Cartridge::ReadSPI() -> u8 {
  return spidata;
}

void Cartridge::WriteSPI(u8 value) {
  spidata = backup.Transfer(value);
  
  if (!auxspicnt.chipselect_hold)
    backup.Deselect();
}

auto Cartridge::ReadROM() -> u32 {
  ASSERT(transfer.count != 0, "Cartridge: attempted to read data outside of a transfer.");

  u32 data = 0xFFFFFFFF;

  if (transfer.data_count != 0) {
    data = transfer.data[transfer.index++ % transfer.data_count];
  }

  if (transfer.index == transfer.count) {
    transfer.index = 0;
    transfer.count = 0;
  } else if (auxspicnt.enable_ready_irq) {
    // TODO: only raise the IRQ on the CPU which has rights to the cartridge bus.
    irq7.Raise(IRQ::Source::Cart_DataReady);
    irq9.Raise(IRQ::Source::Cart_DataReady);
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
  bool busy = cart.transfer.count != 0;

  switch (offset) {
    case 0:
    case 1:
      return 0;
    case 2:
      // NOTE: this actually is "data ready", but we don't emulate delays at the moment.
      // Therefore data is always available as long as the transfer is in progress.
      return busy ? 0x80 : 0;
    case 3:
      return data_block_size | (busy ? 0x80 : 0);
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
      if (value & 0x80) {
        ASSERT(cart.transfer.count == 0, "Cartridge: attempted to engage transfer while interface is busy.");
        cart.OnCommandStart();
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


} // namespace fauxDS::core
