/*
 * Copyright (C) 2020 fleroviux
 */

using Bus = MemoryBase::Bus;

auto ReadByte(u32 address) -> u32 {
  return memory->ReadByte(address, Bus::Data);
}

auto ReadHalf(u32 address) -> u32 {
  return memory->ReadHalf(address & ~1, Bus::Data);
}

auto ReadWord(u32 address) -> u32 {
  return memory->ReadWord(address & ~3, Bus::Data);
}

auto ReadHalfCode(u32 address) -> u32 {
  return memory->ReadHalf(address & ~1, Bus::Code);
}

auto ReadWordCode(u32 address) -> u32 {
  return memory->ReadWord(address & ~3, Bus::Code);
}

auto ReadByteSigned(u32 address) -> u32 {
  u32 value = memory->ReadByte(address, Bus::Data);

  if (value & 0x80) {
    value |= 0xFFFFFF00;
  }

  return value;
}

auto ReadHalfMaybeRotate(u32 address) -> u32 {
  u32 value = memory->ReadHalf(address & ~1, Bus::Data);
  
  if ((address & 1) && arch == Architecture::ARMv4T) {
    value = (value >> 8) | (value << 24);
  }
  
  return value;
}

auto ReadHalfSigned(u32 address) -> u32 {
  if ((address & 1) && arch == Architecture::ARMv4T) {
    return ReadByteSigned(address);
  }

  u32 value = memory->ReadHalf(address & ~1, Bus::Data);
  if (value & 0x8000) {
    return value | 0xFFFF0000;
  }
  return value;
}

auto ReadWordRotate(u32 address) -> u32 {
  auto value = memory->ReadWord(address & ~3, Bus::Data);
  auto shift = (address & 3) * 8;
  
  return (value >> shift) | (value << (32 - shift));
}

void WriteByte(u32 address, u8  value) {
  memory->WriteByte(address, value);
}

void WriteHalf(u32 address, u16 value) {
  memory->WriteHalf(address & ~1, value);
}

void WriteWord(u32 address, u32 value) {
  memory->WriteWord(address & ~3, value);
}
