/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <algorithm>
#include <util/math/vector.hpp>
#include <util/integer.hpp>

namespace Duality::Core {

namespace detail {

struct ColorComponent {
  constexpr ColorComponent() {}
  constexpr ColorComponent(u16 value) : value(value) {}

  auto raw() const -> u16 { return value; }

  auto operator+(ColorComponent other) const -> ColorComponent {
    return saturate_up(value + other.value);
  }

  auto operator-(ColorComponent other) const -> ColorComponent {
    return saturate_down(value - other.value);
  }

  auto operator*(ColorComponent other) const -> ColorComponent {
    return u16((u32(value) * u32(other.value)) >> 9);
  }

  auto operator+=(ColorComponent other) -> ColorComponent& {
    value = saturate_up(value + other.value);
    return *this;
  }

  auto operator-=(ColorComponent other) -> ColorComponent& {
    value = saturate_down(value - other.value);
    return *this;
  }

  auto operator*=(ColorComponent other) -> ColorComponent& {
    value = u16((u32(value) * u32(other.value)) >> 9);
    return *this;
  }

  bool operator==(ColorComponent other) const {
    return value == other.value;
  }

  bool operator!=(ColorComponent other) const {
    return value != other.value;
  }

  bool operator<=(ColorComponent other) const {
    return value <= other.value;
  }

  bool operator>=(ColorComponent other) const {
    return value >= other.value;
  }

  bool operator<(ColorComponent other) const {
    return value < other.value;
  }

  bool operator>(ColorComponent other) const {
    return value > other.value;
  }

private:
  static auto saturate_up(u16 value) -> u8 {
    return std::min(value, u16(511));
  }

  static auto saturate_down(u16 value) -> u8 {
    return std::max(value, u16(0));
  }

  u16 value {};
};

} // namespace Duality::Core::detail

struct Color4 : Vector<detail::ColorComponent, 4> {
  using value_type = detail::ColorComponent; 

  Color4() {}

  Color4(
    value_type r,
    value_type g,
    value_type b
  ) {
    this->data[0] = r;
    this->data[1] = g;
    this->data[2] = b;
    this->data[3] = value_type{63};
  }

  Color4(
    value_type r,
    value_type g,
    value_type b,
    value_type a
  ) {
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

  auto to_rgb555() -> u16 {
    return (r().raw() >> 4) |
          ((g().raw() >> 4) <<  5) |
          ((b().raw() >> 4) << 10);
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

  auto r() -> value_type& { return this->data[0]; }
  auto g() -> value_type& { return this->data[1]; }
  auto b() -> value_type& { return this->data[2]; }
  auto a() -> value_type& { return this->data[3]; }

  auto r() const -> value_type { return this->data[0]; }
  auto g() const -> value_type { return this->data[1]; }
  auto b() const -> value_type { return this->data[2]; }
  auto a() const -> value_type { return this->data[3]; }
};

} // namespace Duality::Core

template<>
struct NumericConstants<Duality::Core::detail::ColorComponent> {
  static constexpr auto zero() -> Duality::Core::detail::ColorComponent {
    return Duality::Core::detail::ColorComponent{};
  }
  
  static constexpr auto one() -> Duality::Core::detail::ColorComponent {
    return Duality::Core::detail::ColorComponent{63};
  }
};