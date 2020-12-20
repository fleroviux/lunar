/*
 * Copyright (C) 2020 fleroviux
 */

// Used for sorting, probably won't keep it...
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
  // TODO: cleanup and check correctness.
  AddVertex({
    s32(s16(((arg >>  0) & 0x3F) | (arg & (1 <<  9) ? 0xFFC0 : 0))) << 6,
    s32(s16(((arg >> 10) & 0x3F) | (arg & (1 << 19) ? 0xFFC0 : 0))) << 6,
    s32(s16(((arg >> 20) & 0x3F) | (arg & (1 << 29) ? 0xFFC0 : 0))) << 6,
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
  // TODO: cleanup and check correctness.
  AddVertex({
    position_old[0] + (s16(((arg >>  0) & 0x3F) | (arg & (1 <<  9) ? 0xFFC0 : 0)) << 3),
    position_old[1] + (s16(((arg >> 10) & 0x3F) | (arg & (1 << 19) ? 0xFFC0 : 0)) << 3),
    position_old[2] + (s16(((arg >> 20) & 0x3F) | (arg & (1 << 29) ? 0xFFC0 : 0)) << 3),
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
  // TODO: this is a massive hackfest...
  Dequeue();
  
  for (uint i = 0; i < 256 * 192; i++) {
    output[i] = 0x8000;
  }

  // TODO: handle quadliterals, like, at all...
  for (int i = 0; i < polygon.count; i++) {
    Polygon const& poly = polygon.data[i];

    auto const& p0 = vertex.data[poly.indices[0]].position;
    auto const& p1 = vertex.data[poly.indices[1]].position;
    auto const& p2 = vertex.data[poly.indices[2]].position;

    #define view_x(x) (((x * 128) >> 12) + 128)
    #define view_y(y) (((y *  96) >> 12) +  96)
    
    // Skip entire polygon if anything divides by zero...
    // This will be solved by clipping polygons against the view frustum.
    if (p0[3] == 0 || p1[3] == 0 || p2[3] == 0)
      continue;

    // TODO: use some kind of Vec2 struct to store 2d-points, this is atrocious...
    auto p0_x = view_x(( s64(p0[0]) << 12) / p0[3]);
    auto p0_y = view_y((-s64(p0[1]) << 12) / p0[3]);

    auto p1_x = view_x(( s64(p1[0]) << 12) / p1[3]);
    auto p1_y = view_y((-s64(p1[1]) << 12) / p1[3]);

    auto p2_x = view_x(( s64(p2[0]) << 12) / p2[3]);
    auto p2_y = view_y((-s64(p2[1]) << 12) / p2[3]);

    if (p0_y > p1_y || (p0_y == p1_y && p0_x > p1_x)) {
      std::swap(p0_x, p1_x);
      std::swap(p0_y, p1_y);
    }
    
    if (p1_y > p2_y || (p1_y == p2_y && p1_x > p2_x)) {
      std::swap(p1_x, p2_x);
      std::swap(p1_y, p2_y);
    }
    
    if (p0_y > p1_y || (p0_y == p1_y && p0_x > p1_x)) {
      std::swap(p0_x, p1_x);
      std::swap(p0_y, p1_y);
    }

    // TODO: use proper type.
    auto x0 = p2_x << 8;
    auto x1 = p1_x << 8;
    int x0_delta = 0;
    int x1_delta = 0;

    auto y0_delta = p2_y - p0_y;
    auto y1_delta = p1_y - p0_y;

    if (y0_delta != 0) {
      x0 = p0_x << 8;
      x0_delta = ((p2_x - p0_x) << 8) / y0_delta;
    }

    if (y1_delta != 0) {
      x1 = p0_x << 8;
      x1_delta = ((p1_x - p0_x) << 8) / y1_delta;
    }

    if (p1_x < p0_x) {
      std::swap(x0, x1);
      std::swap(x0_delta, x1_delta);
    }

    for (auto y = p0_y; y <= p2_y; y++) {
      if (y >= 0 && y <= 191) {
        auto _x0 = x0 >> 8;
        auto _x1 = x1 >> 8;

        for (auto x = _x0; x <= _x1; x++) {
          if (x >= 0 && x <= 255) {
            output[y * 256 + x] = 0x1F;
          }
        }

        if (_x0 >= 0 && _x0 <= 255) {
          output[y * 256 + _x0] = 0x1F << 10;
        }

        if (_x1 >= 0 && _x1 <= 255) {
          output[y * 256 + _x1] = 0x1F << 10;
        }
      }

      if (y == p1_y) {
        if (p1_x > p0_x) {
          x1_delta = (p2_y - p1_y) == 0 ? 0 : ((p2_x - p1_x) << 8) / (p2_y - p1_y);
        } else {
          x0_delta = (p2_y - p1_y) == 0 ? 0 : ((p2_x - p1_x) << 8) / (p2_y - p1_y);
        }
      }

      x0 += x0_delta;
      x1 += x1_delta;
    }

    if (p0_x >= 0 && p0_x <= 255 && p0_y >= 0 && p0_y <= 191) {
      output[p0_y * 256 + p0_x]  = 0x1F << 0;
    }

    if (p1_x >= 0 && p1_x <= 255 && p1_y >= 0 && p1_y <= 191) {
      output[p1_y * 256 + p1_x]  = 0x1F << 5;
    }

    if (p2_x >= 0 && p2_x <= 255 && p2_y >= 0 && p2_y <= 191) {
      output[p2_y * 256 + p2_x]  = 0x1F << 10;
    }
  }

  vertex.count = 0;
  polygon.count = 0;
}

} // namespace fauxDS::core

