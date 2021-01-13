/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <common/integer.hpp>
#include <core/arm/memory.hpp>
#include <core/interconnect.hpp>

namespace fauxDS::core {

struct ARM7MemoryBus final : arm::MemoryBase {
  ARM7MemoryBus(Interconnect* interconnect);

  auto ReadByte(u32 address, Bus bus) ->  u8 override;
  auto ReadHalf(u32 address, Bus bus) -> u16 override;
  auto ReadWord(u32 address, Bus bus) -> u32 override;

  void WriteByte(u32 address,  u8 value) override;
  void WriteHalf(u32 address, u16 value) override;
  void WriteWord(u32 address, u32 value) override;

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

  /// ARM7 internal memory
  u8 bios[0x4000] {0};
  u8 iwram[0x10000];

  /// ARM7 and ARM9 shared memory
  u8* ewram;
  Interconnect::SWRAM::Alloc const& swram;

  /// HW components and I/O registers
  APU& apu;
  Cartridge& cart;
  IPC& ipc;
  IRQ& irq7;
  SPI& spi;
  Timer& timer;
  DMA7& dma;
  VideoUnit& video_unit;
  VRAM& vram;
  Interconnect::WRAMCNT& wramcnt;
  Interconnect::KeyInput& keyinput;
  Interconnect::ExtKeyInput& extkeyinput;
};

} // namespace fauxDS::core
