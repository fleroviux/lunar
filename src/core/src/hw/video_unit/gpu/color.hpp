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
  constexpr ColorComponent(u8 value) : value(value) {}

  auto raw() const -> u8 { return value; }

  auto operator+(ColorComponent other) const -> ColorComponent {
    return saturate_up(value + other.value);
  }

  auto operator-(ColorComponent other) const -> ColorComponent {
    return saturate_down(value - other.value);
  }

  auto operator*(ColorComponent other) const -> ColorComponent {
    return u8((u16(value) * u16(other.value)) >> 6);
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
    value = u8((u16(value) * u16(other.value)) >> 6);
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
  static auto saturate_up(u8 value) -> u8 {
    return std::min(value, u8(63));
  }

  static auto saturate_down(u8 value) -> u8 {
    return std::max(value, u8(0));
  }

  u8 value {};
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