/*
 * Copyright (C) 2020 fleroviux
 *
 * Licensed under GPLv3 or any later version.
 * Refer to the included LICENSE file.
 */

#include "cp15.hpp"

#include <functional>

using namespace fauxDS::core::arm;

CP15::CP15(int core, MemoryBase* memory) : cpu_id(core), memory(memory) {
  for (int i = 0; i <= 0x7FF; i++) {
    handler_rd[i] = &CP15::DefaultRead;
    handler_wr[i] = &CP15::DefaultWrite;
  }

  RegisterHandler(0, 0, 5, &CP15::ReadCPUID);
}

void CP15::Reset() {
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
                int opcode2) -> std::uint32_t {
  if (opcode1 == 0) {
    return std::invoke(handler_rd[Index(cn, cm, opcode2)], this, cn, cm, opcode2);
  }

  return 0;
}

void CP15::Write(int opcode1,
                 int cn,
                 int cm,
                 int opcode2,
                 std::uint32_t value) {
  if (opcode1 == 0) {
    std::invoke(handler_wr[Index(cn, cm, opcode2)], this, cn, cm, opcode2, value);
  }
}
