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
}

void GPU::CMD_SetNormal() {
}

void GPU::CMD_SetUV() {
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

void GPU::CMD_BeginVertexList() {
  auto arg = Dequeue().argument;
  in_vertex_list = true;
  vertex_counter = 0;
  is_quad = arg & 1;
  is_strip = arg & 2;
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
        return s64(b);
      return (s64(a) * (t_max - t) + s64(b) * t) / t_max;
    };

    for (int j = 0; j < poly.count; j++) {
      auto const& vert = vertex.data[poly.indices[j]];
      auto& point = points[j];

      ASSERT(vert.position[3] != 0, "GPU: w-Coordinate should not be zero.");

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

      s32 x0 = lerp(points[s0].x, points[e0].x, y - points[s0].y, points[e0].y - points[s0].y);
      s32 x1 = lerp(points[s1].x, points[e1].x, y - points[s1].y, points[e1].y - points[s1].y);

      if (x0 > x1)
        std::swap(x0, x1);

      // TODO: boundary checks will be redundant if clipping and viewport tranform work properly.
      if (y >= 0 && y <= 191) {
        for (s32 x = x0; x <= x1; x++) {
          if (x >= 0 && x <= 255) {
            output[y * 256 + x] = 0x999;
          }
        }
      }
    }

    for (int j = 0; j < poly.count; j++) {
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
    }
  }

  vertex.count = 0;
  polygon.count = 0;
}

} // namespace fauxDS::core

