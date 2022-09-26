/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

constexpr auto merge_3d_vert = R"(
  #version 330 core

  layout(location = 0) in vec2 a_position;
  layout(location = 1) in vec2 a_uv;

  out vec2 v_uv;

  void main() {
    v_uv = a_uv;

    gl_Position = vec4(a_position, 0.0, 1.0);
  }
)";

constexpr auto merge_3d_frag = R"(
  #version 330 core

  #define SFX_NONE 0
  #define SFX_BLEND 1
  #define SFX_BRIGHTEN 2
  #define SFX_DARKEN 3

  layout(location = 0) out vec4 frag_color;

  in vec2 v_uv;

  uniform sampler2D u_color2d_map;
  uniform sampler2D u_color3d_map;
  uniform usampler2D u_attribute_map;

  // MMIO:
  // @todo: upload this data per-scanline into a SSBO
  uniform bool u_enable_bg0_3d;
  uniform int u_blend_control;
  uniform float u_blend_eva;
  uniform float u_blend_evb;
  uniform float u_blend_evy;

  void main() {
    vec4 color_2d = texture2D(u_color2d_map, v_uv);
    vec4 color_3d = texture2D(u_color3d_map, v_uv);
    uint attributes = texture(u_attribute_map, v_uv).r;

    uint layer_top = attributes & 7U;
    uint layer_bottom = (attributes >> 3) & 7U;

    int blend_effect = (u_blend_control >> 6) & 3;

    bool have_1st_target = (u_blend_control & (  1 << layer_top   )) != 0;
    bool have_2nd_target = (u_blend_control & (256 << layer_bottom)) != 0;

    if(u_enable_bg0_3d) {
      if(layer_top == 0U) {
        frag_color = vec4(color_3d.rgb, 1.0);

        switch(blend_effect) {
          case SFX_BLEND: {
            if(have_2nd_target) {
              frag_color.rgb = mix(color_2d.rgb, color_3d.rgb, color_3d.a);
            }
            break;
          }
          case SFX_BRIGHTEN: {
            if(have_1st_target) {
              frag_color.rgb = mix(color_3d.rgb, vec3(1.0), u_blend_evy);
            }
            break;
          }
          case SFX_DARKEN: {
            if(have_1st_target) {
              frag_color.rgb = mix(color_3d.rgb, vec3(0.0), u_blend_evy);
            }
            break;
          }
        }
      } else if(
          layer_bottom == 0U &&
          blend_effect == SFX_BLEND &&
          have_1st_target &&
          have_2nd_target
      ) {
        frag_color = vec4(color_2d.rgb * u_blend_eva + color_3d.rgb * u_blend_evb, 1.0);
      } else {
        frag_color = color_2d;
      }
    } else {
      frag_color = color_2d;
    }
  }
)";