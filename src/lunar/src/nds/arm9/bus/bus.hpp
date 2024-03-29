/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <cstddef>
#include <lunatic/cpu.hpp>
#include <atom/integer.hpp>

#include "nds/interconnect.hpp"

namespace lunar::nds {

class ARM9MemoryBus final : public lunatic::Memory {
  public:
    explicit ARM9MemoryBus(Interconnect* interconnect);

    void SetDTCM(TCM::Config const& config) { dtcm.config = config; }
    void SetITCM(TCM::Config const& config) { itcm.config = config; }

    auto ReadByte(u32 address, Bus bus) ->  u8 override;
    auto ReadHalf(u32 address, Bus bus) -> u16 override;
    auto ReadWord(u32 address, Bus bus) -> u32 override;

    void WriteByte(u32 address,  u8 value, Bus bus) override;
    void WriteHalf(u32 address, u16 value, Bus bus) override;
    void WriteWord(u32 address, u32 value, Bus bus) override;

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

    // ARM9 internal memory
    u8 bios[0x8000] {0};
    u8 dtcm_data[0x4000] {0};
    u8 itcm_data[0x8000] {0};

    // ARM7 and ARM9 shared memory
    std::array<u8, 0x400000>& ewram;
    SWRAM& swram;

    // HW components and I/O registers
    Cartridge& cart;
    IPC& ipc;
    IRQ& irq9;
    Math& math;
    Timer& timer;
    DMA9& dma;
    VideoUnit& video_unit;
    VRAM& vram;
    EXMEMCNT& exmemcnt;
    KeyPad& keypad;
    u8 postflag;
};

} // namespace lunar::nds
