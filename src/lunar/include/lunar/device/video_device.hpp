/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <lunar/integer.hpp>

namespace lunar {

struct VideoDevice {
  enum ImageType {
    Software,
    OpenGL
  };

  virtual ~VideoDevice() = default;

  virtual void SetImageTypeTop(ImageType type) = 0;
  virtual void SetImageTypeBottom(ImageType type) = 0;

  virtual void Draw(void const* top, void const* bottom) = 0;
};

} // namespace lunar::nds
