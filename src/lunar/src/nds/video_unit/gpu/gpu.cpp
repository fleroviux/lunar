/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "renderer/opengl/opengl_renderer.hpp"
#include "renderer/software/software_renderer.hpp"
#include "gpu.hpp"

namespace lunar::nds {

GPU::GPU(Scheduler& scheduler, IRQ& irq9, DMA9& dma9, VRAM const& vram)
    : scheduler(scheduler)
    , irq9(irq9)
    , dma9(dma9)
    , vram_texture(vram.region_gpu_texture)
    , vram_palette(vram.region_gpu_palette) {
  Reset();
}

void GPU::Reset() {
  disp3dcnt = {};
  // TODO:
  //gxstat = {};
  gxfifo.Reset();
  gxpipe.Reset();
  packed_cmds = 0;
  packed_args_left = 0;
  alpha_test_ref = {};
  clear_color = {};
  clear_depth = {};
  clrimage_offset = {};
  fog_color = {};
  fog_offset = {};
  
  for (int  i = 0; i < 2; i++) {
    vertices[i] = {};
    polygons[i] = {};
  }
  buffer = 0;
  
  in_vertex_list = false;
  position_old = {};
  current_vertex_list.clear();

  for (auto& light : lights)  light = {};

  material = {};
  toon_table.fill(0x7FFF);
  edge_color_table.fill(0x7FFF);
  fog_density_table.fill(0);
  toon_table_dirty = true;
  fog_density_table_dirty = true;

  matrix_mode = MatrixMode::Projection;
  projection.Reset();
  modelview.Reset();
  direction.Reset();
  texture.Reset();
  clip_matrix = Matrix4<Fixed20x12>::Identity();

  manual_translucent_y_sorting = false;
  manual_translucent_y_sorting_pending = false;
  use_w_buffer_pending = false;
  swap_buffers_pending = false;

  renderer = std::make_unique<OpenGLRenderer>(
    vram_texture, vram_palette, disp3dcnt, alpha_test_ref, clear_color, clear_depth, fog_color, fog_offset, edge_color_table);
//  renderer = std::make_unique<SoftwareRenderer>(
//    vram_texture, vram_palette, disp3dcnt, alpha_test_ref, toon_table, edge_color_table, clear_color, clear_depth);
}

void GPU::Render() {
  if (toon_table_dirty) {
    renderer->UpdateToonTable(toon_table);
    toon_table_dirty = false;
  }

  if (fog_density_table_dirty) {
    renderer->UpdateFogDensityTable(fog_density_table);
    fog_density_table_dirty = false;
  }

  renderer->Render((void const**)polygons_sorted.begin(), (int)polygons_sorted.size());
}

void GPU::WriteToonTable(uint offset, u8 value) {
  auto index = offset >> 1;
  auto shift = (offset & 1) * 8;

  toon_table[index] = (toon_table[index] & ~(0xFF << shift)) | (value << shift);
  toon_table_dirty = true;
}

void GPU::WriteEdgeColorTable(uint offset, u8 value) {
  auto index = offset >> 1;
  auto shift = (offset & 1) * 8;

  edge_color_table[index] = (edge_color_table[index] & ~(0xFF << shift)) | (value << shift);
}

void GPU::WriteFogDensityTable(uint offset, u8 value) {
  fog_density_table[offset] = value & 0x7F;
  fog_density_table_dirty = true;
}

void GPU::SwapBuffers() {
  if (swap_buffers_pending) {
    polygons_sorted.clear();

    for (int i = 0; i < polygons[buffer].count; i++) {
      polygons_sorted.push_back(&polygons[buffer].data[i]);
    }

    std::stable_sort(polygons_sorted.begin(), polygons_sorted.end(), [](auto a, auto b) {
      return a->sorting_key < b->sorting_key;
    });

    buffer ^= 1;
    vertices[buffer].count = 0;
    polygons[buffer].count = 0;
    manual_translucent_y_sorting = manual_translucent_y_sorting_pending;
    renderer->SetWBufferEnable(use_w_buffer_pending);
    swap_buffers_pending = false;
    ProcessCommands();
  }
}

} // namespace lunar::nds
