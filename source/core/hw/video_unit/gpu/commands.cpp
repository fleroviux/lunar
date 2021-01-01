/*
 * Copyright (C) 2020 fleroviux
 */

#include <algorithm>

#include "gpu.hpp"

namespace fauxDS::core {

void GPU::CMD_SetMatrixMode() {
  matrix_mode = static_cast<MatrixMode>(Dequeue().argument & 3);
}

void GPU::CMD_PushMatrix() {
  Dequeue();
  
  switch (matrix_mode) {
    case MatrixMode::Projection:
      projection.Push();
      break;
    case MatrixMode::Modelview:
    case MatrixMode::Simultaneous:
      modelview.Push();
      direction.Push();
      break;
    case MatrixMode::Texture:
      texture.Push();
      break;
  }
}

void GPU::CMD_PopMatrix() {
  auto offset = Dequeue().argument & 31;
  
  switch (matrix_mode) {
    case MatrixMode::Projection:
      projection.Pop(offset);
      break;
    case MatrixMode::Modelview:
    case MatrixMode::Simultaneous:
      modelview.Pop(offset);
      direction.Pop(offset);
      break;
    case MatrixMode::Texture:
      texture.Pop(offset);
      break;
  }
}

void GPU::CMD_StoreMatrix() {
  auto address = Dequeue().argument & 31;
  
  switch (matrix_mode) {
    case MatrixMode::Projection:
      projection.Store(address);
      break;
    case MatrixMode::Modelview:
    case MatrixMode::Simultaneous:
      modelview.Store(address);
      direction.Store(address);
      break;
    case MatrixMode::Texture:
      texture.Store(address);
      break;
  }
}

void GPU::CMD_LoadIdentity() {
  Dequeue();
  
  switch (matrix_mode) {
    case MatrixMode::Projection:
      projection.current.LoadIdentity();
      break;
    case MatrixMode::Modelview:
      modelview.current.LoadIdentity();
      break;
    case MatrixMode::Simultaneous:
      modelview.current.LoadIdentity();
      direction.current.LoadIdentity();
      break;
    case MatrixMode::Texture:
      texture.current.LoadIdentity();
      break;
  }
}

void GPU::CMD_LoadMatrix4x4() {
  auto mat = DequeueMatrix4x4();
  
  switch (matrix_mode) {
    case MatrixMode::Projection:
      projection.current = mat;
      break;
    case MatrixMode::Modelview:
      modelview.current = mat;
      break;
    case MatrixMode::Simultaneous:
      modelview.current = mat;
      direction.current = mat;
      break;
    case MatrixMode::Texture:
      texture.current = mat;
      break;
  }
}

void GPU::CMD_LoadMatrix4x3() {
  auto mat = DequeueMatrix4x3();
  
  switch (matrix_mode) {
    case MatrixMode::Projection:
      projection.current = mat;
      break;
    case MatrixMode::Modelview:
      modelview.current = mat;
      break;
    case MatrixMode::Simultaneous:
      modelview.current = mat;
      direction.current = mat;
      break;
    case MatrixMode::Texture:
      texture.current = mat;
      break;
  }
}

void GPU::CMD_MatrixMultiply4x4() {
  auto mat = DequeueMatrix4x4();
  
  switch (matrix_mode) {
    case MatrixMode::Projection:
      projection.current *= mat;
      break;
    case MatrixMode::Modelview:
      modelview.current *= mat;
      break;
    case MatrixMode::Simultaneous:
      modelview.current *= mat;
      direction.current *= mat;
      break;
    case MatrixMode::Texture:
      texture.current *= mat;
      break;
  }
}

void GPU::CMD_MatrixMultiply4x3() {
  auto mat = DequeueMatrix4x3();
  
  switch (matrix_mode) {
    case MatrixMode::Projection:
      projection.current *= mat;
      break;
    case MatrixMode::Modelview:
      modelview.current *= mat;
      break;
    case MatrixMode::Simultaneous:
      modelview.current *= mat;
      direction.current *= mat;
      break;
    case MatrixMode::Texture:
      texture.current *= mat;
      break;
  }
}

void GPU::CMD_MatrixMultiply3x3() {
  auto mat = DequeueMatrix3x3();
  
  switch (matrix_mode) {
    case MatrixMode::Projection:
      projection.current *= mat;
      break;
    case MatrixMode::Modelview:
      modelview.current *= mat;
      break;
    case MatrixMode::Simultaneous:
      modelview.current *= mat;
      direction.current *= mat;
      break;
    case MatrixMode::Texture:
      texture.current *= mat;
      break;
  }
}

void GPU::CMD_MatrixScale() {
  // TODO: this implementation is unoptimized.
  // A matrix multiplication is complete overkill for scaling.
  
  Matrix4x4 mat;
  for (int i = 0; i < 3; i++) {
    mat[i][i] = Dequeue().argument;
  }
  mat[3][3] = 0x1000;
  
  switch (matrix_mode) {
    case MatrixMode::Projection:
      projection.current *= mat;
      break;
    case MatrixMode::Modelview:
    case MatrixMode::Simultaneous:
      modelview.current *= mat;
      break;
    case MatrixMode::Texture:
      texture.current *= mat;
      break;
  }
}

void GPU::CMD_MatrixTranslate() {
  Matrix4x4 mat;
  mat.LoadIdentity();
  for (int i = 0; i < 3; i++) {
    mat[3][i] = Dequeue().argument;
  }
  
  switch (matrix_mode) {
    case MatrixMode::Projection:
      projection.current *= mat;
      break;
    case MatrixMode::Modelview:
    case MatrixMode::Simultaneous:
      modelview.current *= mat;
      break;
    case MatrixMode::Texture:
      texture.current *= mat;
      break;
  }
}

void GPU::CMD_SetColor() {
  // TODO: fix this absolutely atrocious code...

  auto arg = Dequeue().argument;

  auto r = (arg >>  0) & 31;
  auto g = (arg >>  5) & 31;
  auto b = (arg >> 10) & 31;

  vertex_color[0] = r * 2 + (r + 31) / 32;
  vertex_color[1] = g * 2 + (g + 31) / 32;
  vertex_color[2] = b * 2 + (b + 31) / 32;
}

void GPU::CMD_SetNormal() {
}

void GPU::CMD_SetUV() {
  auto arg = Dequeue().argument;
  vertex_uv[0] = s16(arg & 0xFFFF);
  vertex_uv[1] = s16(arg >> 16);
}

void GPU::CMD_SubmitVertex_16() {
  auto arg0 = Dequeue().argument;
  auto arg1 = Dequeue().argument;
  AddVertex({
    s16(arg0 & 0xFFFF),
    s16(arg0 >> 16),
    s16(arg1 & 0xFFFF),
    0x1000
  });
}

void GPU::CMD_SubmitVertex_10() {
  auto arg = Dequeue().argument;
  AddVertex({
    s16((arg >>  0) << 6),
    s16((arg >> 10) << 6),
    s16((arg >> 20) << 6),
    0x1000
  });
}

void GPU::CMD_SubmitVertex_XY() {
  auto arg = Dequeue().argument;
  AddVertex({
    s16(arg & 0xFFFF),
    s16(arg >> 16),
    position_old[2],
    0x1000
  });
}

void GPU::CMD_SubmitVertex_XZ() {
  auto arg = Dequeue().argument;
  AddVertex({
    s16(arg & 0xFFFF),
    position_old[1],
    s16(arg >> 16),
    0x1000
  });
}

void GPU::CMD_SubmitVertex_YZ() {
  auto arg = Dequeue().argument;
  AddVertex({
    position_old[0],
    s16(arg & 0xFFFF),
    s16(arg >> 16),
    0x1000
  });
}

void GPU::CMD_SubmitVertex_Offset() {
  auto arg = Dequeue().argument;
  AddVertex({
    position_old[0] + (s16((arg >>  0) << 6) >> 6),
    position_old[1] + (s16((arg >> 10) << 6) >> 6),
    position_old[2] + (s16((arg >> 20) << 6) >> 6),
    0x1000
  });
}

void GPU::CMD_SetTextureParameters() {
  auto arg = Dequeue().argument;

  texture_params.address = (arg & 0xFFFF) << 3;
  texture_params.repeat_u = arg & (1 << 16);
  texture_params.repeat_v = arg & (1 << 17);
  texture_params.flip_u = arg & (1 << 18);
  texture_params.flip_v = arg & (1 << 19);
  texture_params.size_u = 8 << ((arg >> 20) & 7);
  texture_params.size_v = 8 << ((arg >> 23) & 7);
  texture_params.format = static_cast<TextureParams::Format>((arg >> 26) & 7);
  texture_params.color0_transparent = arg & (1 << 29);
  texture_params.transform = static_cast<TextureParams::Transform>(arg >> 30);

  LOG_DEBUG("GPU: texture @ 0x{0:08X} width={1} height={2} format={3} transform={4}",
    texture_params.address, texture_params.size_u, texture_params.size_v, texture_params.format, texture_params.transform);
}

void GPU::CMD_SetPaletteBase() {
  texture_params.palette_base = Dequeue().argument & 0x1FFF;
}

void GPU::CMD_BeginVertexList() {
  auto arg = Dequeue().argument;
  in_vertex_list = true;
  vertex_counter = 0;
  is_quad = arg & 1;
  is_strip = arg & 2;

  // TODO: this is likely inaccurate.
  // I don't know when exactly (and if) vertex attributes are reset.
  for (int i = 0; i < 3; i++) {
    vertex_color[i] = 63;
  }
  for (int i = 0; i < 2; i++) {
    vertex_uv[i] = 0;
  }
}

void GPU::CMD_EndVertexList() {
  Dequeue();
  // TODO: allegedly this command is a no-operation on the DS...
  in_vertex_list = false;
}

void GPU::CMD_SwapBuffers() {
  Dequeue();
  
  for (uint i = 0; i < 256 * 192; i++) {
    output[i] = 0x8000;
  }

  for (int i = 0; i < polygon.count; i++) {
    Polygon const& poly = polygon.data[i];

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
      // CHECKME
      if (t_max == 0)
        return a;
      return (a * (t_max - t) + b * t) / t_max;
    };

    auto textureSample = [this](TextureParams const& texture_params, s16 uv[2]) -> u16 {
      // TODO: properly handle fractional part
      // TODO: reduce code redundancy by using arrays for u/v everywhere...
      s16 u = uv[0] >> 4;
      s16 v = uv[1] >> 4;

      if (u < 0) {
        if (texture_params.repeat_u) {
          // TODO: use shifts instead of multiplication and division...
          int repeats = -u / texture_params.size_u;
          u += (1 + repeats) * texture_params.size_u;
          if (texture_params.flip_u && (repeats & 1)) {
            u = texture_params.size_u - u - 1;
          }
        } else {
          u = 0;
        }
      } else if (u >= texture_params.size_u) {
        if (texture_params.repeat_u) {
          // TODO: use shifts instead of multiplication and division...
          int repeats = u / texture_params.size_u;
          u -= repeats * texture_params.size_u;
          if (texture_params.flip_u && (repeats & 1)) {
            u = texture_params.size_u - u - 1;
          }
        } else {
          u = texture_params.size_u - 1;
        }
      }

      if (v < 0) {
        if (texture_params.repeat_v) {
          // TODO: use shifts instead of multiplication and division...
          int repeats = -v / texture_params.size_v;
          v += (1 + repeats) * texture_params.size_v;
          if (texture_params.flip_v && (repeats & 1)) {
            v = texture_params.size_v - v - 1;
          }
        } else {
          v = 0;
        }
      } else if (v >= texture_params.size_v) {
        if (texture_params.repeat_v) {
          // TODO: use shifts instead of multiplication and division...
          int repeats = v / texture_params.size_v;
          v -= repeats * texture_params.size_v;
          if (texture_params.flip_v && (repeats & 1)) {
            v = texture_params.size_v - v - 1;
          }
        } else {
          v = texture_params.size_v - 1;
        }
      }

      switch (texture_params.format) {
        case TextureParams::Format::None: {
          return 0x7FFF;
        }
        case TextureParams::Format::Palette2BPP: {
          auto offset = v * texture_params.size_u + u;
          auto index = (vram_texture.Read<u8>(texture_params.address + (offset >> 2)) >> (2 * (offset & 3))) & 3;
          if (texture_params.color0_transparent && index == 0) {
            return 0x8000;
          }
          return vram_palette.Read<u16>((texture_params.palette_base << 3) + index * sizeof(u16)) & 0x7FFF;
        }
        case TextureParams::Format::Palette4BPP: {
          auto offset = v * texture_params.size_u + u;
          auto index = (vram_texture.Read<u8>(texture_params.address + (offset >> 1)) >> (4 * (offset & 1))) & 15;
          if (texture_params.color0_transparent && index == 0) {
            return 0x8000;
          }
          return vram_palette.Read<u16>((texture_params.palette_base << 4) + index * sizeof(u16)) & 0x7FFF;
        }
      };

      return u16(0x999);
    };

    bool skip = false;

    for (int j = 0; j < poly.count; j++) {
      auto const& vert = vertex.data[poly.indices[j]];
      auto& point = points[j];

      // FIXME
      if (vert.position[3] == 0) {
        skip = true;
      }

      // TODO: use the provided viewport configuration.
      point.x = (( (s64(vert.position[0]) << 12) / vert.position[3] * 128) >> 12) + 128;
      point.y = ((-(s64(vert.position[1]) << 12) / vert.position[3] *  96) >> 12) +  96;
      point.depth = (s64(vert.position[2]) << 12) / vert.position[3];
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
        uv0[j] = lerp(points[s0].vertex->uv[j], points[e0].vertex->uv[j], y - points[s0].y, points[e0].y - points[s0].y);
        uv1[j] = lerp(points[s1].vertex->uv[j], points[e1].vertex->uv[j], y - points[s1].y, points[e1].y - points[s1].y);
      }

      // TODO: find a faster way to swap the edges,
      // maybe use an index into an array?
      if (x0 > x1) {
        std::swap(x0, x1);
        std::swap(color0, color1);
        std::swap(uv0, uv1);
      }

      // TODO: boundary checks will be redundant if clipping and viewport tranform work properly.
      if (y >= 0 && y <= 191) {
        s32 color[3];
        s16 uv[2];

        for (s32 x = x0; x <= x1; x++) {
          for (int j = 0; j < 3; j++) {
            color[j] = lerp(color0[j], color1[j], x - x0, x1 - x0);
          }

          for (int j = 0; j < 2; j++) {
            uv[j] = lerp(uv0[j], uv1[j], x - x0, x1 - x0);
          }

          if (x >= 0 && x <= 255) {
            // FIXME: multiply vertex color with texel.
            //output[y * 256 + x] = (color[0] >> 1) |
            //                     ((color[1] >> 1) <<  5) |
            //                     ((color[2] >> 1) << 10);
            auto texel = textureSample(poly.texture_params, uv);
            if (texel != 0x8000) {
              output[y * 256 + x] = texel;
            }
          }
        }
      }
    }
  }

  vertex.count = 0;
  polygon.count = 0;
}

} // namespace fauxDS::core

