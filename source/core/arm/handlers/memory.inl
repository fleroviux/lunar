/*
 * Copyright (C) 2020 fleroviux
 */

using Bus = MemoryBase::Bus;

auto ReadByte(u32 address) -> u32 {
  return memory->FastRead<u8>(address, Bus::Data);
}

auto ReadHalf(u32 address) -> u32 {
  return memory->FastRead<u16>(address, Bus::Data);
}

auto ReadWord(u32 address) -> u32 {
  return memory->FastRead<u32>(address, Bus::Data);
}

auto ReadHalfCode(u32 address) -> u32 {
  return memory->FastRead<u16>(address, Bus::Code);
}

auto ReadWordCode(u32 address) -> u32 {
  return memory->FastRead<u32>(address, Bus::Code);
}

auto ReadByteSigned(u32 address) -> u32 {
  u32 value = memory->FastRead<u8>(address, Bus::Data);

  if (value & 0x80) {
    value |= 0xFFFFFF00;
  }

  return value;
}

auto ReadHalfMaybeRotate(u32 address) -> u32 {
  u32 value = memory->FastRead<u16>(address, Bus::Data);
  
  if ((address & 1) && arch == Architecture::ARMv4T) {
    value = (value >> 8) | (value << 24);
  }
  
  return value;
}

auto ReadHalfSigned(u32 address) -> u32 {
  if ((address & 1) && arch == Architecture::ARMv4T) {
    return ReadByteSigned(address);
  }

  u32 value = memory->FastRead<u16>(address, Bus::Data);
  if (value & 0x8000) {
    return value | 0xFFFF0000;
  }
  return value;
}

auto ReadWordRotate(u32 address) -> u32 {
  auto value = memory->FastRead<u32>(address, Bus::Data);
  auto shift = (address & 3) * 8;
  
  return (value >> shift) | (value << (32 - shift));
}

void WriteByte(u32 address, u8  value) {
  memory->FastWrite<u8>(address, value);
}

void WriteHalf(u32 address, u16 value) {
  memory->FastWrite<u16>(address, value);
}

void WriteWord(u32 address, u32 value) {
  memory->FastWrite<u32>(address, value);
}
