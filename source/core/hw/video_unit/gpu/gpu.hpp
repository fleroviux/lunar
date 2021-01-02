/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <common/integer.hpp>
#include <common/fifo.hpp>
#include <core/hw/dma/dma9.hpp>
#include <core/hw/irq/irq.hpp>
#include <core/hw/video_unit/vram.hpp>
#include <core/scheduler.hpp>
#include <stack>

#include "matrix_stack.hpp"

namespace fauxDS::core {

/// 3D graphics processing unit (GPU)
struct GPU {
  GPU(Scheduler& scheduler, IRQ& irq9, DMA9& dma9, VRAM const& vram)
      : scheduler(scheduler)
      , irq9(irq9)
      , dma9(dma9)
      , vram_texture(vram.region_gpu_texture)
      , vram_palette(vram.region_gpu_palette) {
    Reset();
  }
  
  void Reset();
  void WriteGXFIFO(u32 value);
  void WriteCommandPort(uint port, u32 value);
  auto GetOutput() -> u16 const* { return &output[0]; }

  struct DISP3DCNT {
    auto ReadByte (uint offset) -> u8;
    void WriteByte(uint offset, u8 value);
    
  private:
    friend struct fauxDS::core::GPU;
    
    enum class Shading {
      Toon = 0,
      Highlight = 1
    };
    
    enum class FogMode {
      ColorAndAlpha = 0,
      Alpha = 1
    };
  
    bool enable_textures = false;
    Shading shading_mode = Shading::Toon;
    bool enable_alpha_test = false;
    bool enable_alpha_blend = false;
    bool enable_antialias = false;
    bool enable_edge_marking = false;
    FogMode fog_mode = FogMode::ColorAndAlpha;
    bool enable_fog = false;
    int fog_depth_shift = 0;
    bool rdlines_underflow = false;
    bool polyvert_ram_overflow = false;
    bool enable_rear_bitmap = false;
  } disp3dcnt;
  
  struct GXSTAT {
    // TODO: implement (box)test and matrix stack bits.

    GXSTAT(GPU& gpu) : gpu(gpu) {}
        
    auto ReadByte (uint offset) -> u8;
    void WriteByte(uint offset, u8 value);
    
  private:
    friend struct fauxDS::core::GPU;
    
    enum class IRQMode {
      Never = 0,
      LessThanHalfFull = 1,
      Empty = 2,
      Reserved = 3
    };
    
    bool gx_busy = false;
    IRQMode cmd_fifo_irq = IRQMode::Never;

    GPU& gpu;
  } gxstat { *this };

private:
  enum class MatrixMode {
    Projection = 0,
    Modelview = 1,
    Simultaneous = 2,
    Texture = 3
  };
  
  struct CmdArgPack {
    u8 command = 0;
    u32 argument = 0;
  };

  struct TextureParams {
    u32 address;
    bool repeat_u;
    bool repeat_v;
    bool flip_u;
    bool flip_v;
    int size_u;
    int size_v;

    enum class Format {
      None,
      A3I5,
      Palette2BPP,
      Palette4BPP,
      Palette8BPP,
      Compressed4x4,
      A5I3,
      Direct
    } format = Format::None;

    bool color0_transparent;

    enum class Transform {
      TexCoord,
      Normal,
      Vertex
    } transform;

    u16 palette_base;
  };

  struct Vertex {
    Vector4 position;
    s32 color[3];
    s16 uv[2];
    // ...
  };

  struct Polygon {
    int count;
    int indices[4];
    TextureParams texture_params;
  };

  void Enqueue(CmdArgPack pack);
  auto Dequeue() -> CmdArgPack;
  void ProcessCommands();
  void CheckGXFIFO_IRQ();
  
  auto DequeueMatrix4x4() -> Matrix4x4 {
    Matrix4x4 mat;
    for (int col = 0; col < 4; col++) {
      for (int row = 0; row < 4; row++) {
        mat[col][row] = Dequeue().argument;
      }
    }
    return mat;
  }
  
  auto DequeueMatrix4x3() -> Matrix4x4 {
    Matrix4x4 mat;
    for (int col = 0; col < 4; col++) {
      for (int row = 0; row < 3; row++) {
        mat[col][row] = Dequeue().argument;
      }
    }
    mat[3][3] = 0x1000;
    return mat;
  }

  auto DequeueMatrix3x3() -> Matrix4x4 {
    Matrix4x4 mat;
    for (int col = 0; col < 3; col++) {
      for (int row = 0; row < 3; row++) {
        mat[col][row] = Dequeue().argument;
      }
    }
    mat[3][3] = 0x1000;
    return mat;
  }

  void AddVertex(Vector4 const& position);

  /// Matrix commands
  void CMD_SetMatrixMode();
  void CMD_PushMatrix();
  void CMD_PopMatrix();
  void CMD_StoreMatrix();
  void CMD_RestoreMatrix();
  void CMD_LoadIdentity();
  void CMD_LoadMatrix4x4();
  void CMD_LoadMatrix4x3();
  void CMD_MatrixMultiply4x4();
  void CMD_MatrixMultiply4x3();
  void CMD_MatrixMultiply3x3();
  void CMD_MatrixScale();
  void CMD_MatrixTranslate();

  /// Vertex submission commands
  void CMD_SetColor();
  void CMD_SetNormal();
  void CMD_SetUV();
  void CMD_SubmitVertex_16();
  void CMD_SubmitVertex_10();
  void CMD_SubmitVertex_XY();
  void CMD_SubmitVertex_XZ();
  void CMD_SubmitVertex_YZ();
  void CMD_SubmitVertex_Offset();
  void CMD_SetTextureParameters();
  void CMD_SetPaletteBase();
  
  /// Vertex lists
  void CMD_BeginVertexList();
  void CMD_EndVertexList();

  void CMD_SwapBuffers();

  bool in_vertex_list;
  bool is_quad;
  bool is_strip;
  int vertex_counter;

  /// Vertex RAM
  struct {
    int count = 0;
    Vertex data[6144];
  } vertex;

  /// Polygon RAM
  struct {
    int count = 0;
    Polygon data[2048];
  } polygon;

  /// Untransformed vertex from the previous vertex submission command.
  Vector4 position_old;

  /// Current vertex color
  s32 vertex_color[3];

  /// Current vertex UV
  s16 vertex_uv[2];

  /// Current texture parameters
  TextureParams texture_params;

  /// GPU texture and texture palette data
  Region<4, 131072> const& vram_texture { 3 };
  Region<8> const& vram_palette { 7 };

  Scheduler& scheduler;
  IRQ& irq9;
  DMA9& dma9;
  common::FIFO<CmdArgPack, 256> gxfifo;
  common::FIFO<CmdArgPack, 4> gxpipe;
  
  u16 output[256 * 192];
  // FIXME: what precision is the depth buffer actually?
  s32 depthbuffer[256 * 192];

  /// Packed command processing
  u32 packed_cmds;
  int packed_args_left;
  
  /// Matrix stacks
  MatrixMode matrix_mode;
  MatrixStack< 1> projection;
  MatrixStack<31> modelview;
  MatrixStack<31> direction;
  MatrixStack< 1> texture;
};

} // namespace fauxDS::core
