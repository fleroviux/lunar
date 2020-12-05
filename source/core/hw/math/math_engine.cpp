/*
 * Copyright (C) 2020 fleroviux
 */

#include <common/log.hpp>
#include <limits>

#include "math_engine.hpp"

namespace fauxDS::core {

void MathEngine::Reset() {
  // TODO!
}

auto MathEngine::DIVCNT::ReadByte(uint offset) -> u8 {
  switch (offset) {
    case 0:
      return static_cast<u8>(mode);
    case 1:
      return error_divide_by_zero ? 64 : 0;
  }

  UNREACHABLE;
}

void MathEngine::DIVCNT::WriteByte(uint offset, u8 value) {
  switch (offset) {
    case 0:
      mode = static_cast<DivisionMode>(value & 3);
      break;
    case 1:
      // TODO: I'm not sure if the error is supposed to be acknowledgable.
      //error_divide_by_zero &= value & 64;
      break;
    default:
      UNREACHABLE;
  }

  math_engine.UpdateDivision();
}

auto MathEngine::DIV::ReadByte(uint offset) -> u8 {
  if (offset >= 8) {
    UNREACHABLE;
  }

  return value >> (offset * 8);
}

void MathEngine::DIV::WriteByte(uint offset, u8 value) {
  if (offset >= 8) {
    UNREACHABLE;
  }

  this->value &= ~(0xFFULL << (offset * 8));
  this->value |=  u64(value) << (offset * 8);

  if (offset == 7)
  math_engine.UpdateDivision();
}

void MathEngine::UpdateDivision() {
  divcnt.error_divide_by_zero = div_denom.value == 0;

  switch (divcnt.mode) {
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

} // namespace fauxDS::core
