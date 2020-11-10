/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <common/integer.hpp>
#include <fstream>
#include <string>

namespace fauxDS::core {

struct Cartridge {
  Cartridge() { Reset(); }

  void Reset();
  void Load(std::string const& path);

  struct ROMCTRL {
  } romctrl;

  struct CARDCMD {
    auto ReadByte (uint offset) -> u8;
    void WriteByte(uint offset, u8 value);

  private:
    friend struct fauxDS::core::Cartridge;

    u8 data[8] {0};
  } cardcmd;

private:
  bool loaded = false;
  std::fstream file;
};

} // namespace fauxDS::core
