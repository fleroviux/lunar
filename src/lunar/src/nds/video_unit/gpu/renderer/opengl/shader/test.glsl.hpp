/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

constexpr auto test_vert = R"(
  #version 330 core

  layout(location = 0) in vec4 a_position;
  layout(location = 1) in vec4 a_color;
  layout(location = 2) in vec2 a_uv;

  out vec4 v_color;
  out vec2 v_uv;

  void main() {
    v_color = a_color;
    v_uv = a_uv;
    gl_Position = a_position;
  }
)";

constexpr auto test_frag = R"(
  #version 330 core

  layout(location = 0) out vec4 frag_color;

  in vec4 v_color;
  in vec2 v_uv;

  // TODO: properly initialize texture uniform
  uniform sampler2D u_map;

  void main() {
    vec2 scaled_uv = v_uv / vec2(textureSize(u_map, 0));

    //frag_color = vec4(scaled_uv, 0.0, 1.0);
    frag_color = v_color * texture2D(u_map, scaled_uv);
  }
)";