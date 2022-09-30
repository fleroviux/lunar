/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "software_renderer.hpp"

namespace lunar::nds {

SoftwareRenderer::SoftwareRenderer(
  Region<4, 131072> const& vram_texture,
  Region<8> const& vram_palette,
  GPU::DISP3DCNT const& disp3dcnt,
  GPU::AlphaTest const& alpha_test,
  std::array<u16, 32> const& toon_table,
  std::array<u16, 8> const& edge_color_table,
  GPU::ClearColor const& clear_color,
  GPU::ClearDepth const& clear_depth
)   : vram_texture(vram_texture)
    , vram_palette(vram_palette)
    , disp3dcnt(disp3dcnt)
    , alpha_test(alpha_test)
    , toon_table(toon_table)
    , edge_color_table(edge_color_table)
    , clear_color(clear_color)
    , clear_depth(clear_depth) {
  std::memset(vram_texture_copy, 0, sizeof(vram_texture_copy));
  std::memset(vram_palette_copy, 0, sizeof(vram_palette_copy));

  for(int i = 0; i < 256 * 192; i++) {
    color_buffer[i] = {};
    depth_buffer[i] = 0;
    attribute_buffer[i] = {};
  }

  SetupRenderWorkers();
}

SoftwareRenderer::~SoftwareRenderer() {
  JoinRenderWorkers();
}

void SoftwareRenderer::Render(const void** polygons_, int polygon_count) {
  this->polygons = (const GPU::Polygon**)polygons_;
  this->polygon_count = polygon_count;

  for (u32 address = 0; address < 0x80000; address += 8) {
    *(u64*)&vram_texture_copy[address] = vram_texture.Read<u64>(address);
  }

  for (u32 address = 0; address < 0x20000; address += 8) {
    *(u64*)&vram_palette_copy[address] = vram_palette.Read<u64>(address);
  }

  for (auto& render_worker : render_workers) {
    std::lock_guard lock{render_worker.rendering_mutex};
    render_worker.rendering = true;
    render_worker.rendering_cv.notify_one();
  }
}

void SoftwareRenderer::SetupRenderWorkers() {
  int min_y = 0;
  int lines_per_thread = 192 / kRenderThreadCount;

  JoinRenderWorkers();

  for (auto& render_worker : render_workers) {
    render_worker.min_y = min_y;
    render_worker.max_y = min_y + lines_per_thread - 1;
    render_worker.running = true;
    render_worker.rendering = false;
    min_y += lines_per_thread;
  }

  // In case 192 is not evenly divisible by the number of threads,
  // makes sure that all scanlines are covered.
  render_workers[kRenderThreadCount - 1].max_y = 191;

  for (auto& render_worker : render_workers) {
    render_worker.thread = std::thread{[this, &render_worker]() {
      while (render_worker.running) {
        if (render_worker.rendering) {
          const int thread_min_y = render_worker.min_y;
          const int thread_max_y = render_worker.max_y;

          RenderRearPlane(thread_min_y, thread_max_y);
          RenderPolygons(thread_min_y, thread_max_y);

          std::unique_lock lock{render_worker.rendering_mutex};
          render_worker.rendering = false;
          render_worker.rendering_cv.wait(lock, [&](){return (bool)render_worker.rendering;});
        }
      }
    }};
  }
}

//void GPU::WaitForRenderWorkers() {
//  for (auto& render_worker : render_workers) {
//    // TODO: this is inefficient - solve properly
//    while (render_worker.rendering) {}
//  }
//
//  if (disp3dcnt.enable_edge_marking) {
//    RenderEdgeMarking();
//  }
//}

void SoftwareRenderer::JoinRenderWorkers() {
  for (auto& render_worker : render_workers) {
    if (render_worker.running) {
      // Wake the render worker up in case it is currently sleeping:
      render_worker.rendering_mutex.lock();
      render_worker.rendering = true;
      render_worker.rendering_cv.notify_one();
      render_worker.rendering_mutex.unlock();

      // Stop the thread and join it:
      render_worker.running = false;
      render_worker.thread.join();
    }
  }
}

} // namespace lunar::nds
