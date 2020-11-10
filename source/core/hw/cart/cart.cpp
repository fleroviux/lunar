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
  ASSERT(false, "unhandled command 0x{0:02X}", cardcmd.buffer[0]);

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
