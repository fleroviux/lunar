/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <array>
#include <buildconfig.hpp>
#include <common/integer.hpp>
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
    Data
  };

  virtual auto ReadByte(u32 address, Bus bus) ->  u8 = 0;
  virtual auto ReadHalf(u32 address, Bus bus) -> u16 = 0;
  virtual auto ReadWord(u32 address, Bus bus) -> u32 = 0;
  
  virtual void WriteByte(u32 address, u8  value) = 0;
  virtual void WriteHalf(u32 address, u16 value) = 0;
  virtual void WriteWord(u32 address, u32 value) = 0;

  template<typename T>
  auto FastRead(u32 address, Bus bus) -> T {
    static_assert(common::is_one_of_v<T, u8, u16, u32>);

    address &= ~(sizeof(T) - 1);

    if (gEnableFastMemory && pagetable != nullptr) {
      auto page = (*pagetable)[address >> kPageShift];
      if (page != nullptr) {
        page += address & kPageMask;
        return *reinterpret_cast<T*>(page);
      }
    }

    if constexpr (std::is_same_v<T,  u8>) return ReadByte(address, bus);
    if constexpr (std::is_same_v<T, u16>) return ReadHalf(address, bus);
    if constexpr (std::is_same_v<T, u32>) return ReadWord(address, bus);
  }

  template<typename T>
  void FastWrite(u32 address, T value) {
    static_assert(common::is_one_of_v<T, u8, u16, u32>);

    address &= ~(sizeof(T) - 1);

    if (gEnableFastMemory && pagetable != nullptr) {
      auto page = (*pagetable)[address >> kPageShift];
      if (page != nullptr) {
        page += address & kPageMask;
        *reinterpret_cast<T*>(page) = value;
        return;
      }
    }

    if constexpr (std::is_same_v<T,  u8>) WriteByte(address, value);
    if constexpr (std::is_same_v<T, u16>) WriteHalf(address, value);
    if constexpr (std::is_same_v<T, u32>) WriteWord(address, value);
  }

  static constexpr int kPageShift = 12; // 2^12 = 4096
  static constexpr int kPageMask = (1 << kPageShift) - 1;

  std::unique_ptr<std::array<u8*, 1048576>> pagetable = nullptr;
};

} // namespace Duality::core::arm
