/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <array>
#include <util/integer.hpp>
#include <util/math/numeric_traits.hpp>

template<typename T, uint n>
struct Vector {
  Vector() {}

  auto operator[](int i) -> T& {
    return data[i];
  }

  auto operator[](int i) const -> T {
    return data[i];
  }

  auto operator+(Vector const& other) -> Vector {
    Vector result{};
    for (uint i = 0; i < n; i++)
      result[i] = data[i] + other[i];
    return result;
  }

  auto operator-(Vector const& other) -> Vector {
    Vector result{};
    for (uint i = 0; i < n; i++)
      result[i] = data[i] - other[i];
    return result;
  }

  auto operator*(T value) const -> Vector {
    Vector result{};
    for (uint i = 0; i < n; i++)
      result[i] = data[i] * value;
    return result;
  }

  auto operator/(T value) const -> Vector {
    Vector result{};
    for (uint i = 0; i < n; i++)
      result[i] = data[i] / value;
    return result;
  }

  auto operator+=(Vector const& other) -> Vector& {
    for (uint i = 0; i < n; i++)
      data[i] += other[i];
    return *this;
  }

  auto operator-=(Vector const& other) -> Vector& {
    for (uint i = 0; i < n; i++)
      data[i] -= other[i];
    return *this;
  }

  auto operator*=(T value) -> Vector& {
    for (uint i = 0; i < n; i++)
      data[i] *= value;
    return *this;
  }

  auto operator/=(T value) -> Vector& {
    for (uint i = 0; i < n; i++)
      data[i] /= value;
    return *this;
  }

  auto operator-() const -> Vector {
    Vector result{};
    for (uint i = 0; i < n; i++)
      result[i] = -data[i];
    return result;
  }

  bool operator==(Vector const& other) const {
    for (uint i = 0; i < n; i++)
      if (data[i] != other[i])
        return false;
    return true;
  }

  bool operator!=(Vector const& other) const {
    return !(*this == other);
  }

  auto dot(Vector const& other) const -> T {
    T result{};
    for (uint i = 0; i < n; i++)
      result += data[i] * other[i];
    return result;
  }

  template<typename U>
  static auto interpolate(Vector const& a, Vector const& b, U factor) -> Vector {
    Vector result{};
    U one_minus_factor = NumericConstants<U>::one() - factor;
    for (uint i = 0; i < n; i++)
      result[i] = a[i] * one_minus_factor + b[i] * factor;
    return result;
  }

protected:
  T data[n] { NumericConstants<T>::zero() };
};

template<typename T>
struct Vector2 : Vector<T, 2> {
  Vector2() {}

  Vector2(T x, T y) {
    this->data[0] = x;
    this->data[1] = y;
  }

  Vector2(Vector<T, 2> const& other) {
    this->data[0] = other[0];
    this->data[1] = other[1];
  }

  auto x() -> T& { return this->data[0]; }
  auto y() -> T& { return this->data[1]; }

  auto x() const -> T { return this->data[0]; }
  auto y() const -> T { return this->data[1]; }
};

template<typename T>
struct Vector3 : Vector<T, 3> {
  Vector3() {}

  Vector3(T x, T y, T z) {
    this->data[0] = x;
    this->data[1] = y;
    this->data[2] = z;
  }

  Vector3(Vector<T, 3> const& other) {
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
struct Vector4 : Vector<T, 4> {
  Vector4() {}

  Vector4(T x, T y, T z, T w) {
    this->data[0] = x;
    this->data[1] = y;
    this->data[2] = z;
    this->data[3] = w;
  }

  Vector4(Vector<T, 3> const& other) {
    for (uint i = 0; i < 3; i++)
      this->data[i] = other[i];
    this->data[3] = NumericConstants<T>::one();
  }

  Vector4(Vector<T, 4> const& other) {
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
};
