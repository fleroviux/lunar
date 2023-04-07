/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <algorithm>

#include "software_renderer.hpp"

namespace lunar::nds {

void SoftwareRenderer::RenderEdgeMarking() {
  bool edge;

  // @todo: the expanded clear depth is already calculated in "RenderRearPlane". Do not calculate it twice.
  auto border_depth = (u32)((clear_depth.depth << 9) + ((clear_depth.depth + 1) >> 15) * 0x1FF);
  auto border_poly_id = clear_color.polygon_id;

  for (int y = 0; y < 192; y++) {
    for (int x = 0; x < 256; x++) {
      int c = y * 256 + x;

      auto attributes = attribute_buffer[c];

      if (attributes.flags & ATTRIBUTE_FLAG_EDGE) {
        int l = c - 1;
        int r = c + 1;
        int u = c - 256;
        int d = c + 256;

        u32 depth = depth_buffer[c];
        int poly_id = attributes.poly_id[0];
        bool border_edge = border_poly_id != poly_id && depth < border_depth;

        edge  = x == 0   ? border_edge : (attribute_buffer[l].poly_id[0] != poly_id && depth < depth_buffer[l]);
        edge |= x == 255 ? border_edge : (attribute_buffer[r].poly_id[0] != poly_id && depth < depth_buffer[r]);
        edge |= y == 0   ? border_edge : (attribute_buffer[u].poly_id[0] != poly_id && depth < depth_buffer[u]);
        edge |= y == 191 ? border_edge : (attribute_buffer[d].poly_id[0] != poly_id && depth < depth_buffer[d]);

        if (edge) {
          // TODO: decode color on write to the edge color table.
          color_buffer[c] = Color4::FromRGB555(edge_color_table[poly_id >> 3]);
        }
      }
    }
  }
}

} // namespace lunar::nds
