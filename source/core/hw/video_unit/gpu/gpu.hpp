/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <common/integer.hpp>
#include <common/fifo.hpp>
#include <core/hw/irq/irq.hpp>
#include <core/scheduler.hpp>
#include <stack>

#include "matrix_stack.hpp"

namespace fauxDS::core {

/// 3D graphics processing unit (GPU)
struct GPU {
  GPU(Scheduler& scheduler, IRQ& irq9)
      : scheduler(scheduler)
      , irq9(irq9) {
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
    // NOTE: command is only used if this is the first entry of a command.
    u8 command = 0;
    u32 argument = 0;
  };

  void Enqueue(CmdArgPack pack);
  auto Dequeue() -> CmdArgPack;
  void ProcessCommands();
  void CheckGXFIFO_IRQ();

  void CMD_SetMatrixMode();
  void CMD_PushMatrix();
  void CMD_PopMatrix();
  void CMD_StoreMatrix();
  void CMD_LoadIdentity();
  void CMD_LoadMatrix4x4();
  void CMD_LoadMatrix4x3();
  void CMD_MatrixMultiply4x4();
  void CMD_MatrixMultiply4x3();
  void CMD_MatrixMultiply3x3();
  void CMD_MatrixScale();
  void CMD_MatrixTranslate();

  Scheduler& scheduler;
  IRQ& irq9;
  common::FIFO<CmdArgPack, 256> gxfifo;
  common::FIFO<CmdArgPack, 4> gxpipe;
  
  u16 output[256 * 192];

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
