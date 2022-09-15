
// Copyright (C) 2022 fleroviux

#pragma once

#include <lunar/integer.hpp>

namespace lunar::nds {

struct EXMEMCNT {
  enum class CPU {
    ARM9 = 0,
    ARM7 = 1
  };

  struct {
    int sram_wait = 0;
    int rom_wait[2] {0};
    int phi = 0;
    CPU cpu_access = CPU::ARM9;
  } gba_slot;

  CPU nds_slot_access = CPU::ARM9;

  int main_mem_enable = 1;
  int main_mem_interface_mode = 0;
  CPU main_mem_access_priority = CPU::ARM9;

  auto ReadByte(uint offset) -> u8;
  void WriteByte(uint offset, u8 value);
};

} // namespace lunar::nds
