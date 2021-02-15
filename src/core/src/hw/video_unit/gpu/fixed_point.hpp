/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <util/integer.hpp>

namespace Duality::core {

struct Fixed20x12 {
  constexpr Fixed20x12() {}
  constexpr Fixed20x12(s32 value) : value(value) {}

  static auto from_int(int value) -> Fixed20x12 {
    return { s32(value) << 12 };
  }

  auto integer() -> s32 { return value >> 12; }
  auto raw() -> s32 { return value; }
  auto absolute() -> Fixed20x12 { return value < 0 ? -value : value; }

  auto operator+(Fixed20x12 other) const -> Fixed20x12 {
    return value + other.value;
  }

  auto operator-(Fixed20x12 other) const -> Fixed20x12 {
    return value - other.value;
  }

  auto operator*(Fixed20x12 other) const -> Fixed20x12 {
    return s32((s64(value) * s64(other.value)) >> 12);
  }

  auto operator/(Fixed20x12 other) const -> Fixed20x12 {
    return s32((s64(value) << 12) / s64(other.value));
  }

  auto operator+=(Fixed20x12 other) -> Fixed20x12& {
    value += other.value;
    return *this;
  }

  auto operator-=(Fixed20x12 other) -> Fixed20x12& {
    value -= other.value;
    return *this;
  }

  auto operator*=(Fixed20x12 other) -> Fixed20x12& {
    value = s32((s64(value) * s64(other.value)) >> 12);
    return *this;
  }

  auto operator/=(Fixed20x12 other) -> Fixed20x12 {
    value = s32((s64(value) << 12) / s64(other.value));
    return *this;
  }

  auto operator-() const -> Fixed20x12 {
    return -value;
  }

  bool operator==(Fixed20x12 other) {
    return value == other.value;
  }

  bool operator!=(Fixed20x12 other) {
    return value != other.value;
  }

  bool operator<=(Fixed20x12 other) {
    return value <= other.value;
  }

  bool operator>=(Fixed20x12 other) {
    return value >= other.value;
  }

  bool operator<(Fixed20x12 other) {
    return value < other.value;
  }

  bool operator>(Fixed20x12 other) {
    return value > other.value;
  }

private:
  s32 value = 0;
};

} // namespace Duality::core

template<>
struct NumericConstants<Duality::core::Fixed20x12> {
  static constexpr auto zero() -> Duality::core::Fixed20x12 {
    return Duality::core::Fixed20x12{};
  }
  
  static constexpr auto one() -> Duality::core::Fixed20x12 {
    return Duality::core::Fixed20x12{1 << 12};
  }
};
