/*
 * Copyright (C) 2020 fleroviux
 */

#include <algorithm>

#include "gpu.hpp"

namespace Duality::core {

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
  int offset = Dequeue().argument & 63;
  
  if (offset & 32) {
    offset -= 64;
  }
  
  switch (matrix_mode) {
    case MatrixMode::Projection:
      projection.Pop(offset);
      UpdateClipMatrix();
      break;
    case MatrixMode::Modelview:
    case MatrixMode::Simultaneous:
      modelview.Pop(offset);
      direction.Pop(offset);
      UpdateClipMatrix();
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

void GPU::CMD_RestoreMatrix() {
  auto address = Dequeue().argument & 31;

  switch (matrix_mode) {
    case MatrixMode::Projection:
      projection.Restore(address);
      UpdateClipMatrix();
      break;
    case MatrixMode::Modelview:
    case MatrixMode::Simultaneous:
      modelview.Restore(address);
      direction.Restore(address);
      UpdateClipMatrix();
      break;
    case MatrixMode::Texture:
      texture.Restore(address);
      break;
  }
}

void GPU::CMD_LoadIdentity() {
  Dequeue();
  
  switch (matrix_mode) {
    case MatrixMode::Projection:
      projection.current.LoadIdentity();
      UpdateClipMatrix();
      break;
    case MatrixMode::Modelview:
      modelview.current.LoadIdentity();
      UpdateClipMatrix();
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
      UpdateClipMatrix();
      break;
    case MatrixMode::Modelview:
      modelview.current = mat;
      UpdateClipMatrix();
      break;
    case MatrixMode::Simultaneous:
      modelview.current = mat;
      direction.current = mat;
      UpdateClipMatrix();
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
      UpdateClipMatrix();
      break;
    case MatrixMode::Modelview:
      modelview.current = mat;
      UpdateClipMatrix();
      break;
    case MatrixMode::Simultaneous:
      modelview.current = mat;
      direction.current = mat;
      UpdateClipMatrix();
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
      UpdateClipMatrix();
      break;
    case MatrixMode::Modelview:
      modelview.current *= mat;
      UpdateClipMatrix();
      break;
    case MatrixMode::Simultaneous:
      modelview.current *= mat;
      direction.current *= mat;
      UpdateClipMatrix();
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
      UpdateClipMatrix();
      break;
    case MatrixMode::Modelview:
      modelview.current *= mat;
      UpdateClipMatrix();
      break;
    case MatrixMode::Simultaneous:
      modelview.current *= mat;
      direction.current *= mat;
      UpdateClipMatrix();
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
      UpdateClipMatrix();
      break;
    case MatrixMode::Modelview:
      modelview.current *= mat;
      UpdateClipMatrix();
      break;
    case MatrixMode::Simultaneous:
      modelview.current *= mat;
      direction.current *= mat;
      UpdateClipMatrix();
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
      UpdateClipMatrix();
      break;
    case MatrixMode::Modelview:
    case MatrixMode::Simultaneous:
      modelview.current *= mat;
      UpdateClipMatrix();
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
      UpdateClipMatrix();
      break;
    case MatrixMode::Modelview:
    case MatrixMode::Simultaneous:
      modelview.current *= mat;
      UpdateClipMatrix();
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
  texture_params.repeat[0] = arg & (1 << 16);
  texture_params.repeat[1]= arg & (1 << 17);
  texture_params.flip[0] = arg & (1 << 18);
  texture_params.flip[1] = arg & (1 << 19);
  texture_params.size[0] = (arg >> 20) & 7;
  texture_params.size[1] = (arg >> 23) & 7;
  texture_params.format = static_cast<TextureParams::Format>((arg >> 26) & 7);
  texture_params.color0_transparent = arg & (1 << 29);
  texture_params.transform = static_cast<TextureParams::Transform>(arg >> 30);
}

void GPU::CMD_SetPaletteBase() {
  texture_params.palette_base = Dequeue().argument & 0x1FFF;
}

void GPU::CMD_BeginVertexList() {
  auto arg = Dequeue().argument;
  in_vertex_list = true;
  is_quad = arg & 1;
  is_strip = arg & 2;
  is_first = true;

  vertices.clear();

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

  // TODO: use data from rear image MMIO registers.
  for (uint i = 0; i < 256 * 192; i++) {
    output[i] = 0x8000;
    depthbuffer[i] = 0x7FFFFFFF;
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
      auto const& vert = vertex.data[poly.indices[j]];
      auto& point = points[j];

      // FIXME
      if (vert.position[3] == 0) {
        skip = true;
        break;
      }

      // TODO: use the provided viewport configuration.
      // FIXME: have some kind of way to handle fixed-point constants.
      point.x = ( vert.position.x() / vert.position.w() * (128 << 12)).integer() + 128;
      point.y = (-vert.position.y() / vert.position.w() * ( 96 << 12)).integer() +  96;
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
        uv0[j] = lerp(points[s0].vertex->uv[j], points[e0].vertex->uv[j], y - points[s0].y, points[e0].y - points[s0].y);
        uv1[j] = lerp(points[s1].vertex->uv[j], points[e1].vertex->uv[j], y - points[s1].y, points[e1].y - points[s1].y);
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

    /*for (int j = 0; j < poly.count; j++) {
      if (points[j].y < 0 || points[j].y > 191 || points[j].x < 0 || points[j].x > 255) {
        continue;
      }
      u16 color = 0x4269;
      switch (j) {
        case 0: color = 0x1F; break;
        case 1: color = 0x1f << 5; break;
        case 2: color = 0x1f << 10; break;
        case 3: color = (0x1F << 5) | 0x1F; break;
        case 4: color = 0x7FFF; break;
      }
      output[points[j].y * 256 + points[j].x] = color;
    }*/
  }

  vertex.count = 0;
  polygon.count = 0;
}

} // namespace Duality::core

