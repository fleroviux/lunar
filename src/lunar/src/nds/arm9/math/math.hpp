/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <atom/bit.hpp>
#include <atom/integer.hpp>

namespace lunar::nds {

// Hardware accelerated 64-bit division and square root engine.
class Math {
  public:
    enum class DivisionMode {
      S32_S32 = 0,
      S64_S32 = 1,
      S64_S64 = 2,
      Reserved = 3
    };

    Math();

    void Reset();

    struct DIVCNT {
      Math* self{};

      union {
        atom::Bits<0, 2, u16> mode;
        atom::Bits<14, 1, u16> error_divide_by_zero;
        u16 half = 0u;
      };

      u16 ReadHalf() const {
        return half;
      }

      void WriteHalf(u16 value) {
        half = (half & 0xFFFCu) | (value & 3u);
        self->UpdateDivision();
      }
    } divcnt;

    struct DIV {
      DIV(Math& math) : math(math) {}

      auto ReadByte (uint offset) -> u8;
      void WriteByte(uint offset, u8 value);
    private:
      friend struct lunar::nds::Math;

      u64 value = 0;
      Math& math;
    };

    DIV div_numer { *this };
    DIV div_denom { *this };
    DIV div_result { *this };
    DIV div_remain { *this };

    struct SQRTCNT {
      SQRTCNT(Math& math) : math(math) {}

      auto ReadByte (uint offset) -> u8;
      void WriteByte(uint offset, u8 value);
    private:
      friend struct lunar::nds::Math;

      bool mode_64bit = false;
      Math& math;
    } sqrtcnt { *this };

    struct SQRT_RESULT {
      auto ReadByte(uint offset) -> u8;

    private:
      friend struct lunar::nds::Math;

      u32 value = 0;
    } sqrt_result;

    struct SQRT_PARAM {
      SQRT_PARAM(Math& math) : math(math) {}

      auto ReadByte (uint offset) -> u8;
      void WriteByte(uint offset, u8 value);

    private:
      friend struct lunar::nds::Math;

      u64 value = 0;
      Math& math;
    } sqrt_param { *this };

  private:
    void UpdateDivision();
    void UpdateSquareRoot();
};

}; // namespace lunar::nds
