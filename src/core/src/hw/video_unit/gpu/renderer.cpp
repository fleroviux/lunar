/*
 * Copyright (C) 2021 fleroviux
 */

#include "gpu.hpp"

namespace Duality::Core {

auto GPU::SampleTexture(TextureParams const& params, Vector2<Fixed12x4> const& uv) -> Color4 {
  const int size[2] {
    8 << params.size[0],
    8 << params.size[1]
  };

  int coord[2] { uv.x().integer(), uv.y().integer() };

  for (int i = 0; i < 2; i++) {
    if (coord[i] < 0 || coord[i] >= size[i]) {
      int mask = size[i] - 1;
      if (params.repeat[i]) {
        bool odd = (coord[i] >> (3 + params.size[i])) & 1;
        coord[i] &= mask;
        if (params.flip[i] && odd) {
          coord[i] ^= mask;
        }
      } else {
        coord[i] = std::clamp(coord[i], 0, mask);
      }
    }
  }

  auto offset = coord[1] * size[0] + coord[0];
  auto palette_addr = params.palette_base << 4;

  switch (params.format) {
    case TextureParams::Format::None: {
      return Color4{};
    }
    case TextureParams::Format::A3I5: {
      u8  value = vram_texture.Read<u8>(params.address + offset);
      int index = value & 0x1F;
      int alpha = value >> 5;

      auto rgb555 = vram_palette.Read<u16>(palette_addr + index * sizeof(u16)) & 0x7FFF;
      auto rgb6666 = Color4::from_rgb555(rgb555);
      
      if (params.color0_transparent && index == 0) {
        rgb6666.a() = 0;
      } else {
        rgb6666.a() = (alpha << 3) | alpha; // 3-bit alpha to 6-bit alpha  
      }

      return rgb6666;
    }
    case TextureParams::Format::Palette2BPP: {
      auto index = (vram_texture.Read<u8>(params.address + (offset >> 2)) >> (2 * (offset & 3))) & 3;

      if (params.color0_transparent && index == 0) {
        return Color4{0, 0, 0, 0};
      }
      
      return Color4::from_rgb555(vram_palette.Read<u16>((palette_addr >> 1) + index * sizeof(u16)) & 0x7FFF);
    }
    case TextureParams::Format::Palette4BPP: {
      auto index = (vram_texture.Read<u8>(params.address + (offset >> 1)) >> (4 * (offset & 1))) & 15;
      
      if (params.color0_transparent && index == 0) {
        return Color4{0, 0, 0, 0};
      }
      
      return Color4::from_rgb555(vram_palette.Read<u16>(palette_addr + index * sizeof(u16)) & 0x7FFF);
    }
    case TextureParams::Format::Palette8BPP: {
      auto index = vram_texture.Read<u8>(params.address + offset);

      if (params.color0_transparent && index == 0) {
        return Color4{0, 0, 0, 0};
      }

      return Color4::from_rgb555(vram_palette.Read<u16>(palette_addr + index * sizeof(u16)) & 0x7FFF);
    }
    case TextureParams::Format::Compressed4x4: {
      auto row_x = coord[0] >> 2;
      auto row_y = coord[1] >> 2;
      auto tile_x = coord[0] & 3;
      auto tile_y = coord[1] & 3;
      auto row_size = size[0] >> 2;

      auto data_address = params.address + (row_y * row_size + row_x) * sizeof(u32) + tile_y;

      auto data_slot_index  = data_address >> 18;
      auto data_slot_offset = data_address & 0x1FFFF;
      auto info_address = 0x20000 + (data_slot_offset >> 1) + (data_slot_index * 0x10000);

      auto data = vram_texture.Read<u8>(data_address); 
      auto info = vram_texture.Read<u16>(info_address);

      auto index = (data >> (tile_x * 2)) & 3;
      auto palette_offset = info & 0x3FFF;
      auto mode = info >> 14;

      palette_addr += palette_offset << 2;

      switch (mode) {
        case 0: {
          if (index == 3) {
            return Color4{0, 0, 0, 0};
          }
          return Color4::from_rgb555(vram_palette.Read<u16>(palette_addr + index * sizeof(u16)) & 0x7FFF);
        }
        case 1: {
          if (index == 2) {
            auto color_0 = Color4::from_rgb555(vram_palette.Read<u16>(palette_addr + 0) & 0x7FFF);
            auto color_1 = Color4::from_rgb555(vram_palette.Read<u16>(palette_addr + 2) & 0x7FFF);

            for (uint i = 0; i < 3; i++) {
              color_0[i] = Fixed6{s8((color_0[i].raw() >> 1) + (color_1[i].raw() >> 1))};
            }

            return color_0;
          }
          if (index == 3) {
            return Color4{0, 0, 0, 0};
          }
          return Color4::from_rgb555(vram_palette.Read<u16>(palette_addr + index * sizeof(u16)) & 0x7FFF);
        }
        case 2: {
          return Color4::from_rgb555(vram_palette.Read<u16>(palette_addr + index * sizeof(u16)) & 0x7FFF);
        }
        default: {
          if (index == 2 || index == 3) {
            int coeff_0 = index == 2 ? 5 : 3;
            int coeff_1 = index == 2 ? 3 : 5;

            auto color_0 = Color4::from_rgb555(vram_palette.Read<u16>(palette_addr + 0) & 0x7FFF);
            auto color_1 = Color4::from_rgb555(vram_palette.Read<u16>(palette_addr + 2) & 0x7FFF);

            for (uint i = 0; i < 3; i++) {
              color_0[i] = Fixed6{s8(((color_0[i].raw() * coeff_0) + (color_1[i].raw() * coeff_1)) >> 3)};
            }

            return color_0;
          }
          return Color4::from_rgb555(vram_palette.Read<u16>(palette_addr + index * sizeof(u16)) & 0x7FFF);
        }
      }
    }
    case TextureParams::Format::A5I3: {
      u8  value = vram_texture.Read<u8>(params.address + offset);
      int index = value & 7;
      int alpha = value >> 3;

      auto rgb555 = vram_palette.Read<u16>((params.palette_base << 4) + index * sizeof(u16)) & 0x7FFF;
      auto rgb6666 = Color4::from_rgb555(rgb555);
      
      if (params.color0_transparent && index == 0) {
        rgb6666.a() = 0;
      } else {
        rgb6666.a() = (alpha << 1) | (alpha >> 4); // 5-bit alpha to 6-bit alpha  
      }

      return rgb6666;
    }
    case TextureParams::Format::Direct: {
      auto rgb1555 = vram_texture.Read<u16>(params.address + offset * sizeof(u16));
      auto rgb6666 = Color4::from_rgb555(rgb1555);

      rgb6666.a() = rgb6666.a().raw() * (rgb1555 >> 15);
      return rgb6666;
    }
  };

  return Color4{};
}

// TODO: what format should normalized w-Coordinates be stored in?

// TODO: move this into the appropriate class and remove the GPU:: prefix.
struct Point {
  s32 x;
  s32 y;
  u32 depth;
  s32 w_norm;
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

    if (flat_horizontal) {
      LOG_ERROR("")
    }
  }

private:
  void CalculateSlope() {
    s32 x_diff = p1->x - p0->x;
    s32 y_diff = p1->y - p0->y;

    // TODO: how does hardware handle this edge-case? Does it ever happen?
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
  void Setup(s32 w0, s32 w1, u16 w0_norm, u16 w1_norm, s32 x, s32 x_min, s32 x_max) {
    CalculateLerpFactor(x, x_min, x_max);
    CalculatePerpFactor(w0_norm, w1_norm, x, x_min, x_max);

    // LOG_ERROR("GPU: w0={:08X} w1={:08X} w0n={:04X} w1n={:04X} x={} x_min={} x_max={}",
    //   w0, w1, w0_norm, w1_norm, x, x_min, x_max);

    // TODO: use 127 instead of 126 for span interpolation.
    // Also, if possible, simply overwrite the perp factor with the lerp factor if this is true.
    force_lerp = w0 == w1 && (w0 & 126) == 0 && (w1 & 126) == 0;
  }

  // TODO: use correct formulas and clean this up.

  template<typename T>
  auto Interpolate(T a, T b) -> T {
    auto factor = force_lerp ? factor_lerp : factor_perp;

    return (a * ((1 << precision) - factor) + b * factor) >> precision;
  }

  // TODO: generalize this for arbitrary vectors?

  auto Interpolate(Color4 const& color_a, Color4 const& color_b, Color4& color_out) {
    auto factor = force_lerp ? factor_lerp : factor_perp;

    // if (factor > 512) {
    //   LOG_ERROR("GPU: bad factor: {}", factor);
    // }

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
    auto denominator = x_max - x_min;

    // TODO: how does the DS GPU deal with division-by-zero?
    if (denominator != 0) {
      factor_lerp = ((x - x_min) << precision) / denominator;
    } else {
      factor_lerp = 0;
    }
  }

  void CalculatePerpFactor(u16 w0_norm, u16 w1_norm, s32 x, s32 x_min, s32 x_max) {
    u16 w0_num;
    u16 w0_denom;
    u16 w1_denom;

    if ((w0_norm & 1) && !(w1_norm & 1)) {
      w0_num = w0_norm - 1;
      w0_denom = w0_norm + 1;
      w1_denom = w1_norm;
    } else {
      w0_num = w0_norm & 0xFFFE;
      w0_denom = w0_num;
      w1_denom = w1_norm & 0xFFFE;
    }

    auto t0 = x - x_min;
    auto t1 = x_max - x;
    auto denominator = t1 * w1_denom + t0 * w0_denom;

    // TODO: how does the DS GPU deal with division-by-zero?
    if (denominator != 0) {
      factor_perp = ((t0 << precision) * w0_num) / denominator;
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
  s32 w_norm[2];
  u32 depth[2];
  Vector2<Fixed12x4> uv[2];
  Color4 color[2];
};

void GPU::Render() {
  Point points[10];
  Span span;

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

    for (uint i = 0; i < 256 * 192; i++) {
      draw_buffer[i] = color;
    }

    for (uint i = 0; i < 256 * 192; i++) {
      depth_buffer[i] = depth;
    }
  }

  auto buffer_id = gx_buffer_id ^ 1;
  auto& vert_ram = vertex[buffer_id];
  auto& poly_ram = polygon[buffer_id];
  auto poly_count = poly_ram.count;

  for (int i = 0; i < poly_count; i++) {
    Polygon const& poly = poly_ram.data[i];

    int start;
    int end;
    s32 y_min = 256;
    s32 y_max = 0;

    auto vert_count = poly.count;

    for (int j = 0; j < vert_count; j++) {
      auto& point = points[j];
      auto const& vert = vert_ram.data[poly.indices[j]];

      // TODO: use the provided viewport configuration.
      // TODO: support w-Buffering mode.
      point.x = ( vert.position.x() / vert.position.w() * Fixed20x12::from_int(128)).integer() + 128;
      point.y = (-vert.position.y() / vert.position.w() * Fixed20x12::from_int( 96)).integer() +  96;
      point.depth = (((s64(vert.position.z().raw()) << 14) / vert.position.w().raw()) + 0x3FFF) << 9;
      point.w_norm = vert.position.w().raw();
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

    // w-normalization.
    // TODO: move this to the correct place in the pipeline
    // Also make sure that this is actually correct.
    {
      int min_leading = 32;

      for (int j = 0; j < vert_count; j++) {
        auto const& point = points[j];

        if (point.w_norm < 0) {
          min_leading = std::min(min_leading, __builtin_clz(~point.w_norm));
        } else {
          min_leading = std::min(min_leading, __builtin_clz( point.w_norm));
        }
      }

      if (min_leading < 16) {
        int shift = 16 - min_leading;

        if ((shift & 3) != 0) {
          shift += 4 - (shift & 3);
        }

        for (int j = 0; j < vert_count; j++) {
          points[j].w_norm >>= shift;
        }
      } else if (min_leading > 16) {
        int shift = (min_leading - 16) & ~3 ;
      
        for (int j = 0; j < vert_count; j++) {
          points[j].w_norm <<= shift;
        }
      }

      // LOG_ERROR(
      //   "GPU: w0 =0x{:08X} w1 =0x{:08X} w2 =0x{:08X}", 
      //   points[0].vertex->position.w().raw(), 
      //   points[1].vertex->position.w().raw(),
      //   points[2].vertex->position.w().raw()
      // );
      // LOG_ERROR("GPU: w0n=0x{:08X} w1n=0x{:08X} w2n=0x{:08X}", points[0].w_norm, points[1].w_norm, points[2].w_norm);
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
    bool wireframe = alpha == 0;
    bool force_draw_edges_a = alpha != 63 || disp3dcnt.enable_antialias || disp3dcnt.enable_edge_marking;

    if (wireframe) {
      alpha = 63;
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

      if (x1[0] > x0[1]) {
        l = 1;
        r = 0;
      } else {
        l = 0;
        r = 1;
      }

      for (int j = 0; j < 2; j++) {
        auto const& p0 = points[s[j]];
        auto const& p1 = points[e[j]];

        s32 w0 = points[s[j]].vertex->position.w().raw();
        s32 w1 = points[s[j]].vertex->position.w().raw();
        s32 w0_norm = points[s[j]].w_norm;
        s32 w1_norm = points[e[j]].w_norm;

        if (edge[j].IsXMajor()) {
          int x = (j == l) ? x0[l] : x1[r];

          // TODO: actually use the precision that we have...
          edge_interpolator.Setup(w0, w1, w0_norm, w1_norm, x >> 18, p0.x, p1.x);
        } else {
          edge_interpolator.Setup(w0, w1, w0_norm, w1_norm, y, p0.y, p1.y);
        }

        // TODO: is it accurate to reduce the precision like that?
        span.x0[j] = x0[j] >> 18;
        span.x1[j] = x1[j] >> 18;
        span.w[j] = edge_interpolator.Interpolate(p0.vertex->position.w().raw(), p1.vertex->position.w().raw());
        span.w_norm[j] = edge_interpolator.Interpolate(p0.w_norm, p1.w_norm);
        edge_interpolator.Interpolate(p0.vertex->uv, p1.vertex->uv, span.uv[j]);
        edge_interpolator.Interpolate(p0.vertex->color, p1.vertex->color, span.color[j]);
      }

      LOG_INFO("GPU: span: {} ({} {}) {} @ y={}", span.x0[l], span.x1[l], span.x0[r], span.x1[r], y);

      // TODO: preferrably handle this outside the rasterization loop
      // by limiting the minimum and maximum y-values.
      if (y >= 0 && y <= 191) {
        const auto min_x = span.x0[l];
        const auto max_x = span.x1[r];

        const auto render_span = [&](s32 x_min, s32 x_max) {
          auto uv = Vector2<Fixed12x4>{};
          auto color = Color4{};

          for (s32 x = x_min; x <= x_max; x++) {
            // TODO: clamp x_min and x_max instead
            if (x < 0 || x > 255) continue;

            // TODO: cache calculations that do not depend on x.
            span_interpolator.Setup(span.w[l], span.w[r], span.w_norm[l], span.w_norm[r], x, min_x, max_x);
            span_interpolator.Interpolate(span.uv[l], span.uv[r], uv);
            span_interpolator.Interpolate(span.color[l], span.color[r], color);

            color.a() = alpha; 

            if (disp3dcnt.enable_textures) {
              auto texel = SampleTexture(poly.texture_params, uv);
              // TODO: perform alpha test
              if (texel.a() == 0) {
                continue;
              }
              color *= texel;
            }

            draw_buffer[y * 256 + x] = color;
          }
        };

        if (edge[r].XSlope() == 0) {
          // TODO: the horizontal attribute interpolator's rightmost X coordinate is incremented by one
          //span.x0[r]--;
          //span.x1[r]--;
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

        LOG_INFO("GPU:  CW edge advance @ y={} (next y-target: {})", y, points[e[0]].y);
      }

      // update counter clock-wise edge
      if (points[e[1]].y <= next_y && e[1] != end) {
        s[1] = e[1];
        if (--e[1] == -1)
          e[1] = vert_count - 1;
        edge[1] = Edge{points[s[1]], points[e[1]]};

        LOG_INFO("GPU: CCW edge advance @ y={} (next y-target: {})", y, points[e[1]].y);
      }
    }
  }
}

} // namespace Duality::Core
