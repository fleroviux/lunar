/*
 * Copyright (C) 2020 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

#pragma once

#include <common/integer.hpp>

namespace fauxDS::core::arm {

/** Base class that memory systems must implement
  * in order to be connected to an ARM core.
  * Provides an uniform interface for ARM cores to
  * access memory.
  */
struct MemoryBase {
  enum class Bus : int {
    Code,
    Data
  };

  virtual void SetDTCM(u32 base, u32 limit) {}
  virtual void SetITCM(u32 base, u32 limit) {}

  virtual auto ReadByte(u32 address,
                        Bus bus,
                        int core) -> u8  = 0;
  
  virtual auto ReadHalf(u32 address,
                        Bus bus, 
                        int core) -> u16 = 0;
  
  virtual auto ReadWord(u32 address, 
                        Bus bus,
                        int core) -> u32 = 0;
  
  virtual void WriteByte(u32 address,
                         u8  value,
                         int core) = 0;
  
  virtual void WriteHalf(u32 address,
                         u16 value,
                         int core) = 0;
  
  virtual void WriteWord(u32 address,
                         u32 value,
                         int core) = 0;
};

} // namespace fauxDS::core::arm
