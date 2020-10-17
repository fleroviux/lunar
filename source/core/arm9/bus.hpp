/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <common/integer.hpp>
#include <core/arm/memory.hpp>
#include <core/interconnect.hpp>

namespace fauxDS::core {

struct ARM9MemoryBus final : arm::MemoryBase {
  ARM9MemoryBus(Interconnect* interconnect);

  auto GetMemoryModel() const -> MemoryModel override;

  void SetDTCM(u32 base, u32 limit) override;
  void SetITCM(u32 base, u32 limit) override;

  auto ReadByte(u32 address, Bus bus, int core) -> u8 override;
  auto ReadHalf(u32 address, Bus bus, int core) -> u16 override;
  auto ReadWord(u32 address, Bus bus, int core) -> u32 override;
  
  void WriteByte(u32 address, u8 value, int core) override;
  void WriteHalf(u32 address, u16 value, int core) override;
  void WriteWord(u32 address, u32 value, int core) override;

//private:
  u32 dtcm_base  = 0;
  u32 dtcm_limit = 0;
  u8 dtcm[0x4000] {0};

  u32 itcm_base  = 0;
  u32 itcm_limit = 0;
  u8 itcm[0x8000] {0};

  u8 ewram[0x400000] {0};
  u8 vblank_flag = 0;

  // TODO: this is completely stupid and inaccurate as hell.
  u8 vram[0x200000];

  RegisterSet mmio {0x106E};
  Interconnect* interconnect;
};

} // namespace fauxDS::core
