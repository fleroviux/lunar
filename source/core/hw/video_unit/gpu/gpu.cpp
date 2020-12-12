/*
 * Copyright (C) 2020 fleroviux
 */

#include <common/log.hpp>

#include "gpu.hpp"

namespace fauxDS::core {

static constexpr int kCmdNumParams[] = {
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
}

void GPU::WriteCommandPort(uint port, u32 parameter) {
  u8 command = 0x00;

  // TODO: is there a cleaner way to translate ports to commands...?
  switch (port) {
    case 0x40: command = 0x10; break;
    case 0x44: command = 0x11; break;
    case 0x48: command = 0x12; break;
    case 0x4C: command = 0x13; break;
    case 0x50: command = 0x14; break;
    case 0x54: command = 0x15; break;
    case 0x58: command = 0x16; break;
    case 0x5C: command = 0x17; break;
    case 0x60: command = 0x18; break;
    case 0x64: command = 0x19; break;
    case 0x68: command = 0x1A; break;
    case 0x6C: command = 0x1B; break;
    case 0x70: command = 0x1C; break;
    case 0x80: command = 0x20; break;
    case 0x84: command = 0x21; break;
    case 0x88: command = 0x22; break;
    case 0x8C: command = 0x23; break;
    case 0x90: command = 0x24; break;
    case 0x94: command = 0x25; break;
    case 0x98: command = 0x26; break;
    case 0x9C: command = 0x27; break;
    case 0xA0: command = 0x28; break;
    case 0xA4: command = 0x29; break;
    case 0xA8: command = 0x2A; break;
    case 0xAC: command = 0x2B; break;
    case 0xC0: command = 0x30; break;
    case 0xC4: command = 0x31; break;
    case 0xC8: command = 0x32; break;
    case 0xCC: command = 0x33; break;
    case 0xD0: command = 0x34; break;
    case 0x100: command = 0x40; break;
    case 0x104: command = 0x41; break;
    case 0x140: command = 0x50; break;
    case 0x180: command = 0x60; break;
    case 0x1C0: command = 0x70; break;
    case 0x1C4: command = 0x71; break;
    case 0x1C8: command = 0x72; break;

    default:
      LOG_ERROR("GPU: unknown command port, port=0x{0:02X}, param=0x{1:08X}", port, parameter);
  }

  LOG_DEBUG("GPU: received command 0x{0:02X} param0=0x{1:08X}", command, parameter);
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
