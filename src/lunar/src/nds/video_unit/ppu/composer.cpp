/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "shader/merge_3d.glsl.hpp"
#include "ppu.hpp"

// @todo: remove this atrocious hack:
GLuint opengl_final_texture;
extern GLuint opengl_color_texture;

namespace lunar::nds {

template<bool window, bool blending>
void PPU::ComposeScanlineTmpl(u16 vcount, int bg_min, int bg_max) {
  auto& mmio = mmio_copy[vcount];

  u16 backdrop = ReadPalette(0, 0);

  auto const& dispcnt = mmio.dispcnt;
  auto const& bgcnt = mmio.bgcnt;
  auto const& winin = mmio.winin;
  auto const& winout = mmio.winout;

  int bg_list[4];
  int bg_count = 0;

  // Sort enabled backgrounds by their respective priority in ascending order.
  for (int prio = 3; prio >= 0; prio--) {
    for (int bg = bg_max; bg >= bg_min; bg--) {
      if (dispcnt.enable[bg] && bgcnt[bg].priority == prio) {
        bg_list[bg_count++] = bg;
      }
    }
  }

  bool win0_active = false;
  bool win1_active = false;
  bool win2_active = false;

  const bool* win_layer_enable;
  
  if constexpr (window) {
    win0_active = dispcnt.enable[ENABLE_WIN0] && window_scanline_enable[0];
    win1_active = dispcnt.enable[ENABLE_WIN1] && window_scanline_enable[1];
    win2_active = dispcnt.enable[ENABLE_OBJWIN];
  }

  int prio[2];
  int layer[2];
  u16 pixel[2];

  for (int x = 0; x < 256; x++) {
    if constexpr (window) {
      // Determine the window with the highest priority for this pixel.
      if (win0_active && buffer_win[0][x]) {
        win_layer_enable = winin.enable[0];
      } else if (win1_active && buffer_win[1][x]) {
        win_layer_enable = winin.enable[1];
      } else if (win2_active && buffer_obj[x].window) {
        win_layer_enable = winout.enable[1];
      } else {
        win_layer_enable = winout.enable[0];
      }
    }

    if constexpr (blending) {
      bool is_alpha_obj = false;

      prio[0] = 4;
      prio[1] = 4;
      layer[0] = LAYER_BD;
      layer[1] = LAYER_BD;
      
      // Find up to two top-most visible background pixels.
      for (int i = 0; i < bg_count; i++) {
        int bg = bg_list[i];

        if (!window || win_layer_enable[bg]) {
          auto pixel_new = buffer_bg[bg][x];
          if (pixel_new != s_color_transparent) {
            layer[1] = layer[0];
            layer[0] = bg;
            prio[1] = prio[0];
            prio[0] = bgcnt[bg].priority;
          }
        }
      }

      /* Check if a OBJ pixel takes priority over one of the two
       * top-most background pixels and insert it accordingly.
       */
      if ((!window || win_layer_enable[LAYER_OBJ]) &&
          dispcnt.enable[ENABLE_OBJ] &&
          buffer_obj[x].color != s_color_transparent) {
        int priority = buffer_obj[x].priority;

        if (priority <= prio[0]) {
          layer[1] = layer[0];
          layer[0] = LAYER_OBJ;
          is_alpha_obj = buffer_obj[x].alpha;
        } else if (priority <= prio[1]) {
          layer[1] = LAYER_OBJ;
        }
      }

      // Map layer numbers to pixels.
      for (int i = 0; i < 2; i++) {
        int _layer = layer[i];
        switch (_layer) {
          case 0:
          case 1:
          case 2:
          case 3:
            pixel[i] = buffer_bg[_layer][x];
            break;
          case 4:
            pixel[i] = buffer_obj[x].color;
            break;
          case 5:
            pixel[i] = backdrop;
            break;
        }
      }

      auto blend_mode = mmio.bldcnt.sfx;
      bool have_dst = mmio.bldcnt.targets[0][layer[0]];
      bool have_src = mmio.bldcnt.targets[1][layer[1]];
      auto sfx_enable = !window || win_layer_enable[LAYER_SFX];

      if (is_alpha_obj && have_src) {
        Blend(vcount, pixel[0], pixel[1], BlendControl::Effect::SFX_BLEND);
      } if (blend_mode == BlendControl::Effect::SFX_BLEND) {
        // TODO: what does HW do if "enable BG0 3D" is disabled in mode 6.
        if (layer[0] == 0 && (mmio.dispcnt.enable_bg0_3d || mmio.dispcnt.bg_mode == 6) && have_src) {
          auto real_bldalpha = mmio.bldalpha;

          // Someone should revoke my coding license for this.
          mmio.bldalpha.a = gpu_output[vcount * 256 + x].a().raw() >> 2;
          mmio.bldalpha.b = 16 - mmio.bldalpha.a;

          Blend(vcount, pixel[0], pixel[1], BlendControl::Effect::SFX_BLEND);

          mmio.bldalpha = real_bldalpha;
        } else if (have_dst && have_src) {
          Blend(vcount, pixel[0], pixel[1], BlendControl::Effect::SFX_BLEND);
        }
      } else if (blend_mode != BlendControl::Effect::SFX_NONE) {
        if (have_dst && sfx_enable) {
          Blend(vcount, pixel[0], pixel[1], blend_mode);
        }
      }
    } else {
      pixel[0] = backdrop;
      prio[0] = 4;
      
      // Find the top-most visible background pixel.
      if (bg_count != 0) {
        for (int i = bg_count - 1; i >= 0; i--) {
          int bg = bg_list[i];

          if (!window || win_layer_enable[bg]) {
            u16 pixel_new = buffer_bg[bg][x];
            if (pixel_new != s_color_transparent) {
              pixel[0] = pixel_new;
              prio[0] = bgcnt[bg].priority;
              break;
            }   
          }
        }
      }

      // Check if a OBJ pixel takes priority over the top-most background pixel.
      if ((!window || win_layer_enable[LAYER_OBJ]) &&
          dispcnt.enable[ENABLE_OBJ] &&
          buffer_obj[x].color != s_color_transparent &&
          buffer_obj[x].priority <= prio[0]) {
        pixel[0] = buffer_obj[x].color;
      }
    }

    buffer_compose[x] = pixel[0] | 0x8000;
  }
}

void PPU::ComposeScanline(u16 vcount, int bg_min, int bg_max) {
  auto const& mmio = mmio_copy[vcount];
  auto const& dispcnt = mmio.dispcnt;

  int key = 0;

  if (dispcnt.enable[ENABLE_WIN0] ||
      dispcnt.enable[ENABLE_WIN1] ||
      dispcnt.enable[ENABLE_OBJWIN]) {
    key |= 1;
  }

  if (mmio.bldcnt.sfx != BlendControl::Effect::SFX_NONE || line_contains_alpha_obj) {
    key |= 2;
  }

  switch (key) {
    case 0b00:
      ComposeScanlineTmpl<false, false>(vcount, bg_min, bg_max);
      break;
    case 0b01:
      ComposeScanlineTmpl<true, false>(vcount, bg_min, bg_max);
      break;
    case 0b10:
      ComposeScanlineTmpl<false, true>(vcount, bg_min, bg_max);
      break;
    case 0b11:
      ComposeScanlineTmpl<true, true>(vcount, bg_min, bg_max);
      break;
  }
}

void PPU::Blend(u16  vcount,
                u16& target1,
                u16  target2,
                BlendControl::Effect sfx) {
  auto const& mmio = mmio_copy[vcount];

  int r1 = (target1 >>  0) & 0x1F;
  int g1 = (target1 >>  5) & 0x1F;
  int b1 = (target1 >> 10) & 0x1F;

  switch (sfx) {
    case BlendControl::Effect::SFX_BLEND: {
      int eva = std::min<int>(16, mmio.bldalpha.a);
      int evb = std::min<int>(16, mmio.bldalpha.b);

      int r2 = (target2 >>  0) & 0x1F;
      int g2 = (target2 >>  5) & 0x1F;
      int b2 = (target2 >> 10) & 0x1F;

      r1 = std::min<u8>((r1 * eva + r2 * evb) >> 4, 31);
      g1 = std::min<u8>((g1 * eva + g2 * evb) >> 4, 31);
      b1 = std::min<u8>((b1 * eva + b2 * evb) >> 4, 31);
      break;
    }
    case BlendControl::Effect::SFX_BRIGHTEN: {
      int evy = std::min<int>(16, mmio.bldy.y);
      
      r1 += ((31 - r1) * evy) >> 4;
      g1 += ((31 - g1) * evy) >> 4;
      b1 += ((31 - b1) * evy) >> 4;
      break;
    }
    case BlendControl::Effect::SFX_DARKEN: {
      int evy = std::min<int>(16, mmio.bldy.y);
      
      r1 -= (r1 * evy) >> 4;
      g1 -= (g1 * evy) >> 4;
      b1 -= (b1 * evy) >> 4;
      break;
    }
  }

  target1 = r1 | (g1 << 5) | (b1 << 10);
}

void PPU::Merge2DWithOpenGL3D() {
  // @todo: get the real dimensions from the GPU or config:
  const int output_width  = 512;
  const int output_height = 384;

  if (!gpu_output) {
    return;
  }

  if (!ogl.initialized) {
    ogl.fbo = FrameBufferObject::Create();
    ogl.output_texture = Texture2D::Create(output_width, output_height, GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE);
    ogl.fbo->Attach(GL_COLOR_ATTACHMENT0, ogl.output_texture);

    ogl.program = ProgramObject::Create(merge_3d_vert, merge_3d_frag);
    ogl.program->SetUniformInt("u_color2d_map", 0);
    ogl.program->SetUniformInt("u_color3d_map", 1);

    // @todo: abstract fullscreen quad creation
    {
      static constexpr float k_quad_vertices[] {
        // POSITION | UV
        -1.0, -1.0,    0.0, 0.0,
         1.0, -1.0,    1.0, 0.0,
         1.0,  1.0,    1.0, 1.0,
        -1.0,  1.0,    0.0, 1.0
      };

      ogl.vao = VertexArrayObject::Create();
      ogl.vbo = BufferObject::CreateArrayBuffer(sizeof(k_quad_vertices), GL_STATIC_DRAW);
      ogl.vao->SetAttribute(0, VertexArrayObject::Attribute{ogl.vbo, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0});
      ogl.vao->SetAttribute(1, VertexArrayObject::Attribute{ogl.vbo, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 2 * sizeof(float)});
      ogl.vbo->Upload(k_quad_vertices, sizeof(k_quad_vertices) / sizeof(float));
    }

    ogl.input_color_texture = Texture2D::Create(256, 192, GL_RGBA, GL_BGRA, GL_UNSIGNED_BYTE);

    opengl_final_texture = ogl.output_texture->Handle();
    ogl.initialized = true;
  }

  ogl.input_color_texture->Upload(output[frame]);

  glViewport(0, 0, output_width, output_height);
  ogl.fbo->Bind();
  ogl.program->Use();
  ogl.input_color_texture->Bind(GL_TEXTURE0);
  // @todo: fix this mess
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, opengl_color_texture);
  ogl.vao->Bind();
  glDrawArrays(GL_QUADS, 0, 4);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glUseProgram(0);
};

} // namespace lunar::nds
