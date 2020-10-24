/*
 * Copyright (C) 2020 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

#pragma once

#include <cstdint>

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

  virtual void SetDTCM(std::uint32_t base, std::uint32_t limit) {}
  virtual void SetITCM(std::uint32_t base, std::uint32_t limit) {}

  virtual auto ReadByte(std::uint32_t address,
                        Bus bus,
                        int core) -> std::uint8_t  = 0;
  
  virtual auto ReadHalf(std::uint32_t address,
                        Bus bus, 
                        int core) -> std::uint16_t = 0;
  
  virtual auto ReadWord(std::uint32_t address, 
                        Bus bus,
                        int core) -> std::uint32_t = 0;
  
  virtual void WriteByte(std::uint32_t address,
                         std::uint8_t  value,
                         int core) = 0;
  
  virtual void WriteHalf(std::uint32_t address,
                         std::uint16_t value,
                         int core) = 0;
  
  virtual void WriteWord(std::uint32_t address,
                         std::uint32_t value,
                         int core) = 0;
};

} // namespace fauxDS::core::arm
