/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

// Include gpu.hpp for definitions... (need to find a better solution for this)
#include "nds/video_unit/gpu/gpu.hpp"

#include <atom/punning.hpp>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "nds/video_unit/gpu/renderer/renderer_base.hpp"
#include "nds/video_unit/gpu/color.hpp"
#include "nds/video_unit/vram_region.hpp"

namespace lunar::nds {

class SoftwareRenderer final : public RendererBase {
  public:
    SoftwareRenderer(
      Region<4, 131072> const& vram_texture,
      Region<8> const& vram_palette,
      GPU::DISP3DCNT const& disp3dcnt,
      GPU::AlphaTest const& alpha_test,
      std::array<u16, 32> const& toon_table,
      std::array<u16, 8> const& edge_color_table,
      GPU::ClearColor const& clear_color,
      GPU::ClearDepth const& clear_depth
    );

   ~SoftwareRenderer();

    // @todo: the GetOutput()/GetOutputImageType() methods are pretty useless here:
    // replace with something simpler? i.e. GetHardwareSurface()?

    auto GetOutput() -> void const* override {
      return nullptr;
    }

    auto GetOutputImageType() const -> VideoDevice::ImageType override {
      return VideoDevice::ImageType::Software;
    }

    void Render(void const** polygons, int polygon_count) override;

    void SetWBufferEnable(bool enable) override {
      use_w_buffer = enable;
    }

    void CaptureColor(u16* buffer, int vcount, int width, bool display_capture);
    void CaptureAlpha(int* buffer, int vcount) override;

    void Sync() override {
      WaitForRenderWorkers();
    }

  private:
    enum AttributeFlags {
      ATTRIBUTE_FLAG_SHADOW = 1,
      ATTRIBUTE_FLAG_EDGE = 2
    };

    struct Attribute {
      u16 flags;
      u8  poly_id[2];
    };

    struct Span {
      s32 x0[2];
      s32 x1[2];
      s32 w[2];
      u32 depth[2];
      Vector2<Fixed12x4> uv[2];
      Color4 color[2];
    };

    template<typename T>
    auto ReadTextureVRAM(u32 address) {
      return atom::read<T>(vram_texture_copy, address & 0x7FFFF & ~(sizeof(T) - 1));
    }

    template<typename T>
    auto ReadPaletteVRAM(u32 address) {
      return atom::read<T>(vram_palette_copy, address & 0x1FFFF & ~(sizeof(T) - 1));
    }

    void RenderRearPlane(int thread_min_y, int thread_max_y);
    void RenderPolygons(int thread_min_y, int thread_max_y);
    void RenderEdgeMarking();

    void SetupRenderWorkers();
    void WaitForRenderWorkers();
    void JoinRenderWorkers();

    auto SampleTexture(
      GPU::TextureParams const& params,
      Vector2<Fixed12x4> const& uv
    ) -> Color4;

    // GPU texture and texture palette data
    Region<4, 131072> const& vram_texture { 3 };
    Region<8> const& vram_palette { 7 };
    u8 vram_texture_copy[524288];
    u8 vram_palette_copy[131072];

    // GPU MMIO
    GPU::DISP3DCNT const& disp3dcnt;
    GPU::AlphaTest const& alpha_test;
    std::array<u16, 32> const& toon_table;
    std::array<u16, 8> const& edge_color_table;
    GPU::ClearColor const& clear_color;
    GPU::ClearDepth const& clear_depth;

    bool use_w_buffer = false;

    // Color, depth and attribute buffers
    Color4 color_buffer[256 * 192];
    u32 depth_buffer[256 * 192];
    Attribute attribute_buffer[256 * 192];

    const GPU::Polygon** polygons = nullptr;
    int polygon_count = 0;

    static constexpr int kRenderThreadCount = 4;

    struct RenderWorker {
      int min_y;
      int max_y;
      std::thread thread;
      std::atomic_bool running;
      std::atomic_bool rendering;
      std::mutex rendering_mutex;
      std::condition_variable rendering_cv;
    } render_workers[kRenderThreadCount];
};

} // namespace lunar::nds
