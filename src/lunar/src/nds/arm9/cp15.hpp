/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <lunatic/cpu.hpp>

#include <atom/integer.hpp>
#include <lunar/log.hpp>

#include "arm/arm.hpp"
#include "bus/bus.hpp"

namespace lunar::nds {

/** This class emulates the System Control Coprocessor (CP15).
  * It is responsible for various things, such as configuration
  * of the MPU, TCM and cache, alignment and endianness modes,
  * and various other things.
  */
struct CP15 : lunatic::Coprocessor {
  CP15(ARM9MemoryBus* bus);

  void Reset() override;
  void SetCore(lunatic::CPU* core) { this->core = core; }

  bool ShouldWriteBreakBasicBlock(int opcode1, int cn, int cm, int opcode2) override;
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
  auto ReadControlRegister(int cn, int cm, int opcode) -> u32;
  void WriteControlRegister(int cn, int cm, int opcode, u32 value);
  void WriteWaitForIRQ(int cn, int cm, int opcode, u32 value);
  void WriteInvalidateICache(int cn, int cm, int opcode, u32 value);
  void WriteInvalidateICacheLine(int cn, int cm, int opcode, u32 value);
  auto ReadDTCMConfig(int cn, int cm, int opcode) -> u32;
  auto ReadITCMConfig(int cn, int cm, int opcode) -> u32;
  void WriteDTCMConfig(int cn, int cm, int opcode, u32 value);
  void WriteITCMConfig(int cn, int cm, int opcode, u32 value);

  u32 reg_control;
  u32 reg_dtcm;
  u32 reg_itcm;

  lunatic::CPU* core;
  ARM9MemoryBus* bus;
  ARM9MemoryBus::TCM::Config dtcm_config;
  ARM9MemoryBus::TCM::Config itcm_config;
  ReadHandler  handler_rd[kLUTSize];
  WriteHandler handler_wr[kLUTSize];
};

} // namespace lunar::nds
