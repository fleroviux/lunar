/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <atom/math/matrix4.hpp>

#include "vector.hpp"

namespace lunar {

  template<typename T>
  class Matrix4 final : public atom::detail::Matrix4<Matrix4<T>, Vector4<T>, T> {
    public:
      using atom::detail::Matrix4<Matrix4<T>, Vector4<T>, T>::Matrix4;
  };

} // namespace lunar
