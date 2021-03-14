/*
 * Copyright (C) 2021 fleroviux
 */

#include "gpu.hpp"

namespace Duality::Core {

auto GPU::SampleTexture(TextureParams const& params, s16 u, s16 v) -> u16 {
  const int size[2] {
    8 << params.size[0],
    8 << params.size[1]
  };

  int coord[2] { u >> 4, v >> 4 };

  for (int i = 0; i < 2; i++) {
    if (coord[i] < 0 || coord[i] >= size[i]) {
      int mask = size[i] - 1;
      if (params.repeat[i]) {
        coord[i] &= mask;
        if (params.flip[i]) {
          coord[i] ^= mask;
        }
      } else {
        coord[i] = std::clamp(coord[i], 0, mask);
      }
    }
  }

  auto offset = coord[1] * size[0] + coord[0];

  switch (params.format) {
    case TextureParams::Format::None: {
      return 0x7FFF;
    }
    case TextureParams::Format::A3I5: {
      u8  value = vram_texture.Read<u8>(params.address + offset);
      int index = value & 0x1F;
      int alpha = value >> 5;
      alpha = (alpha << 2) + (alpha >> 1);
      // TODO: this is incorrect, but we don't support semi-transparency right now.
      // I'm also not sure if this format uses the "Color 0 transparent" flag.
      if (alpha == 0 || (params.color0_transparent && index == 0)) {
        return 0x8000;
      }
      return vram_palette.Read<u16>((params.palette_base << 4) + index * sizeof(u16)) & 0x7FFF;
    }
    case TextureParams::Format::Palette2BPP: {
      auto index = (vram_texture.Read<u8>(params.address + (offset >> 2)) >> (2 * (offset & 3))) & 3;
      if (params.color0_transparent && index == 0) {
        return 0x8000;
      }
      return vram_palette.Read<u16>((params.palette_base << 3) + index * sizeof(u16)) & 0x7FFF;
    }
    case TextureParams::Format::Palette4BPP: {
      auto index = (vram_texture.Read<u8>(params.address + (offset >> 1)) >> (4 * (offset & 1))) & 15;
      if (params.color0_transparent && index == 0) {
        return 0x8000;
      }
      return vram_palette.Read<u16>((params.palette_base << 4) + index * sizeof(u16)) & 0x7FFF;
    }
    case TextureParams::Format::Palette8BPP: {
      auto index = vram_texture.Read<u8>(params.address + offset);
      if (params.color0_transparent && index == 0) {
        return 0x8000;
      }
      return vram_palette.Read<u16>((params.palette_base << 4) + index * sizeof(u16)) & 0x7FFF;
    }
    case TextureParams::Format::A5I3: {
      u8  value = vram_texture.Read<u8>(params.address + offset);
      int index = value & 7;
      int alpha = value >> 3;
      // TODO: this is incorrect, but we don't support semi-transparency right now.
      // I'm also not sure if this format uses the "Color 0 transparent" flag.
      if (alpha == 0 || (params.color0_transparent && index == 0)) {
        return 0x8000;
      }
      return vram_palette.Read<u16>((params.palette_base << 4) + index * sizeof(u16)) & 0x7FFF;
    }
    case TextureParams::Format::Direct: {
      auto color = vram_texture.Read<u16>(params.address + offset * sizeof(u16));
      if (color & 0x8000) {
        return 0x8000;
      }
      return color;
    }
  };

  return u16(0x999);
}

void GPU::Render() {
  for (uint i = 0; i < 256 * 192; i++) {
    output[i] = 0x8000;
    depthbuffer[i] = 0x7FFFFFFF;
  }

  for (int i = 0; i < polygon[gx_buffer_id ^ 1].count; i++) {
    Polygon const& poly = polygon[gx_buffer_id ^ 1].data[i];

    struct Point {
      s32 x;
      s32 y;
      s32 depth;
      Vertex const* vertex;
    } points[poly.count];

    int start = 0;
    s32 y_min = 256;
    s32 y_max = 0;

    bool skip = false;

    for (int j = 0; j < poly.count; j++) {
      auto const& vert = vertex[gx_buffer_id ^ 1].data[poly.indices[j]];
      auto& point = points[j];

      // FIXME
      if (vert.position[3] == 0) {
        skip = true;
        break;
      }

      // TODO: use the provided viewport configuration.
      point.x = ( vert.position.x() / vert.position.w() * Fixed20x12::from_int(128)).integer() + 128;
      point.y = (-vert.position.y() / vert.position.w() * Fixed20x12::from_int( 96)).integer() +  96;
      point.depth = (vert.position.z() / vert.position.w()).raw();
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

    // FIXME
    if (skip) {
      continue;
    }

    int s[2];
    int e[2];

    // first edge (CW)
    s[0] = start;
    e[0] = start == (poly.count - 1) ? 0 : (start + 1);

    // second edge (CCW)
    s[1] = start;
    e[1] = start == 0 ? (poly.count - 1) : (start - 1);

    for (s32 y = y_min; y <= y_max; y++) {
      if (points[e[0]].y <= y) {
        s[0] = e[0];
        if (++e[0] == poly.count)
          e[0] = 0;
      }

      if (points[e[1]].y <= y) {
        s[1] = e[1];
        if (--e[1] == -1)
          e[1] = poly.count - 1;
      }

      struct Span {
        s32 x[2];
        s32 w[2];
        s32 depth[2];
        s16 uv[2][2];
        Color4 color[2];
      } span;

      int a = 0;
      int b = 1;

      auto lerp = [](s32 a, s32 b, s32 t, s32 t_max, s32 w_a = 1 << 12, s32 w_b = 1 << 12) {
        // CHECKME
        if (w_a == 0 || w_b == 0)
          return a;
        auto x = (s64(t_max - t) << 24) / w_a;
        auto y = (s64(t) << 24) / w_b;
        auto max = x + y;
        if (max == 0)
          return a;
        return s32((a * x + b * y) / max);
      };

      for (int j = 0; j < 2; j++) {
        auto t = y - points[s[j]].y;
        auto t_max = points[e[j]].y - points[s[j]].y;
        auto w0 = points[s[j]].vertex->position.w().raw();
        auto w1 = points[e[j]].vertex->position.w().raw();

        span.x[j] = lerp(points[s[j]].x, points[e[j]].x, t, t_max);
        // TODO: this can be simplified because w0 and w1 both cancel out in part of the equation.
        span.w[j] = lerp(w0, w1, t, t_max, w0, w1);
        span.depth[j] = lerp(points[s[j]].depth, points[e[j]].depth, t, t_max, w0, w1);

        for (int k = 0; k < 2; k++) {
          span.uv[j][k] = lerp(
            points[s[j]].vertex->uv[k].raw(),
            points[e[j]].vertex->uv[k].raw(), t, t_max, w0, w1);
        }

        for (int k = 0; k < 3; k++) {
          span.color[j][k] = detail::ColorComponent{lerp(
            points[s[j]].vertex->color[k].raw(),
            points[e[j]].vertex->color[k].raw(), t, t_max, w0, w1)};
        }
      }

      if (span.x[0] > span.x[1]) {
        a ^= 1;
        b ^= 1;
      }

      // TODO: why are these guards still necessary? Is the clipper not doing its job?
      if (y >= 0 && y <= 191) {
        for (s32 x = span.x[a]; x <= span.x[b]; x++) {
          if (x >= 0 && x <= 255) {
            auto t = x - span.x[a];
            auto t_max = span.x[b] - span.x[a];

            s16 uv[2];
            s32 depth;
            Color4 vertex_color;

            for (int j = 0; j < 2; j++) {
              uv[j] = lerp(span.uv[a][j], span.uv[b][j], t, t_max, span.w[a], span.w[b]);
            }

            depth = lerp(span.depth[a], span.depth[b], t, t_max, span.w[a], span.w[b]);

            for (int j = 0; j < 3; j++) {
              vertex_color[j] = detail::ColorComponent{lerp(
                span.color[a][j].raw(),
                span.color[b][j].raw(), t, t_max, span.w[a], span.w[b])};
            }

            // TODO: implement "equal" depth test mode.
            if (depth >= depthbuffer[y * 256 + x]) {
              continue;
            }

            // TODO: do not sample textures if they are disabled.
            auto texel = SampleTexture(poly.texture_params, uv[0], uv[1]);
            // TODO: perform alpha test
            // TODO: respect "depth-value for translucent pixels" setting from "polygon_attr" command.
            if (texel != 0x8000) {
              // TODO: SampleTexture should return a Color4 object instead.
              auto texel_color = Color4::from_rgb555(texel);
              // TODO: final GPU output should be 18-bit (RGB666), I think?
              output[y * 256 + x] = (texel_color * vertex_color).to_rgb555();
              depthbuffer[y * 256 + x] = depth;
            }
          }
        }
      }
    }
  }
}

} // namespace Duality::Core
