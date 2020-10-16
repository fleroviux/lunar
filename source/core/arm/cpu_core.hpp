/*
 * Copyright (C) 2020 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

#pragma once

namespace fauxDS::core::arm {

/// A base class for all ARM emulators.
class CPUCoreBase {
public:
  enum class Architecture {
    ARMv5TE,
    ARMv6K
  };

  virtual void Reset() = 0;
  virtual void Run(int instructions) = 0;
  virtual void SignalIRQ() = 0;
  virtual auto GetPC() -> std::uint32_t = 0;
  virtual void SetPC(std::uint32_t value) = 0;
  virtual bool IsInPrivilegedMode() = 0;

  auto ExceptionBase() -> std::uint32_t const { return exception_base; }
  void ExceptionBase(std::uint32_t base) { exception_base = base; } 

  // TODO: add virtual methods to enable state read/write access?

  // TODO: remove this hack.
  bool hit_unimplemented_or_undefined = false;

private:
  friend class MemoryBase;

  // TODO: store exception base configuration in CP15.
  std::uint32_t exception_base = 0;
};

} // namespace fauxDS::core::arm
