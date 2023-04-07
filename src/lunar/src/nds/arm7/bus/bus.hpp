/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <lunatic/cpu.hpp>
#include <atom/integer.hpp>

#include "nds/interconnect.hpp"

namespace lunar::nds {

class ARM7MemoryBus final : public lunatic::Memory {
  public:
    explicit ARM7MemoryBus(Interconnect* interconnect);

    bool& IsHalted() { return halted; }

    auto ReadByte(u32 address, Bus bus) ->  u8 override;
    auto ReadHalf(u32 address, Bus bus) -> u16 override;
    auto ReadWord(u32 address, Bus bus) -> u32 override;

    void WriteByte(u32 address,  u8 value, Bus bus) override;
    void WriteHalf(u32 address, u16 value, Bus bus) override;
    void WriteWord(u32 address, u32 value, Bus bus) override;

  private:
    void UpdateMemoryMap(u32 address_lo, u64 address_hi);

    template<typename T>
    auto Read(u32 address) -> T;

    template<typename T>
    void Write(u32 address, T value);

    auto ReadByteIO(u32 address) ->  u8;
    auto ReadHalfIO(u32 address) -> u16;
    auto ReadWordIO(u32 address) -> u32;

    void WriteByteIO(u32 address,  u8 value);
    void WriteHalfIO(u32 address, u16 value);
    void WriteWordIO(u32 address, u32 value);

    // ARM7 internal memory
    u8 bios[0x4000] {0};
    u8 iwram[0x10000];

    // ARM7 and ARM9 shared memory
    std::array<u8, 0x400000>& ewram;
    SWRAM& swram;

    // HW components and I/O registers
    APU& apu;
    Cartridge& cart;
    IPC& ipc;
    IRQ& irq7;
    SPI& spi;
    Timer& timer;
    DMA7& dma;
    VideoUnit& video_unit;
    VRAM& vram;
    WIFI& wifi;
    EXMEMCNT& exmemcnt;
    KeyPad& keypad;
    RTC& rtc;
    bool halted;
    u8 postflag;
    u16 soundbias;
};

} // namespace lunar::nds
