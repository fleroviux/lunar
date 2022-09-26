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

  layout(location = 0) out vec4 frag_color;

  in vec2 v_uv;

  uniform sampler2D u_color2d_map;
  uniform sampler2D u_color3d_map;

  void main() {
    vec4 color_2d = texture2D(u_color2d_map, v_uv);
    vec4 color_3d = texture2D(u_color3d_map, v_uv);

    frag_color = vec4(mix(color_2d.rgb, color_3d.rgb, color_3d.a), 1.0);
  }
)";