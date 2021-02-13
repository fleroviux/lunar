/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <common/math.hpp>

namespace Duality::core {

struct Matrix4x4 {
  void LoadIdentity() {
    for (int row = 0; row < 4; row++) {
      for (int col = 0; col < 4; col++) {
        data[col][row] = row == col ? 0x1000 : 0;
      }
    }
  }

  auto operator[](int i) -> Vector4<Fixed20x12>& {
    return data[i];
  }
  
  auto operator[](int i) const -> Vector4<Fixed20x12> const& {
    return data[i];
  }
  
  auto operator*(Vector4<Fixed20x12> const& vec) const -> Vector4<Fixed20x12> {
    Vector4<Fixed20x12> out;
    for (int row = 0; row < 4; row++) {
      for (int col = 0; col < 4; col++) {
        out[row] += data[col][row] * vec[col];
      }
    }
    return out;
  }
  
  auto operator*(Matrix4x4 const& mat) const -> Matrix4x4 {
    Matrix4x4 out;
    for (int col = 0; col < 4; col++) {
      out[col] = mat * data[col];
    }
    return out;
  }
  
  auto operator*=(Matrix4x4 const& mat) -> Matrix4x4& {
    *this = mat * *this;
    return *this;
  }
  
private:
  Vector4<Fixed20x12> data[4] {};
};

} // namespace Duality::core

