/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <cstddef>
#include <lunatic/cpu.hpp>
#include <util/integer.hpp>

// #include "arm/memory.hpp"
#include "interconnect.hpp"

namespace Duality::Core {

struct ARM9MemoryBus final : lunatic::Memory {
  ARM9MemoryBus(Interconnect* interconnect);

  void SetDTCM(TCM::Config const& config) { dtcm.config = config; }
  void SetITCM(TCM::Config const& config) { itcm.config = config; }

  auto ReadByte(u32 address, Bus bus) ->  u8 override;
  auto ReadHalf(u32 address, Bus bus) -> u16 override;
  auto ReadWord(u32 address, Bus bus) -> u32 override;
  // auto ReadQuad(u32 address, Bus bus) -> u64 override;

  void WriteByte(u32 address,  u8 value, Bus bus) override;
  void WriteHalf(u32 address, u16 value, Bus bus) override;
  void WriteWord(u32 address, u32 value, Bus bus) override;
  // void WriteQuad(u32 address, u64 value, Bus bus) override;

private:
  void UpdateMemoryMap(u32 address_lo, u64 address_hi);

  template<typename T>
  auto Read(u32 address, Bus bus) -> T;

  template<typename T>
  void Write(u32 address, T value, Bus bus);

  template<class Functor, typename... Args>
  auto VisitVRAMByAddress(u32 address, Args... args) -> typename Functor::return_type;

  template<typename T>
  struct ReadFunctor {
    using return_type = T;

    template<size_t page_count>
    struct value {
      auto operator()(Region<page_count>& region, u32 offset) -> return_type {
        return region.template Read<T>(offset);
      }
    };
  };

  template<typename T>
  struct WriteFunctor {
    using return_type = void;

    template<size_t page_count>
    struct value {
      auto operator()(Region<page_count>& region, u32 offset, T value) -> return_type {
        region.template Write<T>(offset, value);
      }
    };
  };

  template<typename T>
  struct GetUnsafePointerFunctor {
    using return_type = T*;

    template<size_t page_count>
    struct value {
      auto operator()(Region<page_count>& region, u32 offset) -> return_type {
        return region.template GetUnsafePointer<T>(offset);
      }
    };
  };

  auto ReadByteIO(u32 address) ->  u8;
  auto ReadHalfIO(u32 address) -> u16;
  auto ReadWordIO(u32 address) -> u32;

  void WriteByteIO(u32 address,  u8 value);
  void WriteHalfIO(u32 address, u16 value);
  void WriteWordIO(u32 address, u32 value);

  /// ARM9 internal memory
  u8 bios[0x8000] {0};
  u8 dtcm_data[0x4000] {0};
  u8 itcm_data[0x8000] {0};

  /// ARM7 and ARM9 shared memory
  u8* ewram;
  Interconnect::SWRAM::Alloc const& swram;

  /// HW components and I/O registers
  Cartridge& cart;
  IPC& ipc;
  IRQ& irq9;
  MathEngine& math_engine;
  Timer& timer;
  DMA9& dma;
  VideoUnit& video_unit;
  VRAM& vram;
  Interconnect::WRAMCNT& wramcnt;
  Interconnect::KeyInput& keyinput;
};

} // namespace Duality::Core
