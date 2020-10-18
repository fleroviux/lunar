/*
 * Copyright (C) 2020 fleroviux
 */

#include <common/log.hpp>

#include "bus.hpp"

namespace fauxDS::core {

ARM9MemoryBus::ARM9MemoryBus(Interconnect* interconnect) : swram(interconnect->swram.arm9) {
  mmio.Map(0x0004, interconnect->fake_dispstat);
  mmio.Map(0x0130, interconnect->keyinput);
}

auto ARM9MemoryBus::ReadByte(u32 address, Bus bus, int core) -> u8 {
  if (address >= itcm_base && address <= itcm_limit)
    return itcm[(address - itcm_base) & 0x7FFF];

  if (bus == Bus::Data && address >= dtcm_base && address <= dtcm_limit)
    return dtcm[(address - dtcm_base) & 0x3FFF];

  switch (address >> 24) {
    // TODO: EWRAM actually is shared between the ARM9 and ARM7 core.
    case 0x02:
      return ewram[address & 0x3FFFFF];
    case 0x03:
      if (swram.data == nullptr) {
        LOG_ERROR("ARM9: attempted to read from SWRAM but it isn't mapped.");
        return 0;
      }
      return swram.data[address & swram.mask];
    case 0x04:
      return mmio.Read(address & 0x00FFFFFF);
    default:
      LOG_ERROR("ARM9: unhandled read byte from 0x{0:08X}", address);
      for (;;) ;
  }

  return 0;
}
  
auto ARM9MemoryBus::ReadHalf(u32 address, Bus bus, int core) -> u16 {
  return ReadByte(address + 0, bus, core) | 
        (ReadByte(address + 1, bus, core) << 8);
}
  
auto ARM9MemoryBus::ReadWord(u32 address, Bus bus, int core) -> u32 {
  return ReadHalf(address + 0, bus, core) | 
        (ReadHalf(address + 2, bus, core) << 16);
}
  
void ARM9MemoryBus::WriteByte(u32 address, u8 value, int core) {
  if (address >= itcm_base && address <= itcm_limit) {
    itcm[(address - itcm_base) & 0x7FFF] = value;
    return;
  }

  if (address >= dtcm_base && address <= dtcm_limit) {
    dtcm[(address - dtcm_base) & 0x3FFF] = value;
    return;
  }

  switch (address >> 24) {
    case 0x02:
      ewram[address & 0x3FFFFF] = value;
      break;
    case 0x03:
      if (swram.data == nullptr) {
        LOG_ERROR("ARM9: attempted to read from SWRAM but it isn't mapped.");
        return;
      }
      swram.data[address & swram.mask] = value;
      break;
    case 0x04:
      mmio.Write(address & 0xFFFFFF, value);
      break;
    case 0x06:
      vram[address & 0x1FFFFF] = value;
      break;
    default:
      LOG_ERROR("ARM9: unhandled write byte 0x{0:08X} = 0x{1:02X}", address, value);
      //for (;;) ;    
  }    
}
  
void ARM9MemoryBus::WriteHalf(u32 address, u16 value, int core) {
  WriteByte(address + 0, value & 0xFF, core);
  WriteByte(address + 1, value >> 8, core);
}

void ARM9MemoryBus::WriteWord(u32 address, u32 value, int core) {
  WriteHalf(address + 0, value & 0xFFFF, core);
  WriteHalf(address + 2, value >> 16, core);
}

} // namespace fauxDS::core
