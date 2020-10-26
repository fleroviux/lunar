/*
 * Copyright (C) 2020 fleroviux
 */

#include <functional>

#include "cp15.hpp"

namespace fauxDS::core {

CP15::CP15(ARM9MemoryBus* bus) : bus(bus) {
  for (int i = 0; i <= 0x7FF; i++) {
    handler_rd[i] = &CP15::DefaultRead;
    handler_wr[i] = &CP15::DefaultWrite;
  }

  RegisterHandler(0, 0, 0, &CP15::ReadMainID);
  RegisterHandler(0, 0, 1, &CP15::ReadCacheType);
  RegisterHandler(7, 0, 4, &CP15::WriteWaitForIRQ);
  RegisterHandler(7, 8, 2, &CP15::WriteWaitForIRQ);
  RegisterHandler(9, 1, 0, &CP15::ReadDTCMConfig);
  RegisterHandler(9, 1, 1, &CP15::ReadITCMConfig);
  RegisterHandler(9, 1, 0, &CP15::WriteDTCMConfig);
  RegisterHandler(9, 1, 1, &CP15::WriteITCMConfig);
}

void CP15::Reset() {
  // Reset DTCM and ITCM configuration.
  // TODO: what are the actual values upon boot?
  Write(0, 9, 1, 0, 0x0080000A);
  Write(0, 9, 1, 1, 0x0000000C);
}

void CP15::RegisterHandler(int cn, int cm, int opcode, ReadHandler handler) {
  handler_rd[Index(cn, cm, opcode)] = handler;
}

void CP15::RegisterHandler(int cn, int cm, int opcode, WriteHandler handler) {
  handler_wr[Index(cn, cm, opcode)] = handler;
}

auto CP15::Read(int opcode1,
                int cn,
                int cm,
                int opcode2) -> u32 {
  if (opcode1 == 0) {
    return std::invoke(handler_rd[Index(cn, cm, opcode2)], this, cn, cm, opcode2);
  }

  return 0;
}

void CP15::Write(int opcode1,
                 int cn,
                 int cm,
                 int opcode2,
                 u32 value) {
  if (opcode1 == 0) {
    std::invoke(handler_wr[Index(cn, cm, opcode2)], this, cn, cm, opcode2, value);
  }
}

auto CP15::ReadMainID(int cn, int cm, int opcode) -> u32 {
  return 0x41059461;
}

auto CP15::ReadCacheType(int cn, int cm, int opcode) -> u32 {
  return 0x0F0D2112;
}

void CP15::WriteWaitForIRQ(int cn, int cm, int opcode, u32 value) {
  LOG_ERROR("CP15: unhandled wait for interrupt!");
}

auto CP15::ReadDTCMConfig(int cn, int cm, int opcode) -> u32 {
  return dtcm.value;
}

auto CP15::ReadITCMConfig(int cn, int cm, int opcode) -> u32 {
  return itcm.value;
}

void CP15::WriteDTCMConfig(int cn, int cm, int opcode, u32 value) {
  auto base = value & 0xFFFFF000;
  auto size = (value >> 1) & 0x1F;
  if (size < 3 || size > 23) {
    LOG_ERROR("CP15: DTCM virtual size must be between 4 KiB (3) and 4 GiB (23)");
  }
  dtcm.value = value;
  dtcm.base = base;
  dtcm.limit = base + (512 << size) - 1;
  if (dtcm.limit < dtcm.base) {
    LOG_ERROR("CP15: DTCM limit is lower than base address!");
  }
  LOG_INFO("CP15: DTCM mapped @ 0x{0:08X} - 0x{1:08X}", dtcm.base, dtcm.limit);
  bus->SetDTCM(dtcm.base, dtcm.limit);
}

void CP15::WriteITCMConfig(int cn, int cm, int opcode, u32 value) {
  auto base = value & 0xFFFFF000;
  if (base != 0) {
    base = 0;
    value &= 0xFFF;
    LOG_ERROR("CP15: ITCM base address is not writable on the Nintendo DS!");
  }
  auto size = (value >> 1) & 0x1F;
  if (size < 3 || size > 23) {
    LOG_ERROR("CP15: ITCM virtual size must be between 4 KiB (3) and 4 GiB (23)");
  }
  itcm.value = value;
  itcm.base = 0;
  itcm.limit = base + (512 << size) - 1;
  if (itcm.limit < itcm.base) {
    LOG_ERROR("CP15: ITCM limit is lower than base address!");
  }
  LOG_INFO("CP15: ITCM mapped @ 0x{0:08X} - 0x{1:08X}", itcm.base, itcm.limit);
  bus->SetITCM(itcm.base, itcm.limit);
}

} // namespace fauxDS::core
