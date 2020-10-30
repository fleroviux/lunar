/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <common/integer.hpp>
#include <common/log.hpp>
#include <core/arm/arm.hpp>
#include <core/arm/coprocessor.hpp>

#include "bus.hpp"

namespace fauxDS::core {

/** This class emulates the System Control Coprocessor (CP15).
  * It is responsable for various things, such as configuration
  * of the MPU, TCM and cache, alignment and endianness modes,
  * and various other things.
  */
struct CP15 : arm::Coprocessor {
  CP15(arm::ARM* core, ARM9MemoryBus* bus);

  void Reset() override;

  auto Read (int opcode1, int cn, int cm, int opcode2) -> u32 override;
  void Write(int opcode1, int cn, int cm, int opcode2, u32 value) override;

private:
  // Unfortunately we cannot invoke "Index" from here.
  // Calculation: Index(15, 15, 7) + 1
  static constexpr int kLUTSize = 0x800;

  static constexpr auto Index(int cn, int cm, int opcode) -> int {
    return (cn << 7) | (cm << 3) | opcode;
  }

  using ReadHandler  = auto (CP15::*)(int cn, int cm, int opcode) -> u32;
  using WriteHandler = void (CP15::*)(int cn, int cm, int opcode, u32 value);

  void RegisterHandler(int cn, int cm, int opcode, ReadHandler  handler);
  void RegisterHandler(int cn, int cm, int opcode, WriteHandler handler);

  auto DefaultRead(int cn, int cm, int opcode) -> u32;
  void DefaultWrite(int cn, int cm, int opcode, u32 value);
  auto ReadMainID(int cn, int cm, int opcode) -> u32;
  auto ReadCacheType(int cn, int cm, int opcode) -> u32;
  void WriteWaitForIRQ(int cn, int cm, int opcode, u32 value);
  auto ReadDTCMConfig(int cn, int cm, int opcode) -> u32;
  auto ReadITCMConfig(int cn, int cm, int opcode) -> u32;
  void WriteDTCMConfig(int cn, int cm, int opcode, u32 value);
  void WriteITCMConfig(int cn, int cm, int opcode, u32 value);

  struct TCMConfig {
    u32 value;
    u32 base;
    u32 limit;
  } dtcm, itcm;

  arm::ARM* core;
  ARM9MemoryBus* bus;
  ReadHandler  handler_rd[kLUTSize];
  WriteHandler handler_wr[kLUTSize];
};

} // namespace fauxDS::core
