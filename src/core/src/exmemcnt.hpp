
#pragma once

#include <util/integer.hpp>

namespace Duality::Core {

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

  auto ReadByte(uint offset) {
    switch (offset) {
      case 0:
        return gba_slot.sram_wait |
          (     gba_slot.rom_wait[0] << 2) |
          (     gba_slot.rom_wait[1] << 4) |
          (     gba_slot.phi         << 5) |
          ((int)gba_slot.cpu_access  << 7);
      case 1:
        return ((int)nds_slot_access          << 3) |
                (     main_mem_enable          << 5) |
                (     main_mem_interface_mode  << 6) |
                ((int)main_mem_access_priority << 7);
    }

    return 0;
  }

  void WriteByte(uint offset, u8 value) {
    switch (offset) {
      case 0:
        gba_slot.sram_wait = value & 3;
        gba_slot.rom_wait[0] = (value >> 2) & 3;
        gba_slot.rom_wait[1] = (value >> 4) & 1;
        gba_slot.phi = (value >> 5) & 3;
        gba_slot.cpu_access = (CPU)(value >> 7);
        break;
      case 1:
        nds_slot_access = (CPU)((value >> 3) & 1);
        //main_mem_enable |= (value >> 5) & 1;
        main_mem_interface_mode = (value >> 6) & 1;
        main_mem_access_priority = (CPU)(value >> 7);
        break;
    }
  }
};

} // namespace Duality::Core
