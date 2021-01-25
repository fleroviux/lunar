/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <array>
#include <buildconfig.hpp>
#include <common/integer.hpp>
#include <common/likely.hpp>
#include <common/meta.hpp>
#include <memory>

namespace Duality::core::arm {

/** Base class that memory systems must implement
  * in order to be connected to an ARM core.
  * Provides an uniform interface for ARM cores to
  * access memory.
  */
struct MemoryBase {
  enum class Bus : int {
    Code,
    Data,
    System
  };

  virtual auto ReadByte(u32 address, Bus bus) ->  u8 = 0;
  virtual auto ReadHalf(u32 address, Bus bus) -> u16 = 0;
  virtual auto ReadWord(u32 address, Bus bus) -> u32 = 0;
  virtual auto ReadQuad(u32 address, Bus bus) -> u64 = 0;
  
  virtual void WriteByte(u32 address, u8  value, Bus bus) = 0;
  virtual void WriteHalf(u32 address, u16 value, Bus bus) = 0;
  virtual void WriteWord(u32 address, u32 value, Bus bus) = 0;
  virtual void WriteQuad(u32 address, u64 value, Bus bus) = 0;

  template<typename T, Bus bus>
  auto FastRead(u32 address) -> T {
    static_assert(common::is_one_of_v<T, u8, u16, u32, u64>);

    address &= ~(sizeof(T) - 1);

    if constexpr (gEnableFastMemory && bus != Bus::System) {
      if (itcm.config.enable_read && address >= itcm.config.base && address <= itcm.config.limit) {
        return *reinterpret_cast<T*>(&itcm.data[(address - itcm.config.base) & itcm.mask]);
      }
    }

    if constexpr (gEnableFastMemory && bus == Bus::Data) {
      if (dtcm.config.enable_read && address >= dtcm.config.base && address <= dtcm.config.limit) {
        return *reinterpret_cast<T*>(&dtcm.data[(address - dtcm.config.base) & dtcm.mask]);
      }
    }

    if (gEnableFastMemory && likely(pagetable != nullptr)) {
      auto page = (*pagetable)[address >> kPageShift];
      if (likely(page != nullptr)) {
        page += address & kPageMask;
        return *reinterpret_cast<T*>(page);
      }
    }

    if constexpr (std::is_same_v<T,  u8>) return ReadByte(address, bus);
    if constexpr (std::is_same_v<T, u16>) return ReadHalf(address, bus);
    if constexpr (std::is_same_v<T, u32>) return ReadWord(address, bus);
    if constexpr (std::is_same_v<T, u64>) return ReadQuad(address, bus);
  }

  template<typename T, Bus bus>
  void FastWrite(u32 address, T value) {
    static_assert(common::is_one_of_v<T, u8, u16, u32, u64>);

    address &= ~(sizeof(T) - 1);

    if constexpr (gEnableFastMemory && bus != Bus::System) {
      if (itcm.config.enable && address >= itcm.config.base && address <= itcm.config.limit) {
        *reinterpret_cast<T*>(&itcm.data[(address - itcm.config.base) & itcm.mask]) = value;
        return;
      }

      if (dtcm.config.enable && address >= dtcm.config.base && address <= dtcm.config.limit) {
        *reinterpret_cast<T*>(&dtcm.data[(address - dtcm.config.base) & dtcm.mask]) = value;
        return;
      }
    }

    if (gEnableFastMemory && likely(pagetable != nullptr)) {
      auto page = (*pagetable)[address >> kPageShift];
      if (likely(page != nullptr)) {
        page += address & kPageMask;
        *reinterpret_cast<T*>(page) = value;
        return;
      }
    }

    if constexpr (std::is_same_v<T,  u8>) WriteByte(address, value, bus);
    if constexpr (std::is_same_v<T, u16>) WriteHalf(address, value, bus);
    if constexpr (std::is_same_v<T, u32>) WriteWord(address, value, bus);
    if constexpr (std::is_same_v<T, u64>) WriteQuad(address, value, bus);
  }

  static constexpr int kPageShift = 12; // 2^12 = 4096
  static constexpr int kPageMask = (1 << kPageShift) - 1;

  std::unique_ptr<std::array<u8*, 1048576>> pagetable = nullptr;

  struct TCM {
    u8* data = nullptr;
    u32 mask = 0;

    struct Config {
      bool enable = false;
      bool enable_read = false;
      u32 base = 0;
      u32 limit = 0;
    } config;
  } itcm, dtcm;
};

} // namespace Duality::core::arm
