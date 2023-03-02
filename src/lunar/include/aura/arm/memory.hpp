
#pragma once

#include <atom/integer.hpp>

namespace aura::arm {

class Memory {
  public:
    enum class Bus {
      Code,
      Data,
      System
    };

    virtual ~Memory() = default;

    virtual u8  ReadByte(u32 vaddr, Bus bus) = 0;
    virtual u16 ReadHalf(u32 vaddr, Bus bus) = 0;
    virtual u32 ReadWord(u32 vaddr, Bus bus) = 0;

    virtual void WriteByte(u32 vaddr, u8  value, Bus bus) = 0;
    virtual void WriteHalf(u32 vaddr, u16 value, Bus bus) = 0;
    virtual void WriteWord(u32 vaddr, u32 value, Bus bus) = 0;
};

} // namespace aura::arm
