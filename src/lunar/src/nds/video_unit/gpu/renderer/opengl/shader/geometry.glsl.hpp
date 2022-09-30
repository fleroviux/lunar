/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

constexpr auto geometry_vert = R"(
  #version 330 core

  layout(location = 0) in vec4 a_position;
  layout(location = 1) in vec4 a_color;
  layout(location = 2) in vec2 a_uv;

  out vec4 v_color;
  out vec2 v_uv;
  out float v_w_coordinate;

  void main() {
    v_color = a_color;
    v_uv = a_uv;
    v_w_coordinate = a_position.w;
    gl_Position = a_position;
  }
)";

constexpr auto geometry_frag = R"(
  #version 330 core

  layout(location = 0) out vec4 frag_color;
  layout(location = 1) out vec4 frag_attributes;

  in vec4 v_color;
  in vec2 v_uv;
  in float v_w_coordinate;

  uniform bool u_discard_translucent_pixels;
  uniform bool u_use_w_buffer;

  uniform float u_polygon_alpha;
  uniform float u_polygon_id;
  uniform int u_polygon_mode;
  uniform int u_shading_mode;
  uniform float u_fog_flag;

  uniform bool u_use_map;
  uniform bool u_use_alpha_test;
  uniform float u_alpha_test_threshold;

  uniform sampler2D u_map;
  uniform sampler2D u_toon_table;

  void main() {
    vec2 uv = v_uv / vec2(textureSize(u_map, 0));

    vec4 color = vec4(v_color.rgb, u_polygon_alpha);

    if (u_use_map) {
      vec4 texel = texture2D(u_map, uv);

      if (texel.a <= (u_use_alpha_test ? u_alpha_test_threshold : 0.0)) {
        discard;
      }

      switch (u_polygon_mode) {
        case 0: {
          // modulation
          color *= texel;
          break;
        }
        case 1:
        case 3: {
          // decal (and textured shadow polygons)
          color.rgb = mix(color.rgb, texel.rgb, texel.a);
          break;
        }
        case 2: {
          // shaded
          vec4 toon_color = texture2D(u_toon_table, vec2(color.r, 0.0));

          if (u_shading_mode == 0) {
            color.rgb = texel.rgb * toon_color.rgb;
          } else {
            color.rgb = min(texel.rgb * color.rgb + toon_color.rgb, vec3(1.0));
          }

          color.a = texel.a * color.a;
          break;
        }
      }
    } else if (u_polygon_mode == 2) {
      // shaded polygon mode with no texture
      vec4 toon_color = texture2D(u_toon_table, vec2(color.r, 0.0));

      if (u_shading_mode == 0) {
        color.rgb = toon_color.rgb;
      } else {
        color.rgb = min(color.rgb + toon_color.rgb, vec3(1.0));
      }
    }

    if (u_discard_translucent_pixels && color.a < 1.0) {
      discard;
    }

    frag_color = color;
    frag_attributes = vec4(u_polygon_id, u_fog_flag, 0.0, 1.0);

    gl_FragDepth = gl_FragCoord.z;

    if (u_use_w_buffer) {
      /* On the DS GPU the depth buffer has 24-bit precision and
       * the w-coordinate is a 32-bit fixed point number with a 12-bit fractional part.
       * This means the lower 12-bit of the *integer* part span the full 24-bit depth buffer range.
       * To get a depth value in the 0 to 1 range we should therefore divide the w-coordinate by 2^12.
       */
      gl_FragDepth = v_w_coordinate * 0.000244140625;
    }
  }
)";
