/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <common/integer.hpp>
#include <common/fifo.hpp>
#include <core/hw/irq/irq.hpp>

namespace fauxDS::core {

/// 3D graphics processing unit (GPU)
struct GPU {
  GPU(IRQ& irq9)
      : irq9(irq9) {
    Reset();
  }
  
  void Reset();
  
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
  // TODO: come up with a name that does *not* suck.
  struct CmdArgPack {
    // NOTE: command is only used if this is the first entry of a command.
    u8 command = 0;
    u32 argument = 0;
  };

  void CheckGXFIFO_IRQ();

  common::FIFO<CmdArgPack, 256> gxfifo;

  IRQ& irq9;
};

} // namespace fauxDS::core
