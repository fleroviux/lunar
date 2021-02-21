/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <util/integer.hpp>

namespace Duality::core {

namespace detail {

template<typename T, typename U, uint shift>
struct FixedBase {
  constexpr FixedBase() {}
  constexpr FixedBase(T value) : value(value) {}

  static auto from_int(int value) -> FixedBase {
    return { T(value) << shift };
  }

  auto integer() const -> T { return value >> shift; }
  auto raw() const -> T { return value; }
  auto absolute() const -> FixedBase { return value < 0 ? -value : value; }

  auto operator+(FixedBase other) const -> FixedBase {
    return value + other.value;
  }

  auto operator-(FixedBase other) const -> FixedBase {
    return value - other.value;
  }

  auto operator*(FixedBase<T, U, shift> other) const -> FixedBase {
    return T((U(value) * U(other.value)) >> shift);
  }

  auto operator/(FixedBase other) const -> FixedBase {
    return T((U(value) << shift) / U(other.value));
  }

  auto operator+=(FixedBase other) -> FixedBase& {
    value += other.value;
    return *this;
  }

  auto operator-=(FixedBase other) -> FixedBase& {
    value -= other.value;
    return *this;
  }

  auto operator*=(FixedBase other) -> FixedBase& {
    value = T((U(value) * U(other.value)) >> shift);
    return *this;
  }

  auto operator/=(FixedBase other) -> FixedBase {
    value = T((U(value) << shift) / U(other.value));
    return *this;
  }

  auto operator-() const -> FixedBase {
    return -value;
  }

  bool operator==(FixedBase other) const {
    return value == other.value;
  }

  bool operator!=(FixedBase other) const {
    return value != other.value;
  }

  bool operator<=(FixedBase other) const {
    return value <= other.value;
  }

  bool operator>=(FixedBase other) const {
    return value >= other.value;
  }

  bool operator<(FixedBase other) const {
    return value < other.value;
  }

  bool operator>(FixedBase other) const {
    return value > other.value;
  }

private:
  T value {};
};

} // namespace Duality::core::detail

using Fixed20x12 = detail::FixedBase<s32, s64, 12>;
using Fixed12x4 = detail::FixedBase<s16, s32, 4>;

namespace detail {
inline auto operator*(Fixed12x4 lhs, Fixed20x12 rhs) -> Fixed12x4 {
  return Fixed12x4{s16((Fixed20x12{lhs.raw()} * rhs).raw())};
}
} // namespace Duality::core::detail

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

template<>
struct NumericConstants<Duality::core::Fixed12x4> {
  static constexpr auto zero() -> Duality::core::Fixed12x4 {
    return Duality::core::Fixed12x4{};
  }
  
  static constexpr auto one() -> Duality::core::Fixed12x4 {
    return Duality::core::Fixed12x4{1 << 4};
  }
};
