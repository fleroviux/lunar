
// Copyright (C) 2022 fleroviux

#pragma once

#include <array>
#include <lunar/integer.hpp>

#include "numeric_traits.hpp"

namespace lunar {

template<typename Derived, typename T, uint n>
struct Vector {
  Vector() {}

  auto operator[](int i) -> T& {
    return data[i];
  }

  auto operator[](int i) const -> T {
    return data[i];
  }

  auto operator+(Derived const& other) const -> Derived {
    Derived result{};
    for (uint i = 0; i < n; i++)
      result[i] = data[i] + other[i];
    return result;
  }

  auto operator-(Derived const& other) const -> Derived {
    Derived result{};
    for (uint i = 0; i < n; i++)
      result[i] = data[i] - other[i];
    return result;
  }

  auto operator*(T value) const -> Derived {
    Derived result{};
    for (uint i = 0; i < n; i++)
      result[i] = data[i] * value;
    return result;
  }

  auto operator/(T value) const -> Derived {
    Derived result{};
    for (uint i = 0; i < n; i++)
      result[i] = data[i] / value;
    return result;
  }

  auto operator+=(Derived const& other) -> Derived& {
    for (uint i = 0; i < n; i++)
      data[i] += other[i];
    return *static_cast<Derived*>(this);
  }

  auto operator-=(Derived const& other) -> Derived& {
    for (uint i = 0; i < n; i++)
      data[i] -= other[i];
    return *static_cast<Derived*>(this);
  }

  auto operator*=(T value) -> Derived& {
    for (uint i = 0; i < n; i++)
      data[i] *= value;
    return *static_cast<Derived*>(this);
  }

  auto operator/=(T value) -> Derived& {
    for (uint i = 0; i < n; i++)
      data[i] /= value;
    return *static_cast<Derived*>(this);
  }

  auto operator-() const -> Derived {
    Derived result{};
    for (uint i = 0; i < n; i++)
      result[i] = -data[i];
    return result;
  }

  bool operator==(Derived const& other) const {
    for (uint i = 0; i < n; i++)
      if (data[i] != other[i])
        return false;
    return true;
  }

  bool operator!=(Derived const& other) const {
    return !(*this == other);
  }

  auto dot(Derived const& other) const -> T {
    T result{};
    for (uint i = 0; i < n; i++)
      result += data[i] * other[i];
    return result;
  }

protected:
  T data[n] { NumericConstants<T>::zero() };
};

template<typename T>
struct Vector2 : Vector<Vector2<T>, T, 2> {
  Vector2() {}

  Vector2(T x, T y) {
    this->data[0] = x;
    this->data[1] = y;
  }

  Vector2(Vector2 const& other) {
    this->data[0] = other[0];
    this->data[1] = other[1];
  }

  auto x() -> T& { return this->data[0]; }
  auto y() -> T& { return this->data[1]; }

  auto x() const -> T { return this->data[0]; }
  auto y() const -> T { return this->data[1]; }
};

template<typename T>
struct Vector3 : Vector<Vector3<T>, T, 3> {
  Vector3() {}

  Vector3(T x, T y, T z) {
    this->data[0] = x;
    this->data[1] = y;
    this->data[2] = z;
  }

  Vector3(Vector3 const& other) {
    for (uint i = 0; i < 3; i++)
      this->data[i] = other[i];
  }

  auto x() -> T& { return this->data[0]; }
  auto y() -> T& { return this->data[1]; }
  auto z() -> T& { return this->data[2]; }

  auto x() const -> T { return this->data[0]; }
  auto y() const -> T { return this->data[1]; }
  auto z() const -> T { return this->data[2]; }

  auto cross(Vector3 const& other) -> Vector3 {
    return {
      this->data[1] * other[2] - this->data[2] * other[1],
      this->data[2] * other[0] - this->data[0] * other[2],
      this->data[0] * other[1] - this->data[1] * other[0]
    };
  }
};

template<typename T>
struct Vector4 : Vector<Vector4<T>, T, 4> {
  Vector4() {}

  Vector4(T x, T y, T z, T w) {
    this->data[0] = x;
    this->data[1] = y;
    this->data[2] = z;
    this->data[3] = w;
  }

  Vector4(Vector3<T> const& other) {
    for (uint i = 0; i < 3; i++)
      this->data[i] = other[i];
    this->data[3] = NumericConstants<T>::one();
  }

  Vector4(Vector4 const& other) {
    for (uint i = 0; i < 4; i++)
      this->data[i] = other[i];
  }

  auto x() -> T& { return this->data[0]; }
  auto y() -> T& { return this->data[1]; }
  auto z() -> T& { return this->data[2]; }
  auto w() -> T& { return this->data[3]; }

  auto x() const -> T { return this->data[0]; }
  auto y() const -> T { return this->data[1]; }
  auto z() const -> T { return this->data[2]; }
  auto w() const -> T { return this->data[3]; }

  auto xyz() const -> Vector3<T> { return Vector3<T>{x(), y(), z()}; }
};

} // namespace lunar
