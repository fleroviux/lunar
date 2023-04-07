/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <atom/logger/logger.hpp>
#include <atom/panic.hpp>
#include <algorithm>

#include "gpu.hpp"

namespace lunar::nds {

static constexpr int kCmdNumParams[256] {
  0, 0, 0, 0,  0, 0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, // 0x00 - 0x0F (all NOPs)
  1, 0, 1, 1,  1, 0, 16, 12, 16, 12, 9, 3, 3, 0, 0, 0, // 0x10 - 0x1F (Matrix engine)
  1, 1, 1, 2,  1, 1,  1,  1,  1,  1, 1, 1, 0, 0, 0, 0, // 0x20 - 0x2F (Vertex and polygon attributes, mostly)
  1, 1, 1, 1, 32, 0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, // 0x30 - 0x3F (Material / lighting properties)
  1, 0, 0, 0,  0, 0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, // 0x40 - 0x4F (Begin/end vertex)
  1, 0, 0, 0,  0, 0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, // 0x50 - 0x5F (Swap buffers)
  1, 0, 0, 0,  0, 0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0, // 0x60 - 0x6F (Set viewport)
  3, 2, 1                                              // 0x70 - 0x7F (Box, position and vector test)
};

void GPU::WriteGXFIFO(u32 value) {
  u8 command;

  // Handle arguments for the correct command.
  if (packed_args_left != 0) {
    command = packed_cmds & 0xFF;
    Enqueue({ command, value });

    // Do not process further commands until all arguments have been send.
    if (--packed_args_left != 0) {
      return;
    }

    packed_cmds >>= 8;
  } else {
    packed_cmds = value;
  }

  // Enqueue commands that don't have any arguments,
  // but only until we encounter a command which does require arguments.
  while (packed_cmds != 0) {
    command = packed_cmds & 0xFF;
    packed_args_left = kCmdNumParams[command];
    if (packed_args_left == 0) {
      Enqueue({ command, 0 });
      packed_cmds >>= 8;
    } else {
      break;
    }
  }
}

void GPU::WriteCommandPort(uint port, u32 value) {
  if (port <= 0x3F || port >= 0x1CC) {
    ATOM_ERROR("GPU: unknown command port 0x{0:03X}", port);
    return;
  }

  Enqueue({ static_cast<u8>(port >> 2), value });
}

void GPU::Enqueue(CmdArgPack pack) {
  if (gxfifo.IsEmpty() && !gxpipe.IsFull()) {
    gxpipe.Write(pack);
  } else {
    /* HACK: before we drop any command or argument data,
     * execute the next command early.
     * In hardware enqueueing into full queue would stall the CPU or DMA,
     * but this is difficult to emulate accurately.
     */
    while (gxfifo.IsFull()) {
      gxstat.gx_busy = false;
      if (cmd_event != nullptr) {
        scheduler.Cancel(cmd_event);
      }
      ProcessCommands();
    }

    gxfifo.Write(pack);
    dma9.SetGXFIFOHalfEmpty(gxfifo.Count() < 128);
  }

  ProcessCommands();
}

auto GPU::Dequeue() -> CmdArgPack {
  if(gxpipe.Count() == 0) {
    ATOM_PANIC("GPU: attempted to dequeue entry from empty GXPIPE");
  }

  auto entry = gxpipe.Read();
  if (gxpipe.Count() <= 2 && !gxfifo.IsEmpty()) {
    gxpipe.Write(gxfifo.Read());
    CheckGXFIFO_IRQ();

    if (gxfifo.Count() < 128) {
      dma9.SetGXFIFOHalfEmpty(true);
      dma9.Request(DMA9::Time::GxFIFO);
    } else {
      dma9.SetGXFIFOHalfEmpty(false);
    }
  }

  return entry;
}

void GPU::ProcessCommands() {
  auto count = gxpipe.Count() + gxfifo.Count();

  if (count == 0 || gxstat.gx_busy) {
    return;
  }

  auto command = gxpipe.Peek().command;
  auto arg_count = kCmdNumParams[command];

  if (count >= arg_count) {
    switch (command) {
      case 0x10: CMD_SetMatrixMode(); break;
      case 0x11: CMD_PushMatrix(); break;
      case 0x12: CMD_PopMatrix(); break;
      case 0x13: CMD_StoreMatrix(); break;
      case 0x14: CMD_RestoreMatrix(); break;
      case 0x15: CMD_LoadIdentity(); break;
      case 0x16: CMD_LoadMatrix4x4(); break;
      case 0x17: CMD_LoadMatrix4x3(); break;
      case 0x18: CMD_MatrixMultiply4x4(); break;
      case 0x19: CMD_MatrixMultiply4x3(); break;
      case 0x1A: CMD_MatrixMultiply3x3(); break;
      case 0x1B: CMD_MatrixScale(); break;
      case 0x1C: CMD_MatrixTranslate(); break;
      case 0x20: CMD_SetColor(); break;
      case 0x21: CMD_SetNormal(); break;
      case 0x22: CMD_SetUV(); break;
      case 0x23: CMD_SubmitVertex_16(); break;
      case 0x24: CMD_SubmitVertex_10(); break;
      case 0x25: CMD_SubmitVertex_XY(); break;
      case 0x26: CMD_SubmitVertex_XZ(); break;
      case 0x27: CMD_SubmitVertex_YZ(); break;
      case 0x28: CMD_SubmitVertex_Offset(); break;
      case 0x29: CMD_SetPolygonAttributes(); break;
      case 0x2A: CMD_SetTextureParameters(); break;
      case 0x2B: CMD_SetPaletteBase(); break;
      case 0x30: CMD_SetMaterialColor0(); break;
      case 0x31: CMD_SetMaterialColor1(); break;
      case 0x32: CMD_SetLightVector(); break;
      case 0x33: CMD_SetLightColor(); break;
      case 0x40: CMD_BeginVertexList(); break;
      case 0x41: CMD_EndVertexList(); break;
      case 0x50: CMD_SwapBuffers(); break;
      default: {
        ATOM_ERROR("GPU: unimplemented command 0x{0:02X}", command);
        Dequeue();
        for (int i = 1; i < arg_count; i++)
          Dequeue();
        break;
      }
    }

    if (!swap_buffers_pending) {
      // TODO: scheduling a bunch of events with 1 cycle delay might be a tad slow.
      gxstat.gx_busy = true;
      cmd_event = scheduler.Add(1, [this](int cycles_late) {
        gxstat.gx_busy = false;
        cmd_event = nullptr;
        ProcessCommands();
      });
    }
  }
}

void GPU::CMD_SetMatrixMode() {
  matrix_mode = static_cast<MatrixMode>(Dequeue().argument & 3);
}

void GPU::CMD_PushMatrix() {
  Dequeue();
  
  switch (matrix_mode) {
    case MatrixMode::Projection: {
      projection.Push();
      break;
    }
    case MatrixMode::Modelview:
    case MatrixMode::Simultaneous: {
      modelview.Push();
      direction.Push();
      break;
    }
    case MatrixMode::Texture: {
      texture.Push();
      break;
    }
  }
}

void GPU::CMD_PopMatrix() {
  int offset = Dequeue().argument & 63;
  
  if (offset & 32) {
    offset -= 64;
  }
  
  switch (matrix_mode) {
    case MatrixMode::Projection: {
      projection.Pop(offset);
      UpdateClipMatrix();
      break;
    }
    case MatrixMode::Modelview:
    case MatrixMode::Simultaneous: {
      modelview.Pop(offset);
      direction.Pop(offset);
      UpdateClipMatrix();
      break;
    }
    case MatrixMode::Texture: {
      texture.Pop(offset);
      break;
    }
  }
}

void GPU::CMD_StoreMatrix() {
  auto address = Dequeue().argument & 31;
  
  switch (matrix_mode) {
    case MatrixMode::Projection: {
      projection.Store(address);
      break;
    }
    case MatrixMode::Modelview:
    case MatrixMode::Simultaneous: {
      modelview.Store(address);
      direction.Store(address);
      break;
    }
    case MatrixMode::Texture: {
      texture.Store(address);
      break;
    }
  }
}

void GPU::CMD_RestoreMatrix() {
  auto address = Dequeue().argument & 31;

  switch (matrix_mode) {
    case MatrixMode::Projection: {
      projection.Restore(address);
      UpdateClipMatrix();
      break;
    }
    case MatrixMode::Modelview:
    case MatrixMode::Simultaneous: {
      modelview.Restore(address);
      direction.Restore(address);
      UpdateClipMatrix();
      break;
    }
    case MatrixMode::Texture: {
      texture.Restore(address);
      break;
    }
  }
}

void GPU::CMD_LoadIdentity() {
  Dequeue();
  
  switch (matrix_mode) {
    case MatrixMode::Projection: {
      projection.current = Matrix4<Fixed20x12>::Identity();
      UpdateClipMatrix();
      break;
    }
    case MatrixMode::Modelview: {
      modelview.current = Matrix4<Fixed20x12>::Identity();
      UpdateClipMatrix();
      break;
    }
    case MatrixMode::Simultaneous: {
      modelview.current = Matrix4<Fixed20x12>::Identity();
      direction.current = Matrix4<Fixed20x12>::Identity();
      UpdateClipMatrix();
      break;
    }
    case MatrixMode::Texture: {
      texture.current = Matrix4<Fixed20x12>::Identity();
      break;
    }
  }
}

void GPU::CMD_LoadMatrix4x4() {
  auto mat = DequeueMatrix4x4();
  
  switch (matrix_mode) {
    case MatrixMode::Projection: {
      projection.current = mat;
      UpdateClipMatrix();
      break;
    }
    case MatrixMode::Modelview: {
      modelview.current = mat;
      UpdateClipMatrix();
      break;
    }
    case MatrixMode::Simultaneous: {
      modelview.current = mat;
      direction.current = mat;
      UpdateClipMatrix();
      break;
    }
    case MatrixMode::Texture: {
      texture.current = mat;
      break;
    }
  }
}

void GPU::CMD_LoadMatrix4x3() {
  auto mat = DequeueMatrix4x3();
  
  switch (matrix_mode) {
    case MatrixMode::Projection: {
      projection.current = mat;
      UpdateClipMatrix();
      break;
    }
    case MatrixMode::Modelview: {
      modelview.current = mat;
      UpdateClipMatrix();
      break;
    }
    case MatrixMode::Simultaneous: {
      modelview.current = mat;
      direction.current = mat;
      UpdateClipMatrix();
      break;
    }
    case MatrixMode::Texture: {
      texture.current = mat;
      break;
    }
  }
}

void GPU::CMD_MatrixMultiply4x4() {
  auto mat = DequeueMatrix4x4();
  
  switch (matrix_mode) {
    case MatrixMode::Projection: {
      projection.current = projection.current * mat;
      UpdateClipMatrix();
      break;
    }
    case MatrixMode::Modelview: {
      modelview.current = modelview.current * mat;
      UpdateClipMatrix();
      break;
    }
    case MatrixMode::Simultaneous: {
      modelview.current = modelview.current * mat;
      direction.current = direction.current * mat;
      UpdateClipMatrix();
      break;
    }
    case MatrixMode::Texture: {
      texture.current = texture.current * mat;
      break;
    }
  }
}

void GPU::CMD_MatrixMultiply4x3() {
  auto mat = DequeueMatrix4x3();
  
  switch (matrix_mode) {
    case MatrixMode::Projection: {
      projection.current = projection.current * mat;
      UpdateClipMatrix();
      break;
    }
    case MatrixMode::Modelview: {
      modelview.current = modelview.current * mat;
      UpdateClipMatrix();
      break;
    }
    case MatrixMode::Simultaneous: {
      modelview.current = modelview.current * mat;
      direction.current = direction.current * mat;
      UpdateClipMatrix();
      break;
    }
    case MatrixMode::Texture: {
      texture.current = texture.current * mat;
      break;
    }
  }
}

void GPU::CMD_MatrixMultiply3x3() {
  auto mat = DequeueMatrix3x3();
  
  switch (matrix_mode) {
    case MatrixMode::Projection: {
      projection.current = projection.current * mat;
      UpdateClipMatrix();
      break;
    }
    case MatrixMode::Modelview: {
      modelview.current = modelview.current * mat;
      UpdateClipMatrix();
      break;
    }
    case MatrixMode::Simultaneous: {
      modelview.current = modelview.current * mat;
      direction.current = direction.current * mat;
      UpdateClipMatrix();
      break;
    }
    case MatrixMode::Texture: {
      texture.current = texture.current * mat;
      break;
    }
  }
}

void GPU::CMD_MatrixScale() {
  Matrix4<Fixed20x12> mat;

  for (int i = 0; i < 3; i++) {
    mat[i][i] = Dequeue().argument;
  }
  mat[3][3] = 0x1000;
  
  switch (matrix_mode) {
    case MatrixMode::Projection: {
      projection.current = projection.current * mat;
      UpdateClipMatrix();
      break;
    }
    case MatrixMode::Modelview:
    case MatrixMode::Simultaneous: {
      modelview.current = modelview.current * mat;
      UpdateClipMatrix();
      break;
    }
    case MatrixMode::Texture: {
      texture.current = texture.current * mat;
      break;
    }
  }
}

void GPU::CMD_MatrixTranslate() {
  Matrix4<Fixed20x12> mat;

  mat = Matrix4<Fixed20x12>::Identity();
  for (int i = 0; i < 3; i++) {
    mat[3][i] = Dequeue().argument;
  }
  
  switch (matrix_mode) {
    case MatrixMode::Projection: {
      projection.current = projection.current * mat;
      UpdateClipMatrix();
      break;
    }
    case MatrixMode::Modelview:
    case MatrixMode::Simultaneous: {
      modelview.current = modelview.current * mat;
      UpdateClipMatrix();
      break;
    }
    case MatrixMode::Texture: {
      texture.current = texture.current * mat;
      break;
    }
  }
}

void GPU::CMD_SetColor() {
  vertex_color = Color4::FromRGB555(u16(Dequeue().argument));
}

void GPU::CMD_SetNormal() {
  auto arg = Dequeue().argument;

  auto x = Fixed20x12{s16(((arg >>  0) & 0x3FF) << 6) >> 3};
  auto y = Fixed20x12{s16(((arg >> 10) & 0x3FF) << 6) >> 3};
  auto z = Fixed20x12{s16(((arg >> 20) & 0x3FF) << 6) >> 3};

  if (texture_params.transform == TextureParams::Transform::Normal) {
    auto const& matrix = texture.current;

    auto t_x = Fixed20x12{vertex_uv_source.X().raw() << 12};
    auto t_y = Fixed20x12{vertex_uv_source.Y().raw() << 12};

    vertex_uv = Vector2<Fixed12x4>{
      s16((x * matrix[0].X() + y * matrix[1].X() + z * matrix[2].X() + t_x).raw() >> 12),
      s16((x * matrix[0].Y() + y * matrix[1].Y() + z * matrix[2].Y() + t_y).raw() >> 12)
    };
  }

  auto normal = (direction.current * Vector4<Fixed20x12>{ x, y, z, atom::NumericConstants<Fixed20x12>::Zero() }).XYZ();

  vertex_color = material.emissive;

  for (int i = 0; i < 4; i++) {
    // TODO: latch the enable bits on BeginVertexList commands?
    if (!poly_params.enable_light[i]) {
      continue;
    }

    auto const& light = lights[i];

    auto cos_theta = -light.direction.Dot(normal);
    auto shinyness = -light.halfway.Dot(normal);

    // TODO: support min/max/clamp for the fixed point types.
    if (cos_theta.raw() < 0) cos_theta = atom::NumericConstants<Fixed20x12>::Zero();
    if (shinyness.raw() < 0) shinyness = atom::NumericConstants<Fixed20x12>::Zero();

    shinyness *= shinyness;

    if (material.enable_shinyness_table) {
      // TODO!
    }

    auto diffuse_r = (material.diffuse.R().raw() * cos_theta.raw() * light.color.R().raw()) >> 18;
    auto diffuse_g = (material.diffuse.G().raw() * cos_theta.raw() * light.color.G().raw()) >> 18;
    auto diffuse_b = (material.diffuse.B().raw() * cos_theta.raw() * light.color.B().raw()) >> 18;

    auto specular_r = (material.specular.R().raw() * shinyness.raw() * light.color.R().raw()) >> 18;
    auto specular_g = (material.specular.G().raw() * shinyness.raw() * light.color.G().raw()) >> 18;
    auto specular_b = (material.specular.B().raw() * shinyness.raw() * light.color.B().raw()) >> 18;

    auto ambient_r = (material.ambient.R().raw() * light.color.R().raw()) >> 6;
    auto ambient_g = (material.ambient.G().raw() * light.color.G().raw()) >> 6;
    auto ambient_b = (material.ambient.B().raw() * light.color.B().raw()) >> 6;

    vertex_color.R() = std::clamp(vertex_color.R().raw() + diffuse_r + specular_r + ambient_r, 0, 63);
    vertex_color.G() = std::clamp(vertex_color.G().raw() + diffuse_g + specular_g + ambient_g, 0, 63);
    vertex_color.B() = std::clamp(vertex_color.B().raw() + diffuse_b + specular_b + ambient_b, 0, 63);
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

    auto t_x = (matrix[2].X() + matrix[3].X()) * Fixed20x12{256};
    auto t_y = (matrix[2].Y() + matrix[3].Y()) * Fixed20x12{256};

    vertex_uv = Vector2<Fixed12x4>{
      s16((x * matrix[0].X() + y * matrix[1].X() + t_x).raw() >> 8),
      s16((x * matrix[0].Y() + y * matrix[1].Y() + t_y).raw() >> 8),
    };
  } else {
    vertex_uv = vertex_uv_source;
  }
}

void GPU::CMD_SubmitVertex_16() {
  auto arg0 = Dequeue().argument;
  auto arg1 = Dequeue().argument;

  SubmitVertex({
   s16(arg0 & 0xFFFF),
   s16(arg0 >> 16),
   s16(arg1 & 0xFFFF),
   0x1000
  });
}

void GPU::CMD_SubmitVertex_10() {
  auto arg = Dequeue().argument;

  SubmitVertex({
   s16((arg >> 0) << 6),
   s16((arg >> 10) << 6),
   s16((arg >> 20) << 6),
   0x1000
  });
}

void GPU::CMD_SubmitVertex_XY() {
  auto arg = Dequeue().argument;

  SubmitVertex({
   s16(arg & 0xFFFF),
   s16(arg >> 16),
   position_old[2],
   0x1000
  });
}

void GPU::CMD_SubmitVertex_XZ() {
  auto arg = Dequeue().argument;
  SubmitVertex({
   s16(arg & 0xFFFF),
   position_old[1],
   s16(arg >> 16),
   0x1000
  });
}

void GPU::CMD_SubmitVertex_YZ() {
  auto arg = Dequeue().argument;
  SubmitVertex({
   position_old[0],
   s16(arg & 0xFFFF),
   s16(arg >> 16),
   0x1000
  });
}

void GPU::CMD_SubmitVertex_Offset() {
  auto arg = Dequeue().argument;
  SubmitVertex({
   position_old[0] + (s16((arg >> 0) << 6) >> 6),
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
  poly_params.mode = static_cast<PolygonParams::Mode>((arg >> 4) & 3);
  poly_params.render_back_side = arg & 64;
  poly_params.render_front_side = arg & 128;
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

  texture_params.raw_value = arg;
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

  material.diffuse = Color4::FromRGB555(arg & 0x7FFF);
  material.ambient = Color4::FromRGB555(arg >> 16);

  if (arg & 0x8000) {
    vertex_color = material.diffuse;
  }
}

void GPU::CMD_SetMaterialColor1() {
  auto arg = Dequeue().argument;

  material.specular = Color4::FromRGB555(arg & 0x7FFF);
  material.emissive = Color4::FromRGB555(arg >> 16);

  material.enable_shinyness_table = arg & 0x8000;
}

void GPU::CMD_SetLightVector() {
  auto arg = Dequeue().argument;

  auto& light = lights[arg >> 30];

  auto direction = Vector4<Fixed20x12>{
    s16(((arg >>  0) & 0x3FF) << 6) >> 3,
    s16(((arg >> 10) & 0x3FF) << 6) >> 3,
    s16(((arg >> 20) & 0x3FF) << 6) >> 3,
    atom::NumericConstants<Fixed20x12>::Zero()
  };

  light.direction = (this->direction.current * direction).XYZ();
  
  // halfway = (direction + (0, 0, -1)) * 0.5
  light.halfway = Vector3<Fixed20x12>{
    light.direction.X().raw() >> 1,
    light.direction.Y().raw() >> 1,
   (light.direction.Z() - atom::NumericConstants<Fixed20x12>::One()).raw() >> 1
  };
}

void GPU::CMD_SetLightColor() {
  auto arg = Dequeue().argument;

  lights[arg >> 30].color = Color4::FromRGB555(arg & 0x7FFF);
}

void GPU::CMD_BeginVertexList() {
  auto arg = Dequeue().argument;
  in_vertex_list = true;
  is_quad = arg & 1;
  is_strip = arg & 2;
  is_first = true;
  polygon_strip_length = 0;

  current_vertex_list.clear();
}

void GPU::CMD_EndVertexList() {
  Dequeue();
  // TODO: allegedly this command is a no-operation on the DS...
  in_vertex_list = false;
}

void GPU::CMD_SwapBuffers() {
  auto arg = Dequeue().argument;
  manual_translucent_y_sorting_pending = arg & 1;
  use_w_buffer_pending = arg & 2;
  swap_buffers_pending = true;
}

void GPU::CheckGXFIFO_IRQ() {
  switch (gxstat.cmd_fifo_irq) {
    case GXSTAT::IRQMode::Never:
    case GXSTAT::IRQMode::Reserved: {
      break;
    }
    case GXSTAT::IRQMode::LessThanHalfFull: {
      if (gxfifo.Count() < 128) {
        irq9.Raise(IRQ::Source::GXFIFO);
      }
      break;
    }
    case GXSTAT::IRQMode::Empty: {
      if (gxfifo.IsEmpty()) {
        irq9.Raise(IRQ::Source::GXFIFO);
      }
      break;
    }
  }
}

void GPU::UpdateClipMatrix() {
  clip_matrix = projection.current * modelview.current;
}

} // namespace lunar::nds

