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
  romctrl = {};
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

auto Cartridge::CARDCMD::ReadByte(uint offset) -> u8 {
  if (offset >= 8)
    UNREACHABLE;
  return data[offset];
}

void Cartridge::CARDCMD::WriteByte(uint offset, u8 value) {
  if (offset >= 8)
    UNREACHABLE;
  data[offset] = value;
}


} // namespace fauxDS::core
