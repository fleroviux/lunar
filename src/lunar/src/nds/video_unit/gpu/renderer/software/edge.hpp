/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include "nds/video_unit/gpu/gpu.hpp"

namespace lunar::nds {

// @todo: what format should normalized w-Coordinates be stored in?
struct Point {
  s32 x;
  s32 y;
  u32 depth;
  s32 w;
  GPU::Vertex const* vertex;
};

class Edge {
  public:
    using fixed14x18 = s32;

    Edge(Point const& p0, Point const& p1) : p0(&p0), p1(&p1) {
      CalculateSlope();
    }

    fixed14x18 XSlope() { return x_slope; }

    bool& IsXMajor() { return x_major; }

    void Interpolate(s32 y, fixed14x18& x0, fixed14x18& x1) {
      if (x_major) {
        // TODO: make sure that the math is correct (especially negative slopes)
        if (x_slope >= 0) {
          x0 = (p0->x << 18) + x_slope * (y - p0->y) + (1 << 17);

          if (y != p1->y || flat_horizontal) {
            x1 = (x0 & ~0x1FF) + x_slope - (1 << 18);
          } else {
            x1 = x0;
          }
        } else {
          x1 = (p0->x << 18) + x_slope * (y - p0->y) + (1 << 17);

          if (y != p1->y || flat_horizontal) {
            x0 = (x1 & ~0x1FF) + x_slope;
          } else {
            x0 = x1;
          }
        }
      } else {
        x0 = (p0->x << 18) + x_slope * (y - p0->y);
        x1 = x0;
      }
    }

  private:
    void CalculateSlope() {
      s32 x_diff = p1->x - p0->x;
      s32 y_diff = p1->y - p0->y;

      if (y_diff == 0) {
        x_slope = x_diff << 18;
        x_major = std::abs(x_diff) > 1;
        flat_horizontal = true;
      } else {
        fixed14x18 y_reciprocal = (1 << 18) / y_diff;

        x_slope = x_diff * y_reciprocal;
        x_major = std::abs(x_diff) > std::abs(y_diff);
        flat_horizontal = false;
      }
    }

    Point const* p0;
    Point const* p1;
    fixed14x18 x_slope;
    bool x_major;
    bool flat_horizontal;
};

} // namespace lunar::nds
