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
  ARM9MemoryBus(Interconnect* interconnect);

  auto GetMemoryModel() const -> MemoryModel override {
    return MemoryModel::ProtectionUnit;
  }

  void SetDTCM(u32 base, u32 limit) override {
    dtcm_base  = base;
    dtcm_limit = limit;
  }

  void SetITCM(u32 base, u32 limit) override {
    itcm_base  = base;
    itcm_limit = limit;
  }

  auto ReadByte(u32 address, Bus bus, int core) -> u8 override;
  auto ReadHalf(u32 address, Bus bus, int core) -> u16 override;
  auto ReadWord(u32 address, Bus bus, int core) -> u32 override;
  
  void WriteByte(u32 address, u8 value, int core) override;
  void WriteHalf(u32 address, u16 value, int core) override;
  void WriteWord(u32 address, u32 value, int core) override;

  // TODO: this is completely stupid and inaccurate as hell.
  u8 vram[0x200000];

private:
  template<typename T>
  auto Read(u32 address, Bus bus, int core) -> T;

  template<typename T>
  void Write(u32 address, T value, int core);

  auto ReadByteIO(u32 address) ->  u8;
  auto ReadHalfIO(u32 address) -> u16;
  auto ReadWordIO(u32 address) -> u32;
  
  void WriteByteIO(u32 address,  u8 value);
  void WriteHalfIO(u32 address, u16 value);
  void WriteWordIO(u32 address, u32 value);

  u32 dtcm_base  = 0;
  u32 dtcm_limit = 0;
  u8 dtcm[0x4000] {0};

  u32 itcm_base  = 0;
  u32 itcm_limit = 0;
  u8 itcm[0x8000] {0};

  u8* ewram;

  RegisterSet mmio {0x106E};
  Interconnect::SWRAM::Alloc const& swram;
};

} // namespace fauxDS::core
