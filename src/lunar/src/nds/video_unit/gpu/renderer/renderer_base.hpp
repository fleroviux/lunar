/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <array>
#include <lunar/device/video_device.hpp>
#include <atom/integer.hpp>

namespace lunar::nds {

// @todo: come up with a way to nicely share definitions between the GPU and renderer.

class RendererBase {
  public:
    virtual ~RendererBase() = default;

    virtual auto GetOutput() -> void const* = 0;
    virtual auto GetOutputImageType() const -> VideoDevice::ImageType = 0;

    virtual void Render(void const** polygons, int polygon_count) = 0;
    virtual void UpdateToonTable(std::array<u16, 32> const& toon_table) {}
    virtual void UpdateFogDensityTable(std::array<u8, 32> const& fog_density_table) {}
    virtual void SetWBufferEnable(bool enable) = 0;

    virtual void CaptureColor(u16* buffer, int vcount, int width, bool display_capture) = 0;
    virtual void CaptureAlpha(int* buffer, int vcount) = 0;

    virtual void Sync() {}
};

} // namespace lunar::nds