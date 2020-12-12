/*
 * Copyright (C) 2020 fleroviux
 */

#include <common/log.hpp>

#include "gpu.hpp"

namespace fauxDS::core {

void GPU::Reset() {
  disp3dcnt = {};
  // FIXME
  //gxstat = {};
  gxfifo.Reset();
}

void GPU::WriteCommandPort(u8 port, u32 parameter) {
  LOG_DEBUG("GPU: write to command port, port=0x{0:02X}, param=0x{1:08X}", port, parameter);
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
