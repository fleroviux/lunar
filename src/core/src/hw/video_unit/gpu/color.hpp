/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <algorithm>
#include <util/math/vector.hpp>
#include <util/integer.hpp>

#include "fixed_point.hpp"

namespace Duality::Core {

using Fixed6 = detail::FixedBase<s8, int, 6>;

namespace detail {

inline auto operator*(Fixed6 lhs, Fixed20x12 rhs) -> Fixed6 {
  return Fixed6{s8((s64(lhs.raw()) * rhs.raw()) >> 12)};
}

} // namespace detail

struct Color4 : Vector<Color4, Fixed6, 4> {
  Color4() {
    for (uint i = 0; i < 4; i++)
      this->data[i] = Fixed6{63};
  }

  Color4(Fixed6 r, Fixed6 g, Fixed6 b) {
    this->data[0] = r;
    this->data[1] = g;
    this->data[2] = b;
    this->data[3] = Fixed6{63};
  }

  Color4(Fixed6 r, Fixed6 g, Fixed6 b, Fixed6 a) {
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
    auto r = (color << 1) & 62;
    auto g = (color >> 4) & 62;
    auto b = (color >> 9) & 62;
    
    return Color4{
      s8(r | (r >> 5)),
      s8(g | (g >> 5)),
      s8(b | (b >> 5)),
      s8(63)
    };
  }

  auto to_rgb555() const -> u16 {
    return (r().raw() >> 1) |
          ((g().raw() >> 1) <<  5) |
          ((b().raw() >> 1) << 10);
  }

  auto operator*(Fixed6 other) const -> Color4 {
    Color4 result{};
    for (uint i = 0; i < 4; i++)
      result[i] = data[i] * other;
    return result;
  }

  auto operator*=(Fixed6 other) -> Color4& {
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

  auto r() -> Fixed6& { return this->data[0]; }
  auto g() -> Fixed6& { return this->data[1]; }
  auto b() -> Fixed6& { return this->data[2]; }
  auto a() -> Fixed6& { return this->data[3]; }

  auto r() const -> Fixed6 { return this->data[0]; }
  auto g() const -> Fixed6 { return this->data[1]; }
  auto b() const -> Fixed6 { return this->data[2]; }
  auto a() const -> Fixed6 { return this->data[3]; }
};

} // namespace Duality::Core

template<>
struct NumericConstants<Duality::Core::Fixed6> {
  static constexpr auto zero() -> Duality::Core::Fixed6 {
    return Duality::Core::Fixed6{};
  }
  
  static constexpr auto one() -> Duality::Core::Fixed6 {
    return Duality::Core::Fixed6{63};
  }
};
