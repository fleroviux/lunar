/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <common/integer.hpp>
#include <common/log.hpp>
#include <core/arm/coprocessor.hpp>

#include "bus.hpp"

namespace fauxDS::core {

/** This class emulates the System Control Coprocessor (CP15).
  * It is responsable for various things, such as configuration
  * of the MPU/MMU, alignment and endianness modes,
  * cache control and various other things.
  */
struct CP15 : arm::Coprocessor {
  CP15(ARM9MemoryBus* bus);

  void Reset() override;

  /**
    * Read coprocessor register.
    *
    * @param  opcode1  Opcode #1 (should always be zero for CP15)
    * @param  cn       Coprocessor Register #1
    * @param  cm       Coprocessor Register #2
    * @param  opcode2  Opcode #2
    *
    * @returns the value contained in the coprocessor register.
    */
  auto Read(int opcode1, int cn, int cm, int opcode2) -> u32 override;

  /**
    * Write to coprocessor register.
    *
    * @param  opcode1  Opcode #1 (should always be zero for CP15)
    * @param  cn       Coprocessor Register #1
    * @param  cm       Coprocessor Register #2
    * @param  opcode2  Opcode #2
    * @param  value    Word to write
    */
  void Write(int opcode1, int cn, int cm, int opcode2, u32 value) override;

private:
  auto Index(int cn, int cm, int opcode) -> int {
    return (cn << 7) | (cm << 3) | opcode;
  }

  typedef auto (CP15::*ReadHandler )(int cn, int cm, int opcode) -> u32;
  typedef void (CP15::*WriteHandler)(int cn, int cm, int opcode, u32 value);

  /** 
    * Register a register read handler.
    *
    * @param  cn       Coprocessor Register #1
    * @param  cm       Coprocessor Register #2
    * @param  opcode   Opcode #2
    * @param  handler  Read handler
    */
  void RegisterHandler(int cn, int cm, int opcode, ReadHandler handler);
  
  /** 
    * Register a register write handler.
    *
    * @param  cn       Coprocessor Register #1
    * @param  cm       Coprocessor Register #2
    * @param  opcode   Opcode #2
    * @param  handler  Write handler
    */
  void RegisterHandler(int cn, int cm, int opcode, WriteHandler handler);

  auto DefaultRead(int cn, int cm, int opcode) -> u32 {
    LOG_WARN("CP15: unknown read c{0} c{1} #{2}", cn, cm, opcode);
    return 0;
  }

  void DefaultWrite(int cn, int cm, int opcode, u32 value) {
    LOG_WARN("CP15: unknown write c{0} c{1} #{2} = 0x{3:08X}", cn, cm, opcode, value);
  }

  auto ReadMainID(int cn, int cm, int opcode) -> u32;
  auto ReadCacheType(int cn, int cm, int opcode) -> u32;
  auto ReadDTCMConfig(int cn, int cm, int opcode) -> u32;
  auto ReadITCMConfig(int cn, int cm, int opcode) -> u32;
  void WriteDTCMConfig(int cn, int cm, int opcode, u32 value);
  void WriteITCMConfig(int cn, int cm, int opcode, u32 value);

  struct TCMConfig {
    u32 value;
    u32 base;
    u32 limit;
  } dtcm, itcm;

  ARM9MemoryBus* bus;

  /** List of register read handlers.
    * The array is indexed like this:
    * bits 0- 2: opcode2
    * bits 3- 6: cm
    * bits 7-10: cn
    */
  ReadHandler handler_rd[0x800];
  
  /** List of register write handlers.
    * Indexing works the same as for read handlers.
    */
  WriteHandler handler_wr[0x800];
};

} // namespace fauxDS::core
