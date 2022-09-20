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
  layout(location = 1) out vec4 frag_poly_id;

  in vec4 v_color;
  in vec2 v_uv;

  uniform bool u_discard_translucent_pixels;

  uniform float u_polygon_alpha;
  uniform float u_polygon_id;

  uniform bool u_use_map;
  uniform bool u_use_alpha_test;
  uniform float u_alpha_test_threshold;

  // TODO: properly initialize texture uniform
  uniform sampler2D u_map;

  void main() {
    vec2 uv = v_uv / vec2(textureSize(u_map, 0));

    vec4 color = vec4(v_color.rgb, u_polygon_alpha);

    if (u_use_map) {
      vec4 texel = texture2D(u_map, uv);

      if (texel.a <= (u_use_alpha_test ? u_alpha_test_threshold : 0.0)) {
        discard;
      }

      color *= texel;
    }

    if (u_discard_translucent_pixels && color.a < 1.0) {
      discard;
    }

    frag_color = color;
    frag_poly_id = vec4(u_polygon_id, 0.0, 0.0, 1.0);
  }
)";