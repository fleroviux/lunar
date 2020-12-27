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
  Dequeue();
  
  for (uint i = 0; i < 256 * 192; i++) {
    output[i] = 0x8000;
  }

  for (int i = 0; i < polygon.count; i++) {
    Polygon const& poly = polygon.data[i];

    // TODO: remove this, it is unnecessary now.
    int num_vertices = poly.count;

    // TODO: move this to a less questionable place.
    // TODO: once clipping is in place, probably u8 sbould be sufficient.
    struct Point {
      s32 x;
      s32 y;
    };

    Point points[num_vertices];

    bool skip = false;

    for (int j = 0; j < num_vertices; j++) {
      auto const& vert = vertex.data[poly.indices[j]];
      // Avoid division-by-zero. Later this most likely will be rendered redundant by clipping.
      if (vert.position[3] == 0) {
        skip = true;
        break;
      }
      points[j].x = (( vert.position[0] * 128) >> 12) + 128;
      points[j].y = ((-vert.position[1] *  96) >> 12) +  96;
    }

    if (skip) {
      continue;
    }

    int start = 0;
    s32 y_min = 256;
    s32 y_max = 0;

    // TODO: it is unclear how exactly the start vertex is selected,
    // besides being one of the vertices with the lowest y-Value.
    for (int j = 0; j < num_vertices; j++) {
      auto const& point = points[j];
      if (point.y < y_min) {
        y_min = point.y;
        start = j;
      }
      if (point.y > y_max) {
        y_max = point.y;
      }
    }

    // TODO: absolutely make this less sucky... once things work.
    struct Edge {
      s32 x;
      s32 delta;
      s32 y_end;
    
      Edge(Point const& start, Point const& end) {
        s32 y_delta = end.y - start.y;
        // TODO: hardware internally uses a higher precision than 8-bit.
        if (y_delta != 0) {
          x = start.x << 18;
          delta = ((end.x - start.x) << 18) / y_delta;
        } else {
          x = end.x << 18;
          delta = 0;
        }
        y_end = end.y;
      }
    };

    int a = start;
    int b = start;

    Edge edge_a { points[a], points[(num_vertices + a - 1) % num_vertices] };
    Edge edge_b { points[b], points[(b + 1) % num_vertices] };

    for (s32 y = y_min; y <= y_max; y++) {
      if (edge_a.y_end == y) {
        a = (num_vertices + a - 1) % num_vertices;
        edge_a = { points[a], points[(num_vertices + a - 1) % num_vertices] };
      }

      if (edge_b.y_end == y) {
        b = (b + 1) % num_vertices;
        edge_b = { points[b], points[(b + 1) % num_vertices] };
      }

      if (y >= 0 && y <= 191) {
        auto _x0 = edge_a.x >> 18;
        auto _x1 = edge_b.x >> 18;
        if (_x0 > _x1) std::swap(_x0, _x1);
        for (auto x = _x0; x <= _x1; x++) {
          if (x >= 0 && x <= 255) {
            output[y * 256 + x] = 0x1F;
          }
        }
        if (_x0 >= 0 && _x0 <= 255) {
          output[y * 256 + _x0] = 0x666;
        }
        if (_x1 >= 0 && _x1 <= 255) {
          output[y * 256 + _x1] = 0x666;
        }
      }

      edge_a.x += edge_a.delta;
      edge_b.x += edge_b.delta;
    }

    /*for (int j = 0; j < num_vertices; j++) {
      output[points[j].y * 256 + points[j].x] = 0x1F << 10;
    }*/
  }

  vertex.count = 0;
  polygon.count = 0;
}

} // namespace fauxDS::core

