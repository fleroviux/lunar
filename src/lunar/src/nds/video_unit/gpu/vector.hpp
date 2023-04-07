/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <atom/math/vector.hpp>

namespace lunar::nds {

  template<typename T>
  class Vector2 final : public atom::detail::Vector2<Vector2<T>, T> {
    public:
      using atom::detail::Vector2<Vector2<T>, T>::Vector2;
  };

  template<typename T>
  class Vector3 final : public atom::detail::Vector3<Vector3<T>, T> {
    public:
      using atom::detail::Vector3<Vector3<T>, T>::Vector3;
  };

  template<typename T>
  class Vector4 final : public atom::detail::Vector4<Vector4<T>, Vector3<T>, T> {
    public:
      using atom::detail::Vector4<Vector4<T>, Vector3<T>, T>::Vector4;
  };

} // namespace lunar::nds
