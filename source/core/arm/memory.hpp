/*
 * Copyright (C) 2020 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

#pragma once

#include <array>
#include <cstdint>

#include "cpu_core.hpp"

namespace fauxDS::core::arm {

/** Base class that memory systems must implement
  * in order to be connected to an ARM core.
  * Provides an uniform interface for ARM cores to
  * access memory and for the memory system to generate
  * prefetch or data aborts on offending cores.
  */
class MemoryBase {
public:
  /// Maximum number of attachable CPU cores.
  /// TODO(fleroviux): get rid of this artificial limitation?
  static constexpr int kMaxCores = 4;

  enum class Bus : int {
    Code,
    Data
  };

  enum class MemoryModel {
    Unprotected,
    ProtectionUnit,
    ManagementUnit
  };

  void RegisterCore(int id, CPUCoreBase* core) {
    cores[id] = core;
  }

  virtual auto GetMemoryModel() const -> MemoryModel = 0;

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

protected:
  /**
    * Generate an abort exception on a connected CPU core.
    *
    * @param  id       CPU core id
    * @param  address  Address that caused the fault
    * @param  bus      Whether code or data was accessed.
    * @param  write    Whether a write caused the abort. May only be set if bus=Data. 
    */
  void SignalFault(int id, std::uint32_t address, Bus bus, bool write) {
  }

  std::array<CPUCoreBase*, kMaxCores> cores;
};

} // namespace fauxDS::core::arm
