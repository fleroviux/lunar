/*
 * Copyright (C) 2021 fleroviux
 */

#include "gpu.hpp"

namespace Duality::Core {

// TODO: what format should normalized w-Coordinates be stored in?

// TODO: move this into the appropriate class and remove the GPU:: prefix.
struct Point {
  s32 x;
  s32 y;
  u32 depth;
  s32 w;
  GPU::Vertex const* vertex;
};

struct Edge {
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

template<int precision>
struct Interpolator {
  void Setup(u16 w0, u16 w1, s32 x, s32 x_min, s32 x_max) {
    CalculateLerpFactor(x, x_min, x_max);
    CalculatePerpFactor(w0, w1, x, x_min, x_max);

    // TODO: overwrite the perp factor instead of setting a flag?
    if constexpr (precision == 9) {
      force_lerp = w0 == w1 && (w0 & 0x7E) == 0 && (w1 & 0x7E) == 0;
    } else {
      force_lerp = w0 == w1 && (w0 & 0x7F) == 0 && (w1 & 0x7F) == 0;
    }
  }

  // TODO: use correct formulas and clean this up.

  template<typename T>
  auto Interpolate(T a, T b) -> T {
    auto factor = force_lerp ? factor_lerp : factor_perp;

    return (a * ((1 << precision) - factor) + b * factor) >> precision;
  }

  template<typename T>
  auto InterpolateLinear(T a, T b) -> T {
    return (a * ((1 << precision) - factor_lerp) + b * factor_lerp) >> precision;
  }

  // TODO: generalize this for arbitrary vectors?

  auto Interpolate(Color4 const& color_a, Color4 const& color_b, Color4& color_out) {
    auto factor = force_lerp ? factor_lerp : factor_perp;

    for (int i = 0; i < 3; i++) {
      color_out[i] = (color_a[i].raw() * ((1 << precision) - factor) + color_b[i].raw() * factor) >> precision;

      // // Formula from melonDS interpolation article.
      // // is there actually a mathematical difference?
      // // This certainly causes interpolation issues unless we expand the interpolated values to higher precision.
      // auto a = color_a[i].raw();
      // auto b = color_b[i].raw();
      // if (a < b) {
      //   color_outi] = (a + (b - a) * factor) >> precision;
      // } else {
      //   color_out[i] = (b + (a - b) * (((1 << precision) - 1) - factor)) >> precision;
      // }
    }
  }

  auto Interpolate(Vector2<Fixed12x4> const& uv_a, Vector2<Fixed12x4> const& uv_b, Vector2<Fixed12x4>& uv_out) {
    auto factor = force_lerp ? factor_lerp : factor_perp;

    for (int i = 0; i < 2; i++) {
      uv_out[i] = (uv_a[i].raw() * ((1 << precision) - factor) + uv_b[i].raw() * factor) >> precision;
    }
  }

private:
  void CalculateLerpFactor(s32 x, s32 x_min, s32 x_max) {
    // TODO: make sure that this uses the correct amount of precision.
    u32 denominator = x_max - x_min;
    u32 numerator = (x - x_min) << precision;

    // TODO: how does the DS GPU deal with division-by-zero?
    if (denominator != 0) {
      factor_lerp = numerator / denominator;
    } else {
      factor_lerp = 0;
    }
  }

  void CalculatePerpFactor(u16 w0, u16 w1, s32 x, s32 x_min, s32 x_max) {
    u16 w0_num = w0;
    u16 w0_denom = w0;
    u16 w1_denom = w1;

    if constexpr(precision == 9) {
      w0_num   >>= 1;
      w0_denom >>= 1;
      w1_denom >>= 1;

      if ((w0 & 1) && !(w1 & 1)) {
        w0_denom++;
      }
    }

    u32 t0 = x - x_min;
    u32 t1 = x_max - x;
    u32 denominator = t1 * w1_denom + t0 * w0_denom;
    u32 numerator = (t0 << precision) * w0_num;

    // TODO: how does the DS handle division-by-zero here?
    if (denominator != 0) {
      factor_perp = numerator / denominator;
    } else {
      factor_perp = 0;
    }
  }

  u32 factor_lerp;
  u32 factor_perp;
  bool force_lerp;
};

struct Span {
  s32 x0[2];
  s32 x1[2];
  s32 w[2];
  u32 depth[2];
  Vector2<Fixed12x4> uv[2];
  Color4 color[2];
};

void GPU::RenderRearPlane() {
  if (disp3dcnt.enable_rear_bitmap) {
    ASSERT(false, "GPU: unhandled rear bitmap");
  } else {
    auto color = Color4{
      (s8)((clear_color.color_r << 1) | (clear_color.color_r >> 4)),
      (s8)((clear_color.color_g << 1) | (clear_color.color_g >> 4)),
      (s8)((clear_color.color_b << 1) | (clear_color.color_b >> 4)),
      (s8)((clear_color.color_a << 1) | (clear_color.color_a >> 4))
    };

    auto depth = (s32)((clear_depth.depth << 9) + ((clear_depth.depth + 1) >> 15) * 0x1FF);

    auto stencil = clear_color.polygon_id; 

    for (uint i = 0; i < 256 * 192; i++) {
      draw_buffer[i] = color;
    }

    for (uint i = 0; i < 256 * 192; i++) {
      depth_buffer[i] = depth;
    }

    for (uint i = 0; i < 256 * 192; i++) {
      stencil_buffer[i] = stencil;
    }
  }
}

void GPU::RenderPolygons(bool translucent) {
  Point points[10];
  Span span;

  const s32 depth_test_threshold = use_w_buffer ? 0xFF : 0x200;

  // TODO: use real viewport information
  const int viewport_x = 0;
  const int viewport_y = 0;
  const int viewport_width = 255;
  const int viewport_height = 191;

  auto buffer_id = gx_buffer_id ^ 1;
  auto& vert_ram = vertex[buffer_id];
  auto& poly_ram = polygon[buffer_id];
  auto poly_count = poly_ram.count;

  for (int i = 0; i < poly_count; i++) {
    Polygon const& poly = poly_ram.data[i];

    if (poly.params.mode == PolygonParams::Mode::Decal || poly.params.mode == PolygonParams::Mode::Shaded) {
      ASSERT(false, "GPU: unhandled polygon mode: {}", (int)poly.params.mode);
    }

    int start;
    int end;
    s32 y_min = 256;
    s32 y_max = 0;

    auto vert_count = poly.count;

    for (int j = 0; j < vert_count; j++) {
      auto& point = points[j];
      auto const& vert = vert_ram.data[poly.indices[j]];

      auto w = vert.position.w().raw();
      auto two_w = w << 1;

      point.x = (( (s64)vert.position.x().raw() + w) * viewport_width  / two_w) + viewport_x;
      point.y = ((-(s64)vert.position.y().raw() + w) * viewport_height / two_w) + viewport_y;
      point.depth = (u32)(((((s64)vert.position.z().raw() << 14) / w) + 0x3FFF) << 9);
      point.w = w;
      point.vertex = &vert;

      // Pick the first vertex with the lowest y-Coordinate as the start node.
      // Also update the minimum y-Coordinate.
      if (point.y < y_min) {
        y_min = point.y;
        start = j;
      }

      // Update the maximum y-Coordinate
      if (point.y > y_max) {
        y_max = point.y;
        end = j;
      }
    }

    int w_shift = 0;

    // w-normalization.
    // TODO: move this to the correct place in the pipeline
    // Also make sure that this is actually correct.
    {
      int min_leading = 32;

      for (int j = 0; j < vert_count; j++) {
        auto const& point = points[j];

        if (point.w < 0) {
          min_leading = std::min(min_leading, __builtin_clz(~point.w));
        } else {
          min_leading = std::min(min_leading, __builtin_clz( point.w));
        }
      }

      if (min_leading < 16) {
        w_shift = 16 - min_leading;

        if ((w_shift & 3) != 0) {
          w_shift += 4 - (w_shift & 3);
        }

        for (int j = 0; j < vert_count; j++) {
          points[j].w >>= w_shift;
        }
      } else if (min_leading > 16) {
        w_shift = (min_leading - 16) & ~3 ;
      
        for (int j = 0; j < vert_count; j++) {
          points[j].w <<= w_shift;
        }

        w_shift = -w_shift;
      }
    }

    int s[2];
    int e[2];

    // first edge (CW)
    s[0] = start;
    e[0] = start == (vert_count - 1) ? 0 : (start + 1);

    // second edge (CCW)
    s[1] = start;
    e[1] = start == 0 ? (vert_count - 1) : (start - 1);

    Edge edge[2]{
      {points[s[0]], points[e[0]]},
      {points[s[1]], points[e[1]]}
    };

    auto edge_interpolator = Interpolator<9>{};
    auto span_interpolator = Interpolator<8>{};

    auto alpha = (poly.params.alpha << 1) | (poly.params.alpha >> 4);
    auto poly_id = poly.params.polygon_id;
    bool wireframe = alpha == 0;
    bool force_draw_edges_a = alpha != 63 || disp3dcnt.enable_antialias || disp3dcnt.enable_edge_marking;

    if (wireframe) {
      alpha = 63;
    }

    // Opaque and translucent polygons are rendered in separate passes.
    // TODO: move this check to an earlier point.
    if (translucent != (alpha != 63)) {
      continue;
    }

    int alpha_threshold = 0;

    if (disp3dcnt.enable_alpha_test) {
      alpha_threshold = (alpha_test_ref.alpha << 1) | (alpha_test_ref.alpha >> 4);
    }

    for (s32 y = y_min; y <= y_max; y++) {
      bool force_draw_edges_b = force_draw_edges_a || y == 191;

      Edge::fixed14x18 x0[2];
      Edge::fixed14x18 x1[2];

      // interpolate both edges vertically
      for (int j = 0; j < 2; j++) {
        edge[j].Interpolate(y, x0[j], x1[j]);
      }

      // Left and right edge indices
      int l;
      int r;

      // TODO: can we get rid of the second condition?
      // Right now it exists to handle the case where the second edge is contained within the first edge.
      if ((x1[0] >> 18) > (x0[1] >> 18) && (x0[1] >> 18) <= (x0[0] >> 18)) {
        l = 1;
        r = 0;
      } else {
        l = 0;
        r = 1;
      }

      for (int j = 0; j < 2; j++) {
        auto const& p0 = points[s[j]];
        auto const& p1 = points[e[j]];

        s32 w0 = points[s[j]].w;
        s32 w1 = points[e[j]].w;

        if (edge[j].IsXMajor()) {
          // TODO: is it accurate to truncate x like this?
          s32 x = ((j == l) ? x0[l] : x1[r]) >> 18;
          s32 x_min = p0.x;
          s32 x_max = p1.x;

          if (x_min > x_max) {
            std::swap(x_min, x_max);
            x = x_max - (x - x_min);
          }

          edge_interpolator.Setup(w0, w1, x, x_min, x_max);
        } else {
          edge_interpolator.Setup(w0, w1, y, p0.y, p1.y);
        }

        // TODO: is it accurate to reduce the precision like that?
        span.x0[j] = x0[j] >> 18;
        span.x1[j] = x1[j] >> 18;
        span.w[j] = edge_interpolator.Interpolate(p0.w, p1.w);
        if (use_w_buffer) {
          s32 w = span.w[j];

          if (w_shift >= 0) {
            w <<= w_shift;
          } else {
            w >>= w_shift;
          }

          span.depth[j] = (u32)w;
        } else {
          span.depth[j] = edge_interpolator.InterpolateLinear(p0.depth, p1.depth);
        }
        edge_interpolator.Interpolate(p0.vertex->uv, p1.vertex->uv, span.uv[j]);
        edge_interpolator.Interpolate(p0.vertex->color, p1.vertex->color, span.color[j]);
      }

      // TODO: preferrably handle this outside the rasterization loop
      // by limiting the minimum and maximum y-values.
      if (y >= 0 && y <= 191) {
        const auto min_x = span.x0[l];
        const auto max_x = span.x1[r];

        const auto render_span = [&](s32 x0, s32 x1) {
          auto uv = Vector2<Fixed12x4>{};
          auto color = Color4{};

          for (s32 x = x0; x <= x1; x++) {
            // TODO: clamp x_min and x_max instead
            if (x < 0 || x > 255) continue;

            int index = y * 256 + x;

            // TODO: cache calculations that do not depend on x.
            span_interpolator.Setup(span.w[l], span.w[r], x, min_x, max_x);

            u32 depth_old = depth_buffer[index];
            u32 depth_new;

            if (use_w_buffer) {
              depth_new = span_interpolator.Interpolate(span.depth[l], span.depth[r]);
            } else {
              depth_new = span_interpolator.InterpolateLinear(span.depth[l], span.depth[r]);
            }

            bool depth_test_passed;

            if (poly.params.depth_test == PolygonParams::DepthTest::Less) {
              depth_test_passed = depth_new < depth_old;
            } else {
              depth_test_passed = std::abs((s32)depth_new - (s32)depth_old) <= depth_test_threshold;
            }

            if (!depth_test_passed) {
              if (poly.params.mode == PolygonParams::Mode::Shadow && poly_id == 0) {
                stencil_buffer[index] = 0x80;
              }
              continue;
            }

            span_interpolator.Interpolate(span.uv[l], span.uv[r], uv);
            span_interpolator.Interpolate(span.color[l], span.color[r], color);

            color.a() = alpha;

            if (disp3dcnt.enable_textures) {
              auto texel = SampleTexture(poly.texture_params, uv);
              if (texel.a() <= alpha_threshold) {
                continue;
              }
              color *= texel;
            }

            if (disp3dcnt.enable_alpha_blend && draw_buffer[index].a() != 0) {
              auto a0 = color.a();
              auto a1 = Fixed6{63} - a0;
              for (uint j = 0; j < 3; j++)
                color[j] = color[j] * a0 + draw_buffer[index][j] * a1;
              color.a() = std::max(color.a(), draw_buffer[index].a());
            }

            // TODO: make sure that shadow polygon logic is correct.
            if (poly.params.mode == PolygonParams::Mode::Shadow) {
              if (poly_id == 0) {
                stencil_buffer[index] = 0;
              } else {
                if ((stencil_buffer[index] & 0x80) && (stencil_buffer[index] & 0x3F) != poly_id) {
                  draw_buffer[index] = color;
                }
                stencil_buffer[index] = poly_id;
              }
            } else {
              draw_buffer[index] = color;
              stencil_buffer[index] = poly_id;
            }

            if (!translucent || poly.params.enable_translucent_depth_write) {
              depth_buffer[index] = depth_new;
            }
          }
        };

        if (edge[r].XSlope() == 0) {
          // TODO: the horizontal attribute interpolator's rightmost X coordinate is incremented by one
          span.x0[r]--;
          span.x1[r]--;
        }

        if (force_draw_edges_b || edge[l].XSlope() < 0 || !edge[l].IsXMajor()) {
          render_span(span.x0[l], span.x1[l]);
        }
        
        if (!wireframe) {
          render_span(span.x1[l] + 1, span.x0[r] - 1);
        }
        
        if (force_draw_edges_b || (edge[r].XSlope() > 0 && edge[r].IsXMajor()) || edge[r].XSlope() == 0) {
          render_span(span.x0[r], span.x1[r]);
        }
      }

      auto next_y = y + 1;

      // update clock-wise edge
      if (points[e[0]].y <= next_y && e[0] != end) {
        s[0] = e[0];
        if (++e[0] == vert_count)
          e[0] = 0;
        edge[0] = Edge{points[s[0]], points[e[0]]};

        //LOG_INFO("GPU:  CW edge advance @ y={} (next y-target: {})", y, points[e[0]].y);
      }

      // update counter clock-wise edge
      if (points[e[1]].y <= next_y && e[1] != end) {
        s[1] = e[1];
        if (--e[1] == -1)
          e[1] = vert_count - 1;
        edge[1] = Edge{points[s[1]], points[e[1]]};

        //LOG_INFO("GPU: CCW edge advance @ y={} (next y-target: {})", y, points[e[1]].y);
      }
    }
  }
}

void GPU::Render() {
  RenderRearPlane();
  RenderPolygons(false);
  RenderPolygons(true);
}

} // namespace Duality::Core
