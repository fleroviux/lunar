/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <common/integer.hpp>
#include <core/arm/memory.hpp>
#include <core/interconnect.hpp>
#include <cstddef>

namespace fauxDS::core {

struct ARM9MemoryBus final : arm::MemoryBase {
  struct TCMConfig {
    bool enable = false;
    bool enable_read = false;
    u32 base  = 0;
    u32 limit = 0;
  };

  ARM9MemoryBus(Interconnect* interconnect);

  void SetDTCM(TCMConfig& config) { dtcm_config = config; }
  void SetITCM(TCMConfig& config) { itcm_config = config; }

  auto ReadByte(u32 address, Bus bus) ->  u8 override;
  auto ReadHalf(u32 address, Bus bus) -> u16 override;
  auto ReadWord(u32 address, Bus bus) -> u32 override;
  
  void WriteByte(u32 address,  u8 value) override;
  void WriteHalf(u32 address, u16 value) override;
  void WriteWord(u32 address, u32 value) override;

  u8 vram[0x200000];

private:
  template<typename T>
  auto Read(u32 address, Bus bus) -> T;

  template<typename T>
  void Write(u32 address, T value);

  auto ReadByteIO(u32 address) ->  u8;
  auto ReadHalfIO(u32 address) -> u16;
  auto ReadWordIO(u32 address) -> u32;

  void WriteByteIO(u32 address,  u8 value);
  void WriteHalfIO(u32 address, u16 value);
  void WriteWordIO(u32 address, u32 value);

  /// ARM9 internal memory
  u8 bios[0x8000] {0};
  u8 dtcm[0x4000] {0};
  u8 itcm[0x8000] {0};

  /// ITCM and DTCM configuration
  TCMConfig dtcm_config;
  TCMConfig itcm_config;

  /// ARM7 and ARM9 shared memory
  u8* ewram;
  Interconnect::SWRAM::Alloc const& swram;

  /// HW components and I/O registers
  IPC& ipc;
  IRQ& irq9;
  VideoUnit& video_unit;
  Interconnect::WRAMCNT& wramcnt;
  Interconnect::KeyInput& keyinput;
};

} // namespace fauxDS::core
