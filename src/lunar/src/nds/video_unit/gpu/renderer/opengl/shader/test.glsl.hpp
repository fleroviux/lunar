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

  out vec4 v_color;

  void main() {
    v_color = a_color;
    gl_Position = a_position;
  }
)";

constexpr auto test_frag = R"(
  #version 330 core

  layout(location = 0) out vec4 frag_color;

  in vec4 v_color;

  void main() {
    frag_color = v_color;
  }
)";