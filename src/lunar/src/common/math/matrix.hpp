
// Copyright (C) 2022 fleroviux

#pragma once

#include "common/math/vector.hpp"

namespace lunar {

template<typename T>
struct Matrix4 {
  Matrix4() {};

  Matrix4(std::array<T, 16> const& elements) {
    for (uint i = 0; i < 16; i++)
      data[i & 3][i >> 2] = elements[i];
  }

  void identity() {
    for (uint row = 0; row < 4; row++) {
      for (uint col = 0; col < 4; col++) {
        if (row == col) {
          data[col][row] = NumericConstants<T>::one();
        } else {
          data[col][row] = NumericConstants<T>::zero();
        }
      }
    }
  }

  auto operator[](int i) -> Vector4<T>& {
    return data[i];
  }
  
  auto operator[](int i) const -> Vector4<T> const& {
    return data[i];
  }

  auto operator*(Vector4<T> const& vec) const -> Vector4<T> {
    Vector4<T> result{};
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

} // namespace lunar
