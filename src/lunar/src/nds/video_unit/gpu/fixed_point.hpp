
// Copyright (C) 2022 fleroviux

#pragma once

#include <lunar/integer.hpp>

namespace lunar {

namespace nds {

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

} // namespace lunar::nds::detail

using Fixed20x12 = detail::FixedBase<s32, s64, 12>;
using Fixed12x4 = detail::FixedBase<s16, s32, 4>;

namespace detail {
inline auto operator*(Fixed12x4 lhs, Fixed20x12 rhs) -> Fixed12x4 {
  return Fixed12x4{s16((Fixed20x12{lhs.raw()} * rhs).raw())};
}
} // namespace lunar::nds::detail

} // namespace lunar::nds

template<>
struct NumericConstants<lunar::nds::Fixed20x12> {
  static constexpr auto zero() -> lunar::nds::Fixed20x12 {
    return lunar::nds::Fixed20x12{};
  }
  
  static constexpr auto one() -> lunar::nds::Fixed20x12 {
    return lunar::nds::Fixed20x12{1 << 12};
  }
};

template<>
struct NumericConstants<lunar::nds::Fixed12x4> {
  static constexpr auto zero() -> lunar::nds::Fixed12x4 {
    return lunar::nds::Fixed12x4{};
  }
  
  static constexpr auto one() -> lunar::nds::Fixed12x4 {
    return lunar::nds::Fixed12x4{1 << 4};
  }
};

} // namespace lunar