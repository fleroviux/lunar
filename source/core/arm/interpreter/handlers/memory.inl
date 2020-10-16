/*
 * Copyright (C) 2020 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

using Bus = MemoryBase::Bus;

// TODO: for now we allow all memory accesses to be unaligned.
// This does not appear to be true in all cases and definitely
// is not true in the case of the ARM9 (ARMv5TE) processor.
// Also ReadWordRotate/ReadHalfRotate aren't really accurate names
// when we allow the accesses to be unaligned which ARMv6K does support.
// We should probably rename both methods to something that describes
// what kind/class of read access this is instead.

auto ReadByte(std::uint32_t address) -> std::uint32_t {
  return memory->ReadByte(address, Bus::Data, core);
}

auto ReadHalf(std::uint32_t address) -> std::uint32_t {
  return memory->ReadHalf(address & ~1, Bus::Data, core);
}

auto ReadWord(std::uint32_t address) -> std::uint32_t {
  return memory->ReadWord(address & ~3, Bus::Data, core);
}

auto ReadHalfCode(std::uint32_t address) -> std::uint32_t {
  return memory->ReadHalf(address & ~1, Bus::Code, core);
}

auto ReadWordCode(std::uint32_t address) -> std::uint32_t {
  return memory->ReadWord(address & ~3, Bus::Code, core);
}

auto ReadByteSigned(std::uint32_t address) -> std::uint32_t {
  std::uint32_t value = memory->ReadByte(address, Bus::Data, core);

  if (value & 0x80) {
    value |= 0xFFFFFF00;
  }

  return value;
}

auto ReadHalfRotate(std::uint32_t address) -> std::uint32_t {
  std::uint32_t value = memory->ReadHalf(address & ~1, Bus::Data, core);
  
  if (address & 1) {
    value = (value >> 8) | (value << 24);
  }
  
  return value;
}

auto ReadHalfSigned(std::uint32_t address) -> std::uint32_t {
  std::uint32_t value = memory->ReadHalf(address & ~1, Bus::Data, core);

  if (value & 0x8000) {
    value |= 0xFFFF0000;
  }

  return value;
}

auto ReadWordRotate(std::uint32_t address) -> std::uint32_t {
  auto value = memory->ReadWord(address & ~3, Bus::Data, core);
  auto shift = (address & 3) * 8;
  
  return (value >> shift) | (value << (32 - shift));
}

void WriteByte(std::uint32_t address, std::uint8_t  value) {
  memory->WriteByte(address, value, core);
}

void WriteHalf(std::uint32_t address, std::uint16_t value) {
  memory->WriteHalf(address & ~1, value, core);
}

void WriteWord(std::uint32_t address, std::uint32_t value) {
  memory->WriteWord(address & ~3, value, core);
}
