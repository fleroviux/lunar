/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

constexpr auto edge_marking_vert = R"(
  #version 330 core

  layout(location = 0) in vec2 a_position;
  layout(location = 1) in vec2 a_uv;

  out vec2 v_uv;
  out vec2 v_uv_n;
  out vec2 v_uv_s;
  out vec2 v_uv_w;
  out vec2 v_uv_e;

  uniform sampler2D u_color_map;

  void main() {
    vec2 inv_resolution = 1.0 / textureSize(u_color_map, 0);

    v_uv = a_uv;
    v_uv_n = v_uv - vec2(0.0, inv_resolution.y);
    v_uv_s = v_uv + vec2(0.0, inv_resolution.y);
    v_uv_w = v_uv - vec2(inv_resolution.x, 0.0);
    v_uv_e = v_uv + vec2(inv_resolution.x, 0.0);

    gl_Position = vec4(a_position, 0.0, 1.0);
  }
)";

constexpr auto edge_marking_frag = R"(
  #version 330 core

  layout(location = 0) out vec4 frag_color;

  in vec2 v_uv;
  in vec2 v_uv_n;
  in vec2 v_uv_s;
  in vec2 v_uv_w;
  in vec2 v_uv_e;

  uniform sampler2D u_depth_map;
  uniform sampler2D u_attribute_map;

  uniform vec3 u_edge_colors[8];

  void main() {
    float center_depth = texture2D(u_depth_map, v_uv).r;
    float center_poly_id = texture2D(u_attribute_map, v_uv).r;

    bool edge = false;
    edge = edge || (texture2D(u_attribute_map, v_uv_n).r != center_poly_id && texture2D(u_depth_map, v_uv_n).r < center_depth);
    edge = edge || (texture2D(u_attribute_map, v_uv_s).r != center_poly_id && texture2D(u_depth_map, v_uv_s).r < center_depth);
    edge = edge || (texture2D(u_attribute_map, v_uv_w).r != center_poly_id && texture2D(u_depth_map, v_uv_w).r < center_depth);
    edge = edge || (texture2D(u_attribute_map, v_uv_e).r != center_poly_id && texture2D(u_depth_map, v_uv_e).r < center_depth);

    frag_color = vec4(u_edge_colors[uint(center_poly_id * 7.0)], edge ? 1.0 : 0.0);
  }
)";