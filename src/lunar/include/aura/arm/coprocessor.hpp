
#pragma once

#include <atom/integer.hpp>

namespace aura::arm {

class Coprocessor {
  public:
    virtual ~Coprocessor() = default;

    virtual void Reset() {}

    virtual u32  MRC(int opc1, int cn, int cm, int opc2) = 0;
    virtual void MCR(int opc1, int cn, int cm, int opc2, u32 value);
};

} // namespace aura::arm
