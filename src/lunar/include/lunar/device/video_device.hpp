/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <atom/integer.hpp>

namespace lunar {

class VideoDevice {
  public:
    enum ImageType {
      Software,
      OpenGL
    };

    virtual ~VideoDevice() = default;

    virtual void Draw(
      ImageType top_image_type,
      void const* top_image,
      ImageType bottom_image_type,
      void const* bottom_image
    ) = 0;
};

} // namespace lunar::nds
