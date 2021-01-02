/*
 * Copyright (C) 2020 fleroviux
 */

#include <common/log.hpp>

#include "gpu.hpp"

namespace fauxDS::core {

static constexpr int kCmdNumParams[256] {
  // 0x00 - 0x0F (all NOPs)
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

  // 0x10 - 0x1F (Matrix engine)
  1, 0, 1, 1, 1, 0, 16, 12, 16, 12, 9, 3, 3, 0, 0, 0,
  
  // 0x20 - 0x2F (Vertex and polygon attributes, mostly)
  1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
  
  // 0x30 - 0x3F (Material / lighting properties)
  1, 1, 1, 1, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  
  // 0x40 - 0x4F (Begin/end vertex)
  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  
  // 0x50 - 0x5F (Swap buffers)
  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  
  // 0x60 - 0x6F (Set viewport)
  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  
  // 0x70 - 0x7F (Box, position and vector test)
  3, 2, 1
};

void GPU::Reset() {
  disp3dcnt = {};
  // FIXME
  //gxstat = {};
  gxfifo.Reset();
  gxpipe.Reset();
  packed_cmds = 0;
  packed_args_left = 0;
  
  in_vertex_list = false;
  vertex = {};
  polygon = {};
  position_old = {};

  matrix_mode = MatrixMode::Projection;
  projection.Reset();
  modelview.Reset();
  direction.Reset();
  texture.Reset();
  
  for (uint i = 0; i < 256 * 192; i++)
    output[i] = 0x8000;
}

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
    LOG_ERROR("GPU: unknown command port 0x{0:03X}", port);
    return;
  }

  Enqueue({ static_cast<u8>(port >> 2), value });
}

void GPU::Enqueue(CmdArgPack pack) {
  if (gxfifo.IsEmpty() && !gxpipe.IsFull()) {
    gxpipe.Write(pack);
  } else {
    gxfifo.Write(pack);
    dma9.SetGXFIFOHalfEmpty(gxfifo.Count() < 128);
  }

  ProcessCommands();
}

auto GPU::Dequeue() -> CmdArgPack {
  ASSERT(gxpipe.Count() != 0, "GPU: attempted to dequeue entry from empty GXPIPE");

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
      case 0x15: CMD_LoadIdentity(); break;
      case 0x16: CMD_LoadMatrix4x4(); break;
      case 0x17: CMD_LoadMatrix4x3(); break;
      case 0x18: CMD_MatrixMultiply4x4(); break;
      case 0x19: CMD_MatrixMultiply4x3(); break;
      case 0x1A: CMD_MatrixMultiply3x3(); break;
      case 0x1B: CMD_MatrixScale(); break;
      case 0x1C: CMD_MatrixTranslate(); break;
      
      case 0x20: CMD_SetColor(); break;
      case 0x22: CMD_SetUV(); break;
      case 0x23: CMD_SubmitVertex_16(); break;
      case 0x24: CMD_SubmitVertex_10(); break;
      case 0x25: CMD_SubmitVertex_XY(); break;
      case 0x26: CMD_SubmitVertex_XZ(); break;
      case 0x27: CMD_SubmitVertex_YZ(); break;
      case 0x28: CMD_SubmitVertex_Offset(); break;
      case 0x2A: CMD_SetTextureParameters(); break;
      case 0x2B: CMD_SetPaletteBase(); break;

      case 0x40: CMD_BeginVertexList(); break;
      case 0x41: CMD_EndVertexList(); break;

      case 0x50: CMD_SwapBuffers(); break;

      default:
        LOG_ERROR("GPU: unimplemented command 0x{0:02X}", command);
        Dequeue();
        for (int i = 1; i < arg_count; i++)
          Dequeue();
        break;
    }

    // Fake the amount of time it takes to process the command.
    gxstat.gx_busy = true;
    scheduler.Add(9, [this](int cycles_late) {
      gxstat.gx_busy = false;
      ProcessCommands();
    });
  }
}

void GPU::AddVertex(Vector4 const& position) {
  if (!in_vertex_list) {
    LOG_ERROR("GPU: cannot submit vertex data outside of VTX_BEGIN / VTX_END");
    return;
  }

  if (vertex.count == 6144) {
    // TODO: set buffer overflow bit in GXSTAT.
    LOG_ERROR("GPU: submitted more vertices than fit into vertex RAM.");
    return;
  }

  if (polygon.count == 2048) {
    // TODO: set buffer overflow bit in GXSTAT.
    LOG_ERROR("GPU: submitted more polygons than fit into polygon RAM.");
    return;
  }

  int num_vertices = is_quad ? 4 : 3;

  position_old = position;

  auto clip_position = projection.current * (modelview.current * position);
  auto index = vertex.count++;
  vertex.data[index] = {
    clip_position,
    { vertex_color[0], vertex_color[1], vertex_color[2] },
    { vertex_uv[0], vertex_uv[1] }
  };

  ++vertex_counter;

  if (vertex_counter >= num_vertices && (!is_quad || (vertex_counter % 2) == 0)) {
    auto& poly = polygon.data[polygon.count++];

    poly.texture_params = texture_params;

    int start = index - num_vertices + 1;
    int end = index; // alias for legibility
    int indices[3] { end, start, start + 1 };

    poly.count = 0;

    // Clip each vertex against the sides of the view frustum.
    for (int i = 0; i < num_vertices; i++) {
      auto& v = vertex.data[indices[1]];
      auto& vb = vertex.data[indices[0]];
      auto& vn = vertex.data[indices[2]];

      poly.indices[poly.count++] = indices[1];

      bool rearrange_top_down = is_quad && is_strip && i >= 1;

      for (int j = 0; j < 3; j++) {
        if (rearrange_top_down) {
          if (i == 1) indices[j] += 2;
          if (i == 2) indices[j] -= 1;
        } else {
          indices[j]++;
        }

        // Index to the current vertex will never wraparound.
        if (j != 1) {
          if (indices[j] < 0) {
            indices[j] += num_vertices;
          } else if (indices[j] > end) {
            indices[j] -= num_vertices;
          }
        }
      }

      auto w = v.position[3];

      // TODO: find the lowest distance instead of clipping immediately?
      for (int j = 0; j < 3; j++) {
        auto value = v.position[j];

        if (false) {//value < -w || value > w) {
          Vector4 edge_a {
            v.position[0] - vb.position[0],
            v.position[1] - vb.position[1],
            v.position[2] - vb.position[2],
            v.position[3] - vb.position[3]
          };          

          Vector4 edge_b {
            v.position[0] - vn.position[0],
            v.position[1] - vn.position[1],
            v.position[2] - vn.position[2],
            v.position[3] - vn.position[3]
          };

          s64 scale_a;
          s64 scale_b;

          if (value < w) {
            scale_a = -(s64(vb.position[j] + vb.position[3]) << 32) / (edge_a[3] + edge_a[j]);
            scale_b = -(s64(vn.position[j] + vn.position[3]) << 32) / (edge_b[3] + edge_b[j]);
          } else {
            scale_a =  (s64(vb.position[j] - vb.position[3]) << 32) / (edge_a[3] - edge_a[j]);
            scale_b =  (s64(vn.position[j] - vn.position[3]) << 32) / (edge_b[3] - edge_b[j]);
          }

          v.position = Vector4{
            vb.position[0] + s32((edge_a[0] * scale_a) >> 32),
            vb.position[1] + s32((edge_a[1] * scale_a) >> 32),
            vb.position[2] + s32((edge_a[2] * scale_a) >> 32),
            vb.position[3] + s32((edge_a[3] * scale_a) >> 32)
          };

          if (vertex.count == 6144) {
            // TODO: set buffer overflow bit in GXSTAT.
            LOG_ERROR("GPU: submitted more vertices than fit into vertex RAM.");
            break;
          }

          auto new_vertex_id = vertex.count++;
          vertex.data[new_vertex_id] = {
            Vector4{
              vn.position[0] + s32((edge_b[0] * scale_b) >> 32),
              vn.position[1] + s32((edge_b[1] * scale_b) >> 32),
              vn.position[2] + s32((edge_b[2] * scale_b) >> 32),
              vn.position[3] + s32((edge_b[3] * scale_b) >> 32)
            },
            { v.color[0], v.color[1], v.color[2] },
            { v.uv[0], v.uv[1] }
          };

          // TODO: assert if the 10 vertices limit is hit...
          // theoretically it should not happen but practically i'm an idiot.
          poly.indices[poly.count++] = new_vertex_id;
          break;
        }
      }
    }

    // In strip mode we just keep reusing old vertices.
    if (!is_strip) {
      vertex_counter = 0;
    }
  }
}

void GPU::CheckGXFIFO_IRQ() {
  switch (gxstat.cmd_fifo_irq) {
    case GXSTAT::IRQMode::Never:
    case GXSTAT::IRQMode::Reserved:
      break;
    case GXSTAT::IRQMode::LessThanHalfFull:
      if (gxfifo.Count() < 128)
        irq9.Raise(IRQ::Source::GXFIFO);
      break;
    case GXSTAT::IRQMode::Empty:
      if (gxfifo.IsEmpty())
        irq9.Raise(IRQ::Source::GXFIFO);
      break;
  }
}

auto GPU::DISP3DCNT::ReadByte(uint offset) -> u8 {
  switch (offset) {
    case 0:
      return (enable_textures ? 1 : 0) |
             (static_cast<u8>(shading_mode) << 1) |
             (enable_alpha_test  ? 4 : 0) |
             (enable_alpha_blend ? 8 : 0) |
             (enable_antialias  ? 16 : 0) |
             (static_cast<u8>(fog_mode) << 6) |
             (enable_fog ? 128 : 0);
    case 1:
      return fog_depth_shift |
            (rdlines_underflow     ? 16 : 0) |
            (polyvert_ram_overflow ? 32 : 0) |
            (enable_rear_bitmap    ? 64 : 0);
  }
  
  UNREACHABLE;
}

void GPU::DISP3DCNT::WriteByte(uint offset, u8 value) {
  switch (offset) {
    case 0:
      enable_textures = value & 1;
      shading_mode = static_cast<Shading>((value >> 1) & 1);
      enable_alpha_test = value & 4;
      enable_alpha_blend = value & 8;
      enable_antialias = value & 16;
      enable_edge_marking = value & 32;
      fog_mode = static_cast<FogMode>((value >> 6) & 1);
      enable_fog = value & 128;
      break;
    case 1:
      fog_depth_shift = value & 0xF;
      enable_rear_bitmap = value & 64;
      break;
    default:
      UNREACHABLE;
  }
}

auto GPU::GXSTAT::ReadByte(uint offset) -> u8 {
  switch (offset) {
    case 0:
      return 0;
    case 1:
      return 0;
    case 2:
      return gpu.gxfifo.Count();
    case 3:
      return (gpu.gxfifo.IsFull() ? 1 : 0) |
             (gpu.gxfifo.Count() < 128 ? 2 : 0) |
             (gpu.gxfifo.IsEmpty() ? 4 : 0) |
             (gx_busy ? 8 : 0) |
             (static_cast<u8>(cmd_fifo_irq) << 6);
  }
  
  UNREACHABLE;
}

void GPU::GXSTAT::WriteByte(uint offset, u8 value) {
  auto cmd_fifo_irq_old = cmd_fifo_irq;

  switch (offset) {
    case 0:
      break;
    case 1:
      break;
    case 2:
      break;
    case 3:
      cmd_fifo_irq = static_cast<IRQMode>(value >> 6);
      // TODO: confirm that the GXFIFO IRQ indeed can be triggered by this.
      if (cmd_fifo_irq != cmd_fifo_irq_old)
        gpu.CheckGXFIFO_IRQ();
      break;
    default:
      UNREACHABLE;
  }
}

} // namespace fauxDS::core
