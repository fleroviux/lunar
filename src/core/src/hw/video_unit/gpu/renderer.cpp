/*
 * Copyright (C) 2021 fleroviux
 */

#include "gpu.hpp"

namespace Duality::Core {

void GPU::Render() {
  // TODO: use data from rear image MMIO registers.
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

    auto lerp = [](s32 a, s32 b, s32 t, s32 t_max) {
      if (t_max == 0)
        return a;
      return (a * (t_max - t) + b * t) / t_max;
    };

    auto textureSample = [this](TextureParams const& params, s16 u, s16 v) -> u16 {
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

      // TODO: implement 4x4-compressed format.
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
    };

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

      // TODO: it is unclear how exactly the first vertex is selected,
      // if multiple vertices have the same lowest y-Coordinate.
      if (point.y < y_min) {
        y_min = point.y;
        start = j;
      }
      if (point.y > y_max) {
        y_max = point.y;
      }
    }

    // FIXME
    if (skip) {
      continue;
    }

    int s0 = start;
    int e0 = start == (poly.count - 1) ? 0 : (start + 1);
    int s1 = start;
    int e1 = start == 0 ? (poly.count - 1) : (start - 1);

    for (s32 y = y_min; y <= y_max; y++) {
      if (points[e0].y <= y) {
        s0 = e0;
        if (++e0 == poly.count)
          e0 = 0;
      }

      if (points[e1].y <= y) {
        s1 = e1;
        if (--e1 == -1)
          e1 = poly.count - 1;
      }

      // TODO: do not recalculate t and t_max every time we interpolate a vertex attribute.
      s32 x0 = lerp(points[s0].x, points[e0].x, y - points[s0].y, points[e0].y - points[s0].y);
      s32 x1 = lerp(points[s1].x, points[e1].x, y - points[s1].y, points[e1].y - points[s1].y);

      // TODO: do not interpolate attributes which are not used.

      s32 color0[3];
      s32 color1[3];
      for (int j = 0; j < 3; j++) {
        color0[j] = lerp(points[s0].vertex->color[j], points[e0].vertex->color[j], y - points[s0].y, points[e0].y - points[s0].y);
        color1[j] = lerp(points[s1].vertex->color[j], points[e1].vertex->color[j], y - points[s1].y, points[e1].y - points[s1].y);
      }

      s16 uv0[2];
      s16 uv1[2];
      for (int j = 0; j < 2; j++) {
        uv0[j] = lerp(points[s0].vertex->uv[j].raw(), points[e0].vertex->uv[j].raw(), y - points[s0].y, points[e0].y - points[s0].y);
        uv1[j] = lerp(points[s1].vertex->uv[j].raw(), points[e1].vertex->uv[j].raw(), y - points[s1].y, points[e1].y - points[s1].y);
      }

      s32 depth0 = lerp(points[s0].depth, points[e0].depth, y - points[s0].y, points[e0].y - points[s0].y);
      s32 depth1 = lerp(points[s1].depth, points[e1].depth, y - points[s1].y, points[e1].y - points[s1].y);

      // TODO: find a faster way to swap the edges,
      // maybe use an index into an array?
      if (x0 > x1) {
        std::swap(x0, x1);
        std::swap(color0, color1);
        std::swap(uv0, uv1);
        std::swap(depth0, depth1);
      }

      // TODO: boundary checks will be redundant if clipping and viewport tranform work properly.
      if (y >= 0 && y <= 191) {
        s32 color[3];
        s16 uv[2];
        s32 depth;

        for (s32 x = x0; x <= x1; x++) {
          if (x >= 0 && x <= 255) {
            depth = lerp(depth0, depth1, x - x0, x1 - x0);

            // FIXME: implement "equal" depth test mode.
            if (depth >= depthbuffer[y * 256 + x]) {
              continue;
            }

            for (int j = 0; j < 3; j++) {
              color[j] = lerp(color0[j], color1[j], x - x0, x1 - x0);
            }

            for (int j = 0; j < 2; j++) {
              uv[j] = lerp(uv0[j], uv1[j], x - x0, x1 - x0);
            }

            // FIXME: multiply vertex color with texel.
            //output[y * 256 + x] = (color[0] >> 1) |
            //                     ((color[1] >> 1) <<  5) |
            //                     ((color[2] >> 1) << 10);
            auto texel = textureSample(poly.texture_params, uv[0], uv[1]);
            if (texel != 0x8000) {
              output[y * 256 + x] = texel;
              depthbuffer[y * 256 + x] = depth;
            }
          }
        }
      }
    }
  }
}

} // namespace Duality::Core
