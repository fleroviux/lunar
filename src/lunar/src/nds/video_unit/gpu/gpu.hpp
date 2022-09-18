/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <array>
#include <atomic>
#include <condition_variable>
#include <lunar/integer.hpp>
#include <memory>
#include <mutex>
#include <thread>

#include "common/fifo.hpp"
#include "common/meta.hpp"
#include "common/punning.hpp"
#include "common/scheduler.hpp"
#include "common/static_vec.hpp"
#include "nds/arm9/dma/dma.hpp"
#include "nds/irq/irq.hpp"
#include "nds/video_unit/vram.hpp"
#include "color.hpp"
#include "matrix_stack.hpp"
#include "renderer/renderer_base.hpp"

namespace lunar::nds {

// 3D graphics processing unit (GPU)
struct GPU {
  GPU(
    Scheduler& scheduler,
    IRQ& irq9,
    DMA9& dma9,
    VRAM const& vram
  );

 ~GPU();
  
  void Reset();
  void WriteGXFIFO(u32 value);
  void WriteCommandPort(uint port, u32 value);
  void WriteToonTable(uint offset, u8 value);
  void WriteEdgeColorTable(uint offset, u8 value);
  void SwapBuffers();
  void WaitForRenderWorkers();

  template<typename T>
  auto ReadClipMatrix(u32 offset) -> T {
    static_assert(is_one_of_v<T, u8, u16, u32>, "T must be u8, u16 or u32");
    if (offset >= 64) {
      UNREACHABLE;
    }
    auto row = (offset >> 2) & 3;
    auto col =  offset >> 4;
    return static_cast<T>(clip_matrix[col][row].raw() >> ((offset & 3) * 8));
  }

  void Render();
  auto GetOutput() -> Color4 const* { return &color_buffer[0]; }

  struct DISP3DCNT {
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

    auto ReadByte (uint offset) -> u8;
    void WriteByte(uint offset, u8 value);
  } disp3dcnt;

  struct GXSTAT {
    // TODO: implement (box)test and matrix stack bits.
    GXSTAT(GPU& gpu) : gpu(gpu) {}
        
    auto ReadByte (uint offset) -> u8;
    void WriteByte(uint offset, u8 value);
    
  private:
    friend struct lunar::nds::GPU;
    
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

  struct AlphaTest {
    int alpha = 0;

    void WriteByte(u8 value);
  } alpha_test_ref;

  struct ClearColor {
    int color_r = 0;
    int color_g = 0;
    int color_b = 0;
    int color_a = 0;
    int polygon_id = 0;
    bool enable_fog = false;

    void WriteByte(uint offset, u8 value);
  } clear_color;

  struct ClearDepth {
    u16 depth = 0;

    void WriteByte(uint offset, u8 value);
  } clear_depth;

  struct ClearImageOffset {
    int x = 0;
    int y = 0;

    void WriteByte(uint offset, u8 value);
  } clrimage_offset;

//private:
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

  struct PolygonParams {
    bool enable_light[4] { false };
  
    enum class Mode {
      Modulation,
      Decal,
      Shaded,
      Shadow
    } mode = Mode::Modulation;

    bool render_back_side = false;
    bool render_front_side = false;
    bool enable_translucent_depth_write = false; 
    bool render_far_plane_polys = false;
    bool render_1dot_depth_tested = false;

    enum class DepthTest {
      Less,
      Equal
    } depth_test = DepthTest::Less;

    bool enable_fog = false;
    int alpha = 0;
    int polygon_id = 0;
  };

  struct TextureParams {
    u32 raw_value = 0;

    u32 address = 0;
    bool repeat[2] { false };
    bool flip[2] { false };
    int size[2] { 0 };

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

    bool color0_transparent = false;

    enum class Transform {
      None,
      TexCoord,
      Normal,
      Position
    } transform = Transform::TexCoord;

    u16 palette_base = 0;
  };

  struct Vertex {
    Vector4<Fixed20x12> position;
    Color4 color;
    Vector2<Fixed12x4> uv;
  };

  struct Polygon {
    int count;
    Vertex* vertices[10];
    PolygonParams params;
    TextureParams texture_params;
  };

  void Enqueue(CmdArgPack pack);
  auto Dequeue() -> CmdArgPack;
  void ProcessCommands();
  void CheckGXFIFO_IRQ();
  void UpdateClipMatrix();

  auto DequeueMatrix4x4() -> Matrix4<Fixed20x12> {
    Matrix4<Fixed20x12> mat;
    for (int col = 0; col < 4; col++) {
      for (int row = 0; row < 4; row++) {
        mat[col][row] = Dequeue().argument;
      }
    }
    return mat;
  }
  
  auto DequeueMatrix4x3() -> Matrix4<Fixed20x12> {
    Matrix4<Fixed20x12> mat;
    for (int col = 0; col < 4; col++) {
      for (int row = 0; row < 3; row++) {
        mat[col][row] = Dequeue().argument;
      }
    }
    mat[3][3] = Fixed20x12::from_int(1);
    return mat;
  }

  auto DequeueMatrix3x3() -> Matrix4<Fixed20x12> {
    Matrix4<Fixed20x12> mat;
    for (int col = 0; col < 3; col++) {
      for (int row = 0; row < 3; row++) {
        mat[col][row] = Dequeue().argument;
      }
    }
    mat[3][3] = Fixed20x12::from_int(1);
    return mat;
  }

  void SubmitVertex(Vector4<Fixed20x12> const& position);

  bool IsFrontFacing(
    Vector4<Fixed20x12> const& v0,
    Vector4<Fixed20x12> const& v1,
    Vector4<Fixed20x12> const& v2,
    bool invert
  );

  auto ClipPolygon(
    StaticVec<Vertex, 10> const& vertex_list,
    bool quadstrip
  ) -> StaticVec<Vertex, 10>;

  template<int axis, typename Comparator>
  bool ClipPolygonAgainstPlane(
    StaticVec<Vertex, 10> const& vertex_list_in,
    StaticVec<Vertex, 10>& vertex_list_out
  );

  auto SampleTexture(TextureParams const& params, Vector2<Fixed12x4> const& uv) -> Color4;

  template<typename T>
  auto ReadTextureVRAM(u32 address) {
    return read<T>(vram_texture_copy, address & 0x7FFFF & ~(sizeof(T) - 1));
  }

  template<typename T>
  auto ReadPaletteVRAM(u32 address) {
    return read<T>(vram_palette_copy, address & 0x1FFFF & ~(sizeof(T) - 1));
  }

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
  void CMD_SetPolygonAttributes();
  void CMD_SetTextureParameters();
  void CMD_SetPaletteBase();

  /// Lighting commands
  void CMD_SetMaterialColor0();
  void CMD_SetMaterialColor1();
  void CMD_SetLightVector();
  void CMD_SetLightColor();

  /// Vertex list commands
  void CMD_BeginVertexList();
  void CMD_EndVertexList();

  void CMD_SwapBuffers();

  void RenderRearPlane(int thread_min_y, int thread_max_y);
  void RenderPolygons(bool translucent, int thread_min_y, int thread_max_y);
  void RenderEdgeMarking();
  void SetupRenderWorkers();
  void JoinRenderWorkerThreads();

  bool in_vertex_list;
  bool is_quad;
  bool is_strip;
  bool is_first;
  int polygon_strip_length;

  struct VertexRAM {
    int count = 0;
    Vertex data[6144];
  } vertices[2];

  struct PolygonRAM {
    int count = 0;
    Polygon data[2048];
  } polygons[2];

  /// ID of the buffer the geometry engine currently writes into (between 0 and 1).
  int buffer = 0;

  /// List of vertices for the primitive that is being submitted.
  StaticVec<Vertex, 10> current_vertex_list;

  /// Untransformed vertex from the previous vertex submission command.
  Vector4<Fixed20x12> position_old;

  /// Current vertex color
  Color4 vertex_color;

  /// Current vertex UV
  Vector2<Fixed12x4> vertex_uv;
  Vector2<Fixed12x4> vertex_uv_source;

  /// Current polygon parameters
  PolygonParams poly_params;

  /// Current texture parameters
  TextureParams texture_params;

  struct Light {
    Vector3<Fixed20x12> direction;
    Vector3<Fixed20x12> halfway;
    Color4 color;
  } lights[4];

  struct Material {
    Color4 diffuse;
    Color4 ambient;
    Color4 specular;
    Color4 emissive;

    bool enable_shinyness_table = false;
  } material;

  std::array<u16, 32> toon_table;
  std::array<u16, 8> edge_color_table;

  /// GPU texture and texture palette data
  Region<4, 131072> const& vram_texture { 3 };
  Region<8> const& vram_palette { 7 };
  u8 vram_texture_copy[524288];
  u8 vram_palette_copy[131072];

  Scheduler& scheduler;
  IRQ& irq9;
  DMA9& dma9;
  FIFO<CmdArgPack, 256> gxfifo;
  FIFO<CmdArgPack, 4> gxpipe;
  
  Color4 color_buffer[256 * 192];
  u32 depth_buffer[256 * 192];

  enum AttributeFlags {
    ATTRIBUTE_FLAG_SHADOW = 1,
    ATTRIBUTE_FLAG_EDGE = 2
  };

  struct Attribute {
    u16 flags;
    u8  poly_id[2];
  } attribute_buffer[256 * 192];

  /// Packed command processing
  u32 packed_cmds;
  int packed_args_left;
  
  /// Matrix stacks
  MatrixMode matrix_mode;
  MatrixStack< 1> projection;
  MatrixStack<31> modelview;
  MatrixStack<31> direction;
  MatrixStack< 1> texture;
  Matrix4<Fixed20x12> clip_matrix;

  Scheduler::Event* cmd_event = nullptr;

  bool use_w_buffer;
  bool use_w_buffer_pending;
  bool swap_buffers_pending;

  static constexpr int kRenderThreadCount = 4;

  struct RenderWorker {
    int min_y;
    int max_y;
    std::thread thread;
    std::atomic_bool running;
    std::atomic_bool rendering;
    std::mutex rendering_mutex;
    std::condition_variable rendering_cv;
  } render_workers[kRenderThreadCount];

  std::unique_ptr<RendererBase> renderer;
};

} // namespace lunar::nds
