/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <atom/panic.hpp>
#include <limits>
#include <cmath>

#include "math.hpp"

namespace lunar::nds {

Math::Math() {
  Reset();
}

void Math::Reset() {
  divcnt = {this};
  // TODO!
  sqrt_result = {};
}

auto Math::DIV::ReadByte(uint offset) -> u8 {
  if (offset >= 8) {
    ATOM_UNREACHABLE();
  }

  return value >> (offset * 8);
}

void Math::DIV::WriteByte(uint offset, u8 value) {
  if (offset >= 8) {
    ATOM_UNREACHABLE();
  }

  this->value &= ~(0xFFULL << (offset * 8));
  this->value |=  u64(value) << (offset * 8);

  math.UpdateDivision();
}

auto Math::SQRTCNT::ReadByte(uint offset) -> u8 {
  switch (offset) {
    case 0:
      return mode_64bit ? 1 : 0;
    case 1:
      return 0;
  }

  ATOM_UNREACHABLE();
}

void Math::SQRTCNT::WriteByte(uint offset, u8 value) {
  switch (offset) {
    case 0:
      mode_64bit = value & 1;
      break;
    case 1:
      break;
    default:
      ATOM_UNREACHABLE();
  }

  math.UpdateSquareRoot();
}

auto Math::SQRT_RESULT::ReadByte(uint offset) -> u8 {
  if (offset >= 4) {
    ATOM_UNREACHABLE();
  }

  return value >> (offset * 8);
}

auto Math::SQRT_PARAM::ReadByte(uint offset) -> u8 {
  if (offset >= 8) {
    ATOM_UNREACHABLE();
  }

  return value >> (offset * 8);
}

void Math::SQRT_PARAM::WriteByte(uint offset, u8 value) {
  if (offset >= 8) {
    ATOM_UNREACHABLE();
  }

  this->value &= ~(0xFFULL << (offset * 8));
  this->value |=  u64(value) << (offset * 8);

  math.UpdateSquareRoot();
}

void Math::UpdateDivision() {
  divcnt.error_divide_by_zero = div_denom.value == 0;

  switch (static_cast<DivisionMode>(divcnt.mode)) {
    case DivisionMode::Reserved:
    case DivisionMode::S32_S32: {
      s32 numer = s32(div_numer.value);
      s32 denom = s32(div_denom.value);

      if (numer == std::numeric_limits<s32>::min() && denom == -1) {
        // TODO: what should the remainder value actually be?
        div_result.value = u64(u32(std::numeric_limits<s32>::min()));
        div_remain.value = 0;
      } else if (denom == 0) {
        div_result.value = numer < 0 ? 0xFFFFFFFF00000001 : 0xFFFFFFFF;
        div_remain.value = numer;
      } else {
        div_result.value = u64(s64(numer / denom));
        div_remain.value = u64(s64(numer % denom));
      }
      break;
    }
    case DivisionMode::S64_S32:
    case DivisionMode::S64_S64: {
      s64 numer = s64(div_numer.value);
      s64 denom = divcnt.mode == DivisionMode::S64_S64 ? s64(div_denom.value) : s64(s32(div_denom.value));

      if (numer == std::numeric_limits<s64>::min() && denom == -1) {
        // TODO: what should the remainder value actually be?
        div_result.value = u64(std::numeric_limits<s64>::min());
        div_remain.value = 0;
      } else if (denom == 0) {
        div_result.value = numer < 0 ? 1 : 0xFFFFFFFFFFFFFFFF;
        div_remain.value = numer;
      } else {
        div_result.value = u64(numer / denom);
        div_remain.value = u64(numer % denom);
      }
      break;
    }
  }
}

void Math::UpdateSquareRoot() {
  // TODO: is there an efficient solution which does not require floats?
  if (sqrtcnt.mode_64bit) {
    sqrt_result.value = std::sqrt((long double)sqrt_param.value);
  } else {
    sqrt_result.value = std::sqrt(u32(sqrt_param.value));
  }
}

} // namespace lunar::nds
