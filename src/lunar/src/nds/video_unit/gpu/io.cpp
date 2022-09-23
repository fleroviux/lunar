/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <lunar/log.hpp>

#include "gpu.hpp"

namespace lunar::nds {

auto GPU::DISP3DCNT::ReadByte(uint offset) -> u8 {
  switch (offset) {
    case 0: {
      return (enable_textures ? 1 : 0) |
             (static_cast<u8>(shading_mode) << 1) |
             (enable_alpha_test   ?  4 : 0) |
             (enable_alpha_blend  ?  8 : 0) |
             (enable_antialias    ? 16 : 0) |
             (enable_edge_marking ? 32 : 0) |
             (static_cast<u8>(fog_mode) << 6) |
             (enable_fog ? 128 : 0);
    }
    case 1: {
      return fog_depth_shift |
            (rdlines_underflow     ? 16 : 0) |
            (polyvert_ram_overflow ? 32 : 0) |
            (enable_rear_bitmap    ? 64 : 0);
    }
  }

  UNREACHABLE;
}

void GPU::DISP3DCNT::WriteByte(uint offset, u8 value) {
  switch (offset) {
    case 0: {
      enable_textures = value & 1;
      shading_mode = static_cast<Shading>((value >> 1) & 1);
      enable_alpha_test = value & 4;
      enable_alpha_blend = value & 8;
      enable_antialias = value & 16;
      enable_edge_marking = value & 32;
      fog_mode = static_cast<FogMode>((value >> 6) & 1);
      enable_fog = value & 128;
      break;
    }
    case 1: {
      fog_depth_shift = value & 0xF;
      enable_rear_bitmap = value & 64;
      break;
    }
    default: {
      UNREACHABLE;
    }
  }
}

auto GPU::GXSTAT::ReadByte(uint offset) -> u8 {
  switch (offset) {
    case 0: {
      // TODO: do not hardcode the box-test result to be true.
      return 2;
    }
    case 1: {
      return 0;
    }
    case 2: {
      return gpu.gxfifo.Count();
    }
    case 3: {
      return (gpu.gxfifo.IsFull() ? 1 : 0) |
             (gpu.gxfifo.Count() < 128 ? 2 : 0) |
             (gpu.gxfifo.IsEmpty() ? 4 : 0) |
             (gx_busy ? 8 : 0) |
             (static_cast<u8>(cmd_fifo_irq) << 6);
    }
  }

  UNREACHABLE;
}

void GPU::GXSTAT::WriteByte(uint offset, u8 value) {
  auto cmd_fifo_irq_old = cmd_fifo_irq;

  switch (offset) {
    case 0:
    case 1:
    case 2: break;
    case 3: {
      cmd_fifo_irq = static_cast<IRQMode>(value >> 6);
      // TODO: confirm that the GXFIFO IRQ indeed can be triggered by this.
      if (cmd_fifo_irq != cmd_fifo_irq_old) {
        gpu.CheckGXFIFO_IRQ();
      }
      break;
    }
    default: {
      UNREACHABLE;
    }
  }
}

void GPU::AlphaTest::WriteByte(u8 value) {
  alpha = value;
}

void GPU::ClearColor::WriteByte(uint offset, u8 value) {
  switch (offset) {
    case 0: {
      color_r = value & 31;
      color_g = (color_g & ~7) | ((value >> 5) & 7);
      break;
    }
    case 1: {
      color_g = (color_g & 7) | ((value & 3) << 3);
      color_b = (value >> 2) & 31;
      enable_fog = value & 128;
      break;
    }
    case 2: {
      color_a = value & 31;
      break;
    }
    case 3: {
      polygon_id = value & 63;
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

void GPU::FogColor::WriteByte(uint offset, u8 value) {
  switch (offset) {
    case 0: {
      r = value & 0x1F;
      g = (g & ~7) | (value >> 5);
      break;
    }
    case 1: {
      g = (g & 7) | ((value & 3) << 3);
      b = (value >> 2) & 0x1F;
      break;
    }
    case 2: {
      a = value & 0x1F;
      break;
    }
  }
}

void GPU::FogOffset::WriteByte(uint offset, u8 byte) {
  switch (offset) {
    case 0: {
      value = (value & 0x7F00) | byte;
      break;
    }
    case 1: {
      value = (value & 0x00FF) | ((byte << 8) & 0x7F00);
      break;
    }
  }
}

} // namespace lunar::nds