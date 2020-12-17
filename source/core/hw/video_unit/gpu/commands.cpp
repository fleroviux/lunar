/*
 * Copyright (C) 2020 fleroviux
 */

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
  Matrix4x4 mat;
  
  for (int col = 0; col < 4; col++) {
    for (int row = 0; row < 4; row++) {
      mat[col][row] = Dequeue().argument;
    }
  }
  
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
  Matrix4x4 mat;
  
  for (int col = 0; col < 4; col++) {
    for (int row = 0; row < 3; row++) {
      mat[col][row] = Dequeue().argument;
    }
  }
  
  mat[3][3] = 0x1000;
  
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
  Matrix4x4 mat;
  
  for (int col = 0; col < 4; col++) {
    for (int row = 0; row < 4; row++) {
      mat[col][row] = Dequeue().argument;
    }
  }
  
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
  Matrix4x4 mat;
  
  for (int col = 0; col < 4; col++) {
    for (int row = 0; row < 3; row++) {
      mat[col][row] = Dequeue().argument;
    }
  }
  
  // TODO: I really have no clue if this is correct at all!
  mat[3][3] = 0x1000;
  
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
  Matrix4x4 mat;
  
  for (int col = 0; col < 3; col++) {
    for (int row = 0; row < 3; row++) {
      mat[col][row] = Dequeue().argument;
    }
  }
  
  mat[3][3] = 0x1000;
  
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
  // TODO: this can be a lot more optimized...
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
      modelview.current *= mat;
      break;
    case MatrixMode::Simultaneous:
      modelview.current *= mat;
      break;
    case MatrixMode::Texture:
      texture.current *= mat;
      break;
  }
}

void GPU::CMD_MatrixTranslate() {
  Vector4 vec;
  
  for (int i = 0; i < 3; i++) {
    vec[i] = Dequeue().argument;
  }
  
  switch (matrix_mode) {
    case MatrixMode::Projection:
      projection.current[3] += vec;
      break;
    case MatrixMode::Modelview:
      modelview.current[3] += vec;
      break;
    case MatrixMode::Simultaneous:
      modelview.current[3] += vec;
      direction.current[3] += vec;
      break;
    case MatrixMode::Texture:
      texture.current[3] += vec;
      break;
  }
}

} // namespace fauxDS::core

