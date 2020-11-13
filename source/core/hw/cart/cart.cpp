/*
 * Copyright (C) 2020 fleroviux
 */

#include <common/log.hpp>

#include "cart.hpp"

namespace fauxDS::core {

void Cartridge::Reset() {
  if (file.is_open())
    file.close();
  loaded = false;
  // FIXME: properly reset ROMCTRL.
  //romctrl = {};
  cardcmd = {};
}

void Cartridge::Load(std::string const& path) {
  if (file.is_open())
    file.close();
  loaded = false;
  file.open(path, std::ios::in | std::ios::binary);
  ASSERT(file.good(), "Cartridge: failed to load ROM: {0}", path);
  loaded = true;
}

void Cartridge::OnCommandStart() {
  // TODO: come up with a better pattern to decode commands.
  switch (cardcmd.buffer[0]) {
    case 0xB8: // hack
    
    /// Encrypted Data Read
    case 0xB7:
      address  = cardcmd.buffer[1] << 24;
      address |= cardcmd.buffer[2] << 16;
      address |= cardcmd.buffer[3] <<  8;
      address |= cardcmd.buffer[4] <<  0;
      if (romctrl.data_block_size == 0) {
        LOG_WARN("Cartridge: Encrypted Data Read with zero block size.");
        romctrl.data_ready = false;
        romctrl.busy = false;
        return;
      }
      if (romctrl.data_block_size == 7) {
        remaining = 1;
      } else {
        remaining = 0x40 << romctrl.data_block_size;
      }
      romctrl.data_ready = true;
      //ASSERT(false, "Cartridge: Encrypted Data Read with address = 0x{0:08X}", address);
      break;
    default:
      ASSERT(false, "Cartridge: unhandled command 0x{0:02X}", cardcmd.buffer[0]);
  }
}

auto Cartridge::ReadData() -> u32 {
  ASSERT(romctrl.busy && remaining != 0, "Cartridge: attempted to read data outside of a transfer.");

  u32 data;
  file.seekg(address);
  file.read((char*)&data, sizeof(u32));
  ASSERT(file.good(), "Cartridge: failed to read data from ROM.");

  LOG_INFO("Cartridge: read word 0x{0:08X}", data);

  // TODO: handle sector wraparound...
  address += 4;

  if (--remaining == 0) {
    romctrl.busy = false;
    romctrl.data_ready = false;
  }

  return data;
}

auto Cartridge::ROMCTRL::ReadByte (uint offset) -> u8 {
  switch (offset) {
    case 0:
    case 1:
      return 0;
    case 2:
      return data_ready ? 0x80 : 0;
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
        ASSERT(!busy, "Cartridge: attempted to engage transfer while interface is busy.");
        busy = true;
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
