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

  #define LAYER_OBJ 4U
  #define LAYER_BD 5U

  #define MASTER_BRIGHT_UP 1U
  #define MASTER_BRIGHT_DOWN 2U

  layout(location = 0) out vec4 frag_color;

  in vec2 v_uv;

  uniform sampler2D u_color2d_1st_map;
  uniform sampler2D u_color2d_2nd_map;
  uniform sampler2D u_color3d_map;
  uniform usampler2D u_attribute_map;

  // MMIO:
  // @todo: upload this data per-scanline into a SSBO
  uniform bool u_enable_bg0_3d;
  uniform uint u_bg0_priority;
  uniform int u_blend_control;
  uniform float u_blend_eva;
  uniform float u_blend_evb;
  uniform float u_blend_evy;
  uniform uint u_master_bright_mode;
  uniform float u_master_bright_factor;

  void main() {
    vec4 color_1st = texture(u_color2d_1st_map, v_uv);
    vec4 color_2nd = texture(u_color2d_2nd_map, v_uv);
    uint attributes = texture(u_attribute_map, v_uv).r;

    uint layer_1st = (attributes >> 0) & 7U;
    uint layer_2nd = (attributes >> 3) & 7U;
    bool is_alpha_obj = (attributes & 0x4000U) != 0U;
    bool enable_sfx = (attributes & 0x8000U) != 0U;

    if(u_enable_bg0_3d) {
      vec4 color_3d = texture(u_color3d_map, v_uv);

      if (color_3d.a != 0.0) { // @todo: confirm this logic
        uint priority_1st = (attributes >> 6) & 3U;
        uint priority_2nd = (attributes >> 8) & 3U;

        if(layer_1st == LAYER_BD || u_bg0_priority < priority_1st || (u_bg0_priority == priority_1st && layer_1st != LAYER_OBJ)) {
          layer_2nd = layer_1st;
          color_2nd = color_1st;
          layer_1st = 0U;
          color_1st = color_3d;
          is_alpha_obj = false;
        } else if(layer_2nd == LAYER_BD || u_bg0_priority < priority_2nd || (u_bg0_priority == priority_2nd && layer_2nd != LAYER_OBJ)) {
          layer_2nd = 0U;
          color_2nd = color_3d;
        }
      }
    }

    bool have_1st_target = (u_blend_control & (  1 << layer_1st)) != 0;
    bool have_2nd_target = (u_blend_control & (256 << layer_2nd)) != 0;

    int blend_effect = (u_blend_control >> 6) & 3;

    if(is_alpha_obj && have_2nd_target) {
      color_1st.rgb = color_1st.rgb * u_blend_eva + color_2nd.rgb * u_blend_evb;
    } else if(blend_effect == SFX_BLEND) {
      if(have_2nd_target) {
        if(layer_1st == 0U && u_enable_bg0_3d) {
          // @todo: should sfx_enable be respected in this scenario?
          color_1st.rgb = mix(color_2nd.rgb, color_1st.rgb, color_1st.a);
        } else if(have_1st_target && enable_sfx) {
          color_1st.rgb = color_1st.rgb * u_blend_eva + color_2nd.rgb * u_blend_evb;
        }
      }
    } else if(have_1st_target && enable_sfx){
      // none, brighten, darken
      switch(blend_effect) {
        case SFX_BRIGHTEN: {
          color_1st.rgb += (1.0 - color_1st.rgb) * u_blend_evy;
          break;
        }
        case SFX_DARKEN: {
          color_1st.rgb -= color_1st.rgb * u_blend_evy;
          break;
        }
      }
    }

    switch(u_master_bright_mode) {
      case MASTER_BRIGHT_UP: {
        color_1st.rgb += (1.0 - color_1st.rgb) * u_master_bright_factor;
        break;
      }
      case MASTER_BRIGHT_DOWN: {
        color_1st.rgb -= color_1st.rgb * u_master_bright_factor;
        break;
      }
    }

    frag_color.rgb = color_1st.rgb;
    frag_color.a = 1.0;
  }
)";