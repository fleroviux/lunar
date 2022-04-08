/*
 * Copyright (C) 2020 fleroviux
 */

#include <util/log.hpp>

#include "gpu.hpp"

namespace Duality::Core {

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

GPU::GPU(Scheduler& scheduler, IRQ& irq9, DMA9& dma9, VRAM const& vram)
    : scheduler(scheduler)
    , irq9(irq9)
    , dma9(dma9)
    , vram_texture(vram.region_gpu_texture)
    , vram_palette(vram.region_gpu_palette) {
  Reset();
}

void GPU::Reset() {
  disp3dcnt = {};
  // FIXME
  //gxstat = {};
  gxfifo.Reset();
  gxpipe.Reset();
  packed_cmds = 0;
  packed_args_left = 0;
  alpha_test_ref = {};
  clear_color = {};
  clear_depth = {};
  clrimage_offset = {};
  
  for (int  i = 0; i < 2; i++) {
    vertex[i] = {};
    polygon[i] = {};
  }
  gx_buffer_id = 0;
  
  in_vertex_list = false;
  position_old = {};
  vertices.clear();

  for (int i = 0; i < 4; i++) {
    lights[i] = {};
  }
  material = {};
  toon_table.fill(0x7FFF);
  edge_color_table.fill(0x7FFF);

  matrix_mode = MatrixMode::Projection;
  projection.Reset();
  modelview.Reset();
  direction.Reset();
  texture.Reset();
  clip_matrix.identity();
  
  for (uint i = 0; i < 256 * 192; i++) {
    color_buffer[i] = {};  
  }

  use_w_buffer = false;
  use_w_buffer_pending = false;
  swap_buffers_pending = false;
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

void GPU::WriteToonTable(uint offset, u8 value) {
  auto index = offset >> 1;
  auto shift = (offset & 1) * 8;

  toon_table[index] = (toon_table[index] & ~(0xFF << shift)) | (value << shift);
}

void GPU::WriteEdgeColorTable(uint offset, u8 value) {
  auto index = offset >> 1;
  auto shift = (offset & 1) * 8;

  edge_color_table[index] = (edge_color_table[index] & ~(0xFF << shift)) | (value << shift);
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

      default:
        LOG_ERROR("GPU: unimplemented command 0x{0:02X}", command);
        Dequeue();
        for (int i = 1; i < arg_count; i++)
          Dequeue();
        break;
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

void GPU::SwapBuffers() {
  if (swap_buffers_pending) {
    gx_buffer_id ^= 1;
    vertex[gx_buffer_id].count = 0;
    polygon[gx_buffer_id].count = 0;
    use_w_buffer = use_w_buffer_pending;
    swap_buffers_pending = false;
    ProcessCommands();
  }
}

void GPU::AddVertex(Vector4<Fixed20x12> const& position) {
  if (!in_vertex_list) {
    LOG_ERROR("GPU: cannot submit vertex data outside of VTX_BEGIN / VTX_END");
    return;
  }

  if (texture_params.transform == TextureParams::Transform::Position) {
    auto const& matrix = texture.current;

    auto x = position.x();
    auto y = position.y();
    auto z = position.z();

    auto t_x = Fixed20x12{vertex_uv_source.x().raw() << 12};
    auto t_y = Fixed20x12{vertex_uv_source.y().raw() << 12};

    vertex_uv = Vector2<Fixed12x4>{
      s16((x * matrix[0].x() + y * matrix[1].x() + z * matrix[2].x() + t_x).raw() >> 12),
      s16((x * matrix[0].y() + y * matrix[1].y() + z * matrix[2].y() + t_y).raw() >> 12)
    };
  }

  auto clip_position = clip_matrix * position;

  vertices.push_back({ clip_position, vertex_color, vertex_uv });

  position_old = position;

  int required = is_quad ? 4 : 3;
  if (is_strip && !is_first) {
    required -= 2;
  }

  if (vertices.size() == required) {
    // FIXME: this is disgusting.
    if (polygon[gx_buffer_id].count == 2048) {
      vertices.clear();
      return;
    }

    auto& poly = polygon[gx_buffer_id].data[polygon[gx_buffer_id].count];

    // Determine if the polygon must be clipped.
    bool needs_clipping = false;
    for (auto const& v : vertices) {
      auto w = v.position[3];
      for (int i = 0; i < 3; i++) {
        if (v.position[i] < -w || v.position[i] > w) {
          needs_clipping = true;
          break;
        }
      }
    }

    // In a triangle strip the winding order of each second polygon is inverted.
    bool invert_winding = is_strip && !is_quad && (polygon_strip_length % 2) == 1;

    if (needs_clipping) {
      poly.count = 0;

      // TODO: this isn't going to cut it efficiency-wise.
      if (is_strip && !is_first) {
        vertices.insert(vertices.begin(), vertex[gx_buffer_id].data[vertex[gx_buffer_id].count - 1]);
        vertices.insert(vertices.begin(), vertex[gx_buffer_id].data[vertex[gx_buffer_id].count - 2]);
      }

      auto const& v0 = vertices[0].position;
      auto const& v1 = vertices[vertices.size() - 1].position;
      auto const& v2 = vertices[1].position;
      bool front_facing = IsFrontFacing(v0, v1, v2, invert_winding);

      if ((front_facing && poly_params.render_front_side) || (!front_facing && poly_params.render_back_side)) {
        for (auto const& v : ClipPolygon(vertices, is_quad && is_strip)) {
          // FIXME: this is disgusting.
          if (vertex[gx_buffer_id].count == 6144) {
            LOG_ERROR("GPU: submitted more vertices than fit into Vertex RAM.");
            break;
          }
          auto index = vertex[gx_buffer_id].count++;
          vertex[gx_buffer_id].data[index] = v;
          poly.indices[poly.count++] = index;
        }
      }

      // Restart polygon strip based on the last two unclipped vertifces.
      if (is_strip) {
        is_first = true;
        vertices.erase(vertices.begin());
        if (is_quad) {
          vertices.erase(vertices.begin());
        }
      } else {
        vertices.clear();
      }
    } else {
      bool front_facing;

      if (is_strip && !is_first) {
        poly.indices[0] = vertex[gx_buffer_id].count - 2;
        poly.indices[1] = vertex[gx_buffer_id].count - 1;
        poly.count = 2;

        // TODO: make this a bit nicer.
        auto const& v0 = vertex[gx_buffer_id].data[poly.indices[0]].position;
        auto const& v1 = vertices[vertices.size() - 1].position;
        auto const& v2 = vertex[gx_buffer_id].data[poly.indices[1]].position;

        front_facing = IsFrontFacing(v0, v1, v2, invert_winding);
      } else {
        poly.count = 0;

        auto const& v0 = vertices[0].position;
        auto const& v1 = vertices[vertices.size() - 1].position;
        auto const& v2 = vertices[1].position;

        front_facing = IsFrontFacing(v0, v1, v2, invert_winding);
      }

      if ((front_facing && poly_params.render_front_side) || (!front_facing && poly_params.render_back_side)) {
        for (auto const& v : vertices) {
          // FIXME: this is disgusting.
          if (vertex[gx_buffer_id].count == 6144) {
            LOG_ERROR("GPU: submitted more vertices than fit into Vertex RAM.");
            break;
          }
          auto index = vertex[gx_buffer_id].count++;
          vertex[gx_buffer_id].data[index] = v;
          poly.indices[poly.count++] = index;
        }
      } else {
        poly.count = 0;

        // Keep last up to two vertices in vertex RAM to continue the polygon strip.
        // TODO: how does hardware handle this case?
        if (is_strip) {
          for (int i = vertices.size() - ((is_quad || is_first) ? 2 : 1); i < vertices.size(); i++) {
            // FIXME: this is disgusting.
            if (vertex[gx_buffer_id].count == 6144) {
              LOG_ERROR("GPU: submitted more vertices than fit into Vertex RAM.");
              break;
            }
            auto index = vertex[gx_buffer_id].count++;
            vertex[gx_buffer_id].data[index] = vertices[i];
            poly.indices[poly.count++] = index;
          }
        }
      }

      is_first = false;
      vertices.clear();

      if (is_quad && is_strip) {
        std::swap(poly.indices[2], poly.indices[3]);
      }
    }

    if (is_strip) {
      polygon_strip_length++;
    }

    if (poly.count != 0) {
      polygon[gx_buffer_id].count++;
      poly.params = poly_params;
      poly.texture_params = texture_params;
    }
  }
}

bool GPU::IsFrontFacing(Vector4<Fixed20x12> const& v0, Vector4<Fixed20x12> const& v1, Vector4<Fixed20x12> const& v2, bool invert) {
  // auto normal = (v1.xyz() - v0.xyz()).cross(v2.xyz() - v0.xyz());
  // auto dot = v0.xyz().dot(normal);

  float a[3] {
    (v1.x() - v0.x()).raw() / (float)(1 << 12),
    (v1.y() - v0.y()).raw() / (float)(1 << 12),
    (v1.z() - v0.z()).raw() / (float)(1 << 12)
  };

  float b[3] {
    (v2.x() - v0.x()).raw() / (float)(1 << 12),
    (v2.y() - v0.y()).raw() / (float)(1 << 12),
    (v2.z() - v0.z()).raw() / (float)(1 << 12)
  };

  float normal[3] {
    a[1] * b[2] - a[2] * b[1],
    a[2] * b[0] - a[0] * b[2],
    a[0] * b[1] - a[1] * b[0]
  };

  float dot = (v0.x().raw() / (float)(1 << 12)) * normal[0] +
              (v0.y().raw() / (float)(1 << 12)) * normal[1] +
              (v0.z().raw() / (float)(1 << 12)) * normal[2];

  if (invert) {
    return dot >= 0; 
  }

  return dot < 0;
}

auto GPU::ClipPolygon(std::vector<Vertex> const& vertices, bool quadstrip) -> std::vector<Vertex> {
  std::vector<Vertex> clipped[2];

  clipped[0] = vertices;

  if (quadstrip) {
    std::swap(clipped[0][2], clipped[0][3]);
  }

  struct CompareLt {
    bool operator()(Fixed20x12 x, Fixed20x12 w) { return x < -w; }
  };

  struct CompareGt {
    bool operator()(Fixed20x12 x, Fixed20x12 w) { return x >  w; }
  };

  if (!poly_params.render_far_plane_polys && ClipPolygonOnPlane<2, CompareGt>(clipped[0], clipped[1])) {
    return {};
  }
  clipped[0].clear();
  ClipPolygonOnPlane<2, CompareLt>(clipped[1], clipped[0]);
  clipped[1].clear();

  ClipPolygonOnPlane<1, CompareGt>(clipped[0], clipped[1]);
  clipped[0].clear();
  ClipPolygonOnPlane<1, CompareLt>(clipped[1], clipped[0]);
  clipped[1].clear();

  ClipPolygonOnPlane<0, CompareGt>(clipped[0], clipped[1]);
  clipped[0].clear();
  ClipPolygonOnPlane<0, CompareLt>(clipped[1], clipped[0]);
  // clipped[1].clear();

  return clipped[0];
}

template<int axis, typename Comparator>
bool GPU::ClipPolygonOnPlane(std::vector<Vertex> const& vertices_in, std::vector<Vertex>& vertices_out) {
  auto size = vertices_in.size();
  bool clipped = false;

  for (int i = 0; i < size; i++) {
    auto& v0 = vertices_in[i];

    if (Comparator{}(v0.position[axis], v0.position.w())) {  
      int a = i - 1;
      int b = i + 1;
      if (a == -1) a = size - 1;
      if (b == size) b = 0;

      // TODO: can a point be generated in both iterations at the same time?
      for (int j : { a, b }) {
        auto& v1 = vertices_in[j];

        if (!Comparator{}(v1.position[axis], v1.position.w())) {
          auto sign  = Fixed20x12::from_int((v0.position[axis] < -v0.position.w()) ? 1 : -1);
          auto numer = v1.position[axis] + sign * v1.position[3];
          auto denom = (v0.position.w() - v1.position.w()) + (v0.position[axis] - v1.position[axis]) * sign;
          auto scale = -sign * numer / denom;

          vertices_out.push_back({
            .position = Vector4<Fixed20x12>::interpolate(v1.position, v0.position, scale),
            .color = Color4::interpolate(v1.color, v0.color, scale),
            .uv = Vector2<Fixed12x4>::interpolate(v1.uv, v0.uv, scale)
          });
        }
      }

      clipped = true;
    } else {
      vertices_out.push_back(v0);
    }
  }

  return clipped;
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

void GPU::UpdateClipMatrix() {
  clip_matrix = projection.current * modelview.current;
}

auto GPU::DISP3DCNT::ReadByte(uint offset) -> u8 {
  switch (offset) {
    case 0:
      return (enable_textures ? 1 : 0) |
             (static_cast<u8>(shading_mode) << 1) |
             (enable_alpha_test   ?  4 : 0) |
             (enable_alpha_blend  ?  8 : 0) |
             (enable_antialias    ? 16 : 0) |
             (enable_edge_marking ? 32 : 0) |
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
      // TODO: do not hardcode the boxtest result to be true.
      return 2;
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

void GPU::AlphaTest::WriteByte(u8 value) {
  alpha = value;
}

void GPU::ClearColor::WriteByte(uint offset, u8 value) {
  switch (offset) {
    case 0: {
      color_r = (value >>  0) & 31;
      color_g = (value >>  5) & 31;
      color_b = (value >> 10) & 31;
      enable_fog = value & 128;
      break;
    }
    case 1: {
      color_a = value & 31;
      polygon_id = (value >> 8) & 63;
      break;
    }
    default: {
      UNREACHABLE;
    }
  }
}

void GPU::ClearDepth::WriteByte(uint offset, u8 value) {
  switch (offset) {
    case 0: {
      depth = (depth & 0xFF00) | value;
      break;
    }
    case 1: {
      depth = ((depth & 0xFF) | (value << 8)) & 0x7FFF;
      break;
    }
    default: {
      UNREACHABLE;
    }
  }
}

void GPU::ClearImageOffset::WriteByte(uint offset, u8 value) {
  switch (offset) {
    case 0: {
      x = value;
      break;
    }
    case 1: {
      y = value;
      break;
    }
    default: {
      UNREACHABLE;
    }
  }
}

} // namespace Duality::Core
