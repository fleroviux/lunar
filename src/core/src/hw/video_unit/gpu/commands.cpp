/*
 * Copyright (C) 2020 fleroviux
 */

#include <algorithm>

#include "gpu.hpp"

namespace Duality::Core {

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
      projection.current.identity();
      UpdateClipMatrix();
      break;
    case MatrixMode::Modelview:
      modelview.current.identity();
      UpdateClipMatrix();
      break;
    case MatrixMode::Simultaneous:
      modelview.current.identity();
      direction.current.identity();
      break;
    case MatrixMode::Texture:
      texture.current.identity();
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
      projection.current = projection.current * mat;
      UpdateClipMatrix();
      break;
    case MatrixMode::Modelview:
      modelview.current = modelview.current * mat;
      UpdateClipMatrix();
      break;
    case MatrixMode::Simultaneous:
      modelview.current = modelview.current * mat;
      direction.current = direction.current * mat;
      UpdateClipMatrix();
      break;
    case MatrixMode::Texture:
      texture.current = texture.current * mat;
      break;
  }
}

void GPU::CMD_MatrixMultiply4x3() {
  auto mat = DequeueMatrix4x3();
  
  switch (matrix_mode) {
    case MatrixMode::Projection:
      projection.current = projection.current * mat;
      UpdateClipMatrix();
      break;
    case MatrixMode::Modelview:
      modelview.current = modelview.current * mat;
      UpdateClipMatrix();
      break;
    case MatrixMode::Simultaneous:
      modelview.current = modelview.current * mat;
      direction.current = direction.current * mat;
      UpdateClipMatrix();
      break;
    case MatrixMode::Texture:
      texture.current = texture.current * mat;
      break;
  }
}

void GPU::CMD_MatrixMultiply3x3() {
  auto mat = DequeueMatrix3x3();
  
  switch (matrix_mode) {
    case MatrixMode::Projection:
      projection.current = projection.current * mat;
      UpdateClipMatrix();
      break;
    case MatrixMode::Modelview:
      modelview.current = modelview.current * mat;
      UpdateClipMatrix();
      break;
    case MatrixMode::Simultaneous:
      modelview.current = modelview.current * mat;
      direction.current = direction.current * mat;
      UpdateClipMatrix();
      break;
    case MatrixMode::Texture:
      texture.current = texture.current * mat;
      break;
  }
}

void GPU::CMD_MatrixScale() {
  // TODO: this implementation is unoptimized.
  // A matrix multiplication is complete overkill for scaling.
  
  Matrix4<Fixed20x12> mat;
  for (int i = 0; i < 3; i++) {
    mat[i][i] = Dequeue().argument;
  }
  mat[3][3] = 0x1000;
  
  switch (matrix_mode) {
    case MatrixMode::Projection:
      projection.current = projection.current * mat;
      UpdateClipMatrix();
      break;
    case MatrixMode::Modelview:
    case MatrixMode::Simultaneous:
      modelview.current = modelview.current * mat;
      UpdateClipMatrix();
      break;
    case MatrixMode::Texture:
      texture.current = texture.current * mat;
      break;
  }
}

void GPU::CMD_MatrixTranslate() {
  Matrix4<Fixed20x12> mat;
  mat.identity();
  for (int i = 0; i < 3; i++) {
    mat[3][i] = Dequeue().argument;
  }
  
  switch (matrix_mode) {
    case MatrixMode::Projection:
      projection.current = projection.current * mat;
      UpdateClipMatrix();
      break;
    case MatrixMode::Modelview:
    case MatrixMode::Simultaneous:
      modelview.current = modelview.current * mat;
      UpdateClipMatrix();
      break;
    case MatrixMode::Texture:
      texture.current = texture.current * mat;
      break;
  }
}

void GPU::CMD_SetColor() {
  vertex_color = Color4::from_rgb555(u16(Dequeue().argument));
}

void GPU::CMD_SetNormal() {
  auto arg = Dequeue().argument;

  auto x = Fixed20x12{s16(((arg >>  0) & 0x3FF) << 6) >> 3};
  auto y = Fixed20x12{s16(((arg >> 10) & 0x3FF) << 6) >> 3};
  auto z = Fixed20x12{s16(((arg >> 20) & 0x3FF) << 6) >> 3};

  if (texture_params.transform == TextureParams::Transform::Normal) {
    auto const& matrix = texture.current;

    auto t_x = Fixed20x12{vertex_uv_source.x().raw() << 12};
    auto t_y = Fixed20x12{vertex_uv_source.y().raw() << 12};

    vertex_uv = Vector2<Fixed12x4>{
      s16((x * matrix[0].x() + y * matrix[1].x() + z * matrix[2].x() + t_x).raw() >> 12),
      s16((x * matrix[0].y() + y * matrix[1].y() + z * matrix[2].y() + t_y).raw() >> 12)
    };
  }

  // CHECKME: is the command supposed to overwrite the previous vertex color?
  vertex_color = Color4{0, 0, 0, 0};

  for (int i = 0; i < 4; i++) {
    // TODO: latch the enable bits on BeginVertexList commands?
    if (!poly_params.enable_light[i]) {
      continue;
    }

    auto const& light = lights[i];

    // TODO: do not create the normal vector twice...
    auto cos_theta = -light.direction.dot(Vector3{x, y, z});
    auto shinyness = -light.halfway.dot(Vector3{x, y, z});

    // TODO: support min/max/clamp for the fixed point types.
    if (cos_theta.raw() < 0) cos_theta = NumericConstants<Fixed20x12>::zero();
    if (shinyness.raw() < 0) shinyness = NumericConstants<Fixed20x12>::zero();

    shinyness *= shinyness;

    if (material.enable_shinyness_table) {
      // TODO!
    }

    auto diffuse_r = (material.diffuse.r().raw() * cos_theta.raw() * light.color.r().raw()) >> 18;
    auto diffuse_g = (material.diffuse.g().raw() * cos_theta.raw() * light.color.g().raw()) >> 18;
    auto diffuse_b = (material.diffuse.b().raw() * cos_theta.raw() * light.color.b().raw()) >> 18;

    auto specular_r = (material.specular.r().raw() * shinyness.raw() * light.color.r().raw()) >> 18;
    auto specular_g = (material.specular.g().raw() * shinyness.raw() * light.color.g().raw()) >> 18;
    auto specular_b = (material.specular.b().raw() * shinyness.raw() * light.color.b().raw()) >> 18;

    auto ambient_r = (material.ambient.r().raw() * light.color.r().raw()) >> 6;
    auto ambient_g = (material.ambient.g().raw() * light.color.g().raw()) >> 6;
    auto ambient_b = (material.ambient.b().raw() * light.color.b().raw()) >> 6;

    vertex_color.r() = std::clamp(vertex_color.r().raw() + diffuse_r + specular_r + ambient_r, 0, 63);
    vertex_color.g() = std::clamp(vertex_color.g().raw() + diffuse_g + specular_g + ambient_g, 0, 63);
    vertex_color.b() = std::clamp(vertex_color.b().raw() + diffuse_b + specular_b + ambient_b, 0, 63);
  }
}

void GPU::CMD_SetUV() {
  auto arg = Dequeue().argument;

  auto u = s16(arg & 0xFFFF);
  auto v = s16(arg >> 16);

  vertex_uv_source = Vector2<Fixed12x4>{u, v};

  if (texture_params.transform == TextureParams::Transform::TexCoord) {
    auto const& matrix = texture.current;

    auto x = Fixed20x12{u << 8};
    auto y = Fixed20x12{v << 8};

    auto t_x = (matrix[2].x() + matrix[3].x()) * Fixed20x12{256};
    auto t_y = (matrix[2].y() + matrix[3].y()) * Fixed20x12{256};

    vertex_uv = Vector2<Fixed12x4>{
      s16((x * matrix[0].x() + y * matrix[1].x() + t_x).raw() >> 8),
      s16((x * matrix[0].y() + y * matrix[1].y() + t_y).raw() >> 8),
    };
  } else {
    vertex_uv = vertex_uv_source;
  }
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

void GPU::CMD_SetPolygonAttributes() {
  auto arg = Dequeue().argument;

  poly_params.enable_light[0] = arg & 1;
  poly_params.enable_light[1] = arg & 2;
  poly_params.enable_light[2] = arg & 4;
  poly_params.enable_light[3] = arg & 8;
  poly_params.render_back_side = arg & 16;
  poly_params.render_front_side = arg & 32;
  poly_params.enable_translucent_depth_write = arg & (1 << 11);
  poly_params.render_far_plane_polys = arg & (1 << 12);
  poly_params.render_1dot_depth_tested = arg & (1 << 13);
  poly_params.depth_test = static_cast<PolygonParams::DepthTest>((arg >> 14) & 1);
  poly_params.enable_fog = arg & (1 << 15);
  poly_params.alpha = (arg >> 16) & 31;
  poly_params.polygon_id = (arg >> 24) & 63;
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

void GPU::CMD_SetMaterialColor0() {
  auto arg = Dequeue().argument;

  material.diffuse = Color4::from_rgb555(arg & 0x7FFF);
  material.ambient = Color4::from_rgb555(arg >> 16);

  if (arg & 0x8000) {
    vertex_color = material.diffuse;
  }
}

void GPU::CMD_SetMaterialColor1() {
  auto arg = Dequeue().argument;

  material.specular = Color4::from_rgb555(arg & 0x7FFF);
  material.emissive = Color4::from_rgb555(arg >> 16);

  material.enable_shinyness_table = arg & 0x8000;
}

void GPU::CMD_SetLightVector() {
  auto arg = Dequeue().argument;

  auto& light = lights[arg >> 30];

  auto direction = Vector4<Fixed20x12>{
    s16(((arg >>  0) & 0x3FF) << 6) >> 3,
    s16(((arg >> 10) & 0x3FF) << 6) >> 3,
    s16(((arg >> 20) & 0x3FF) << 6) >> 3,
    NumericConstants<Fixed20x12>::one()
  };

  light.direction = (this->direction.current * direction).xyz();
  
  // halfway = (direction + (0, 0, -1)) * 0.5
  light.halfway = Vector3<Fixed20x12>{
    light.direction.x().raw() >> 1,
    light.direction.y().raw() >> 1,
   (light.direction.z() - NumericConstants<Fixed20x12>::one()).raw() >> 1
  };
}

void GPU::CMD_SetLightColor() {
  auto arg = Dequeue().argument;

  lights[arg >> 30].color = Color4::from_rgb555(arg & 0x7FFF);
}

void GPU::CMD_BeginVertexList() {
  auto arg = Dequeue().argument;
  in_vertex_list = true;
  is_quad = arg & 1;
  is_strip = arg & 2;
  is_first = true;

  vertices.clear();
}

void GPU::CMD_EndVertexList() {
  Dequeue();
  // TODO: allegedly this command is a no-operation on the DS...
  in_vertex_list = false;
}

void GPU::CMD_SwapBuffers() {
  Dequeue();
  gx_buffer_id ^= 1;
  vertex[gx_buffer_id].count = 0;
  polygon[gx_buffer_id].count = 0;
}

} // namespace Duality::Core

