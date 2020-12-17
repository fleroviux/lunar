/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <common/integer.hpp>

namespace fauxDS::core {

struct Vector4 {
  auto operator[](int i) -> s32& {
    return data[i];
  }
  
   auto operator[](int i) const -> s32 {
    return data[i];
  }
  
  auto operator+=(Vector4 const& vec) -> Vector4& {
    data[0] += vec.data[0];
    data[1] += vec.data[1];
    data[2] += vec.data[2];
    data[3] += vec.data[3];
    return *this;
  }

private:
  s32 data[4] {};
};

struct Matrix4x4 {
  void LoadIdentity() {
    for (int row = 0; row < 4; row++) {
      for (int col = 0; col < 4; col++) {
        data[col][row] = row == col ? 0x1000 : 0;
      }
    }
  }

  auto operator[](int i) -> Vector4& {
    return data[i];
  }
  
  auto operator[](int i) const -> Vector4 const& {
    return data[i];
  }
  
  auto operator*(Vector4 const& vec) const -> Vector4 {
    Vector4 out;
    for (int row = 0; row < 4; row++) {
      s64 tmp = 0;
      for (int col = 0; col < 4; col++) {
        tmp += s64(data[col][row]) * s64(vec[col]);
      }
      out[row] = tmp >> 12;
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
  Vector4 data[4] {};
};

} // namespace fauxDS::core

