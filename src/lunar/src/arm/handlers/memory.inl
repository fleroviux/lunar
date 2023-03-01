/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

using Bus = aura::arm::Memory::Bus;

auto ReadByte(u32 address) -> u32 {
  return memory->ReadByte(address, Bus::Data);
}

auto ReadHalf(u32 address) -> u32 {
  return memory->ReadHalf(address, Bus::Data);
}

auto ReadWord(u32 address) -> u32 {
  return memory->ReadWord(address, Bus::Data);
}

auto ReadHalfCode(u32 address) -> u32 {
  return memory->ReadHalf(address, Bus::Code);
}

auto ReadWordCode(u32 address) -> u32 {
  return memory->ReadWord(address, Bus::Code);
}

auto ReadByteSigned(u32 address) -> u32 {
  u32 value = memory->ReadByte(address, Bus::Data);

  if (value & 0x80) {
    value |= 0xFFFFFF00;
  }

  return value;
}

auto ReadHalfMaybeRotate(u32 address) -> u32 {
  u32 value = memory->ReadHalf(address, Bus::Data);
  
  if ((address & 1) && arch == Architecture::ARMv4T) {
    value = (value >> 8) | (value << 24);
  }
  
  return value;
}

auto ReadHalfSigned(u32 address) -> u32 {
  if ((address & 1) && arch == Architecture::ARMv4T) {
    return ReadByteSigned(address);
  }

  u32 value = memory->ReadHalf(address, Bus::Data);
  if (value & 0x8000) {
    return value | 0xFFFF0000;
  }
  return value;
}

auto ReadWordRotate(u32 address) -> u32 {
  auto value = memory->ReadWord(address, Bus::Data);
  auto shift = (address & 3) * 8;
  
  return (value >> shift) | (value << (32 - shift));
}

void WriteByte(u32 address, u8  value) {
  memory->WriteByte(address, value, Bus::Data);
}

void WriteHalf(u32 address, u16 value) {
  memory->WriteHalf(address, value, Bus::Data);
}

void WriteWord(u32 address, u32 value) {
  memory->WriteWord(address, value, Bus::Data);
}
