/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

constexpr auto fog_vert = R"(
  #version 330 core

  layout(location = 0) in vec2 a_position;
  layout(location = 1) in vec2 a_uv;

  out vec2 v_uv;

  void main() {
    v_uv = a_uv;

    gl_Position = vec4(a_position, 0.0, 1.0);
  }
)";

constexpr auto fog_frag = R"(
  #version 330 core

  layout(location = 0) out vec4 frag_color;

  in vec2 v_uv;

  uniform sampler2D u_depth_map;
  uniform sampler2D u_fog_density_table;
  uniform sampler2D u_color_map;

  uniform float u_fog_offset;
  uniform float u_fog_width;
  uniform vec4 u_fog_color;
  uniform vec4 u_fog_blend_mask;

  void main() {
    float depth = texture2D(u_depth_map, v_uv).r;

    float s = (depth - u_fog_offset) / u_fog_width;
    s = texture2D(u_fog_density_table, vec2(s, 0.0)).r;

    frag_color = mix(texture2D(u_color_map, v_uv), u_fog_color, s * u_fog_blend_mask);
  }
)";