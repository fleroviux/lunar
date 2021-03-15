/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <algorithm>
#include <util/math/vector.hpp>
#include <util/integer.hpp>

#include "fixed_point.hpp"

namespace Duality::Core {

using Fixed9 = detail::FixedBase<int, int, 9>;

namespace detail {

inline auto operator*(Fixed9 lhs, Fixed20x12 rhs) -> Fixed9 {
  return Fixed9{int((s64(lhs.raw()) * rhs.raw()) >> 12)};
}

} // namespace detail

struct Color4 : Vector<Color4, Fixed9, 4> {
  Color4() {
    for (uint i = 0; i < 4; i++)
      this->data[i] = Fixed9{511};
  }

  Color4(Fixed9 r, Fixed9 g, Fixed9 b) {
    this->data[0] = r;
    this->data[1] = g;
    this->data[2] = b;
    this->data[3] = Fixed9{511};
  }

  Color4(Fixed9 r, Fixed9 g, Fixed9 b, Fixed9 a) {
    this->data[0] = r;
    this->data[1] = g;
    this->data[2] = b;
    this->data[3] = a;
  }

  Color4(Color4 const& other) {
    for (uint i = 0; i < 4; i++)
      this->data[i] = other[i];
  }

  static auto from_rgb555(u16 color) -> Color4 {
    auto r = (color >>  0) & 31;
    auto g = (color >>  5) & 31;
    auto b = (color >> 10) & 31;
    
    // TODO: confirm that the hardware expands the 5-bit value to 9-bit like this.
    return Color4{
      u16((r << 4) | (r >> 1)),
      u16((g << 4) | (g >> 1)),
      u16((b << 4) | (b >> 1)),
      511
    };
  }

  auto to_rgb555() const -> u16 {
    return (r().raw() >> 4) |
          ((g().raw() >> 4) <<  5) |
          ((b().raw() >> 4) << 10);
  }

  auto operator*(Fixed9 other) const -> Color4 {
    Color4 result{};
    for (uint i = 0; i < 4; i++)
      result[i] = data[i] * other;
    return result;
  }

  auto operator*=(Fixed9 other) -> Color4& {
    for (uint i = 0; i < 4; i++)
      data[i] *= other;
    return *this;
  }

  auto operator*(Color4 const& other) const -> Color4 {
    Color4 result{};
    for (uint i = 0; i < 4; i++)
      result[i] = data[i] * other[i];
    return result;
  }

  auto operator*=(Color4 const& other) -> Color4& {
    for (uint i = 0; i < 4; i++)
      data[i] *= other[i];
    return *this;
  }

  auto r() -> Fixed9& { return this->data[0]; }
  auto g() -> Fixed9& { return this->data[1]; }
  auto b() -> Fixed9& { return this->data[2]; }
  auto a() -> Fixed9& { return this->data[3]; }

  auto r() const -> Fixed9 { return this->data[0]; }
  auto g() const -> Fixed9 { return this->data[1]; }
  auto b() const -> Fixed9 { return this->data[2]; }
  auto a() const -> Fixed9 { return this->data[3]; }
};

} // namespace Duality::Core

template<>
struct NumericConstants<Duality::Core::Fixed9> {
  static constexpr auto zero() -> Duality::Core::Fixed9 {
    return Duality::Core::Fixed9{};
  }
  
  static constexpr auto one() -> Duality::Core::Fixed9 {
    return Duality::Core::Fixed9{511};
  }
};