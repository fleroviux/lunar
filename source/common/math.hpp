/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <array>
#include <common/integer.hpp>

template<typename T>
struct NumericConstants {
};

template<>
struct NumericConstants<float> {
  static constexpr auto zero() -> float {
    return 0;
  }
  
  static constexpr auto one() -> float {
    return 1;
  }
};

template<>
struct NumericConstants<double> {
  static constexpr auto zero() -> double {
    return 0;
  }
  
  static constexpr auto one() -> double {
    return 1;
  }
};

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

  auto dot(Vector const& other) const -> T {
    T result{};
    for (uint i = 0; i < n; i++)
      result += data[i] * other[i];
    return result;
  }

  static auto interpolate(Vector const& a, Vector const& b, T factor) -> Vector {
    Vector result{};
    T one_minus_factor = NumericConstants<T>::one() - factor;
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

template<typename T>
struct Matrix4 {
  Matrix4() {};

  Matrix4(std::array<T, 16> const& elements) {
    for (uint i = 0; i < 16; i++)
      data[i & 3][i >> 2] = elements[i];
  }

  auto operator[](int i) -> Vector4<T>& {
    return data[i];
  }
  
  auto operator[](int i) const -> Vector4<T> const& {
    return data[i];
  }

  auto operator*(Vector<T, 4> const& vec) const -> Vector<T, 4> {
    Vector<T, 4> result{};
    for (uint i = 0; i < 4; i++)
      result += data[i] * vec[i];
    return result;
  }

  auto operator*(Matrix4 const& other) const -> Matrix4 {
    Matrix4 result{};
    for (uint i = 0; i < 4; i++)
      result[i] = *this * other[i];
    return result;
  }

private:
  Vector4<T> data[4] {};
};

struct Fixed20x12 {
  constexpr Fixed20x12() {}
  constexpr Fixed20x12(s32 value) : value(value) {}

  auto integer() -> s32 { return value >> 12; }

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

private:
  s32 value = 0;
};

template<>
struct NumericConstants<Fixed20x12> {
  static constexpr auto zero() -> Fixed20x12 {
    return Fixed20x12{};
  }
  
  static constexpr auto one() -> Fixed20x12 {
    return Fixed20x12{1 << 12};
  }
};

