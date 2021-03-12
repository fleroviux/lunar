/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <util/integer.hpp>

namespace Duality::Core::arm {

struct Coprocessor {
  virtual void Reset() = 0;

  /**
    * Read coprocessor register.
    *
    * @param  opcode1  Opcode #1
    * @param  cn       Coprocessor Register #1
    * @param  cm       Coprocessor Register #2
    * @param  opcode2  Opcode #2
    *
    * @returns the value contained in the coprocessor register.
    */
  virtual auto Read(int opcode1, int cn, int cm, int opcode2) -> u32 = 0;

  /**
    * Write to coprocessor register.
    *
    * @param  opcode1  Opcode #1
    * @param  cn       Coprocessor Register #1
    * @param  cm       Coprocessor Register #2
    * @param  opcode2  Opcode #2
    * @param  value    Word to write
    */
  virtual void Write(int opcode1, int cn, int cm, int opcode2, u32 value) = 0;
};

} // namespace Duality::Core::arm
