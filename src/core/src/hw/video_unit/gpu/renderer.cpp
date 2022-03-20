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

  bool& IsXMajor() { return x_major; }

  void Interpolate(s32 y, fixed14x18& x0, fixed14x18& x1) {
    // TODO: always calculating x1 is redundant.
    // I think this should only be necessary for wireframe rendering?
    if (x_major) {
      // TODO: make sure that the math is correct (especially for negative slopes)
      if (x_slope >= 0) {
        x0 = (p0->x << 18) + x_slope * (y - p0->y) + (1 << 17);
        x1 = (x0 & ~0x1FF) + x_slope - (1 << 18);
      } else {
        x1 = (p0->x << 18) + x_slope * (y - p0->y) + (1 << 17) - (1 << 18);
        x0 = (x1 & ~0x1FF) + x_slope;
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

    // TODO: how does hardware handle this edge-case? Does it ever happen?
    if (y_diff == 0) y_diff = 1;

    fixed14x18 y_reciprocal = (1 << 18) / y_diff;

    x_slope = x_diff * y_reciprocal;
    x_major = std::abs(x_diff) > std::abs(y_diff);
  }

  Point const* p0;
  Point const* p1;
  fixed14x18 x_slope;
  bool x_major;
};

template<int precision>
struct Interpolator {
  void Setup(s32 w0, s32 w1, u16 w0_norm, u16 w1_norm, s32 x, s32 x_min, s32 x_max) {
    CalculateLerpFactor(x, x_min, x_max);
    CalculatePerpFactor(w0_norm, w1_norm, x, x_min, x_max);

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
    // NOTE: this might (will) result in division-by-zero.
    factor_lerp = ((x - x_min) << precision) / (x_max - x_min); 
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

    // NOTE: this might (will) result in division-by-zero.
    factor_perp = ((t0 << precision) * w0_num) / (t1 * w1_denom + t0 * w0_denom);
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

    int start = 0;
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

    for (s32 y = y_min; y <= y_max; y++) {
      // update clock-wise edge
      if (points[e[0]].y <= y) {
        s[0] = e[0];
        if (++e[0] == vert_count)
          e[0] = 0;
        edge[0] = Edge{points[s[0]], points[e[0]]};
      }

      // update counter clock-wise edge
      if (points[e[1]].y <= y) {
        s[1] = e[1];
        if (--e[1] == -1)
          e[1] = vert_count - 1;
        edge[1] = Edge{points[s[1]], points[e[1]]};
      }

      // interpolate both edges vertically
      for (int j = 0; j < 2; j++) {
        auto const& p0 = points[s[j]];
        auto const& p1 = points[e[j]];

        Edge::fixed14x18 x0;
        Edge::fixed14x18 x1;

        edge[j].Interpolate(y, x0, x1);

        s32 w0 = points[s[j]].vertex->position.w().raw();
        s32 w1 = points[s[j]].vertex->position.w().raw();
        s32 w0_norm = points[s[j]].w_norm;
        s32 w1_norm = points[e[j]].w_norm;
          
        Color4 color;
        
        if (edge[j].IsXMajor()) {
          // TODO: actually use the precision that we have...
          edge_interpolator.Setup(w0, w1, w0_norm, w1_norm, x0 >> 18, p0.x, p1.x);
        } else {
          edge_interpolator.Setup(w0, w1, w0_norm, w1_norm, y, p0.y, p1.y);
        }

        // TODO: is it accurate to reduce the precision like that?
        span.x0[j] = x0 >> 18;
        span.x1[j] = x1 >> 18;
        span.w[j] = edge_interpolator.Interpolate(p0.vertex->position.w().raw(), p1.vertex->position.w().raw());
        span.w_norm[j] = edge_interpolator.Interpolate(p0.w_norm, p1.w_norm);
        edge_interpolator.Interpolate(p0.vertex->uv, p1.vertex->uv, span.uv[j]);
        edge_interpolator.Interpolate(p0.vertex->color, p1.vertex->color, span.color[j]);
      }

      // Left and right edge indices
      int l;
      int r;

      if (span.x0[0] > span.x1[1]) {
        l = 1;
        r = 0;
      } else {
        l = 0;
        r = 1;
      }

      // TODO: preferrably handle this outside the rasterization loop
      // by limiting the minimum and maximum y-values.
      if (y >= 0 && y <= 191) {
        // TODO: use specialized render method for wireframe drawing.
        auto alpha = (poly.params.alpha << 1) | (poly.params.alpha >> 4);
        bool wireframe = alpha == 0;
        auto uv = Vector2<Fixed12x4>{};
        auto color = Color4{};

        auto x_max = span.x1[r];

        if (!wireframe) {
          x_max--;
        } else {
          alpha = 63;
        }

        for (s32 x = span.x0[l]; x <= x_max; x++) {
          if (wireframe && x > span.x1[l] && x < span.x0[r]) {
            continue;
          }

          if (x >= 0 && x <= 255) {
            // TODO: cache calculations that do not depend on x.
            span_interpolator.Setup(span.w[l], span.w[r], span.w_norm[l], span.w_norm[r], x, span.x0[l], span.x1[r]);
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
        }
      }
    }

    /*for (s32 y = y_min; y <= y_max; y++) {
      int a = 0; // left edge index
      int b = 1; // right edge index

      if (points[e[0]].y <= y) {
        s[0] = e[0];
        if (++e[0] == vert_count)
          e[0] = 0;
      }

      if (points[e[1]].y <= y) {
        s[1] = e[1];
        if (--e[1] == -1)
          e[1] = vert_count - 1;
      }

      auto lerp = [](s32 a, s32 b, s32 t, s32 t_max, s32 w_a = 1 << 12, s32 w_b = 1 << 12) {
        // // If both w-Coordinates are same then division-by-zero should cancel out.
        // if (w_a == w_b) {
        //   w_a = 1;
        //   w_b = 1;
        // }

        // // If only one w-Coordinate is zero, then the other will be biased to zero.
        // if (w_a == 0) return a;
        // if (w_b == 0) return b;

        if (w_a != 0 && w_b != 0) {
          auto x = (s64(t_max - t) << 24) / w_a;
          auto y = (s64(t) << 24) / w_b;
          auto max = x + y;

          if (max != 0) {
            return s32((a * x + b * y) / max);
          }
        }

        return a;
      };

      for (int j = 0; j < 2; j++) {
        auto t = y - points[s[j]].y;
        auto t_max = points[e[j]].y - points[s[j]].y;
        auto w0 = points[s[j]].vertex->position.w().raw();
        auto w1 = points[e[j]].vertex->position.w().raw();

        // TODO: interpolation of w0 and w1 can be optimized.
        span.x[j] = lerp(points[s[j]].x, points[e[j]].x, t, t_max);
        span.w[j] = lerp(w0, w1, t, t_max, w0, w1);
        span.depth[j] = lerp(points[s[j]].depth, points[e[j]].depth, t, t_max);

        for (int k = 0; k < 2; k++) {
          span.uv[j][k] = Fixed12x4{s16(lerp(
            points[s[j]].vertex->uv[k].raw(),
            points[e[j]].vertex->uv[k].raw(), t, t_max, w0, w1))};
        }

        for (int k = 0; k < 3; k++) {
          span.color[j][k] = Fixed6{s8(lerp(
            points[s[j]].vertex->color[k].raw(),
            points[e[j]].vertex->color[k].raw(), t, t_max, w0, w1))};
        }
      }

      if (span.x[0] > span.x[1]) {
        a ^= 1;
        b ^= 1;
      }

      if (y >= 0 && y <= 191) {
        for (s32 x = span.x[a]; x <= span.x[b]; x++) {
          if (x >= 0 && x <= 255) {
            auto t = x - span.x[a];
            auto t_max = span.x[b] - span.x[a];
            auto index = y * 256 + x;

            auto depth_old = depth_buffer[index];
            auto depth_new = lerp(span.depth[a], span.depth[b], t, t_max);

            if (poly.params.depth_test == PolygonParams::DepthTest::Less) {
              if (depth_new >= depth_old)
                continue;
            } else {
              if (std::abs(s32(depth_new) - s32(depth_old)) > 0x200)
                continue;
            }

            Vector2<Fixed12x4> uv;
            Color4 vertex_color;

            for (int j = 0; j < 2; j++) {
              uv[j] = Fixed12x4{s16(lerp(span.uv[a][j].raw(), span.uv[b][j].raw(), t, t_max, span.w[a], span.w[b]))};
            }

            for (int j = 0; j < 3; j++) {
              vertex_color[j] = Fixed6{s8(lerp(
                span.color[a][j].raw(),
                span.color[b][j].raw(), t, t_max, span.w[a], span.w[b]))};
            }

            bool do_blend = disp3dcnt.enable_alpha_blend && draw_buffer[index].a() != 0;

            auto color = vertex_color;

            // TODO: alpha=0 means wireframe mode. handle that.
            color.a() = (poly.params.alpha << 1) | (poly.params.alpha >> 4); 

            if (disp3dcnt.enable_textures) {
              auto texel = SampleTexture(poly.texture_params, uv);
              // TODO: perform alpha test
              if (texel.a() == 0) {
                continue;
              }
              color *= texel;
            }

            if (do_blend) {
              auto a0 = color.a();
              auto a1 = Fixed6{63} - a0;
              for (uint j = 0; j < 3; j++)
                color[j] = color[j] * a0 + draw_buffer[index][j] * a1;
              color.a() = std::max(color.a(), draw_buffer[index].a());
            }

            draw_buffer[index] = color;
            // TODO: figure out what the actual logic is supposed to be.
            //if (color.a() == 63 || poly.params.enable_translucent_depth_write) {
              depth_buffer[index] = depth_new;
            //}
          }
        }
      }
    }*/
  }
}

} // namespace Duality::Core
