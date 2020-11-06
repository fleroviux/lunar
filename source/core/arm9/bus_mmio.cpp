/*
 * Copyright (C) 2020 fleroviux
 */

#include <common/log.hpp>

#include "bus.hpp"

namespace fauxDS::core {

enum Registers {
  // PPU engine A
  REG_DISPSTAT = 0x0400'0004,
  REG_VCOUNT = 0x0400'0006,

  REG_KEYINPUT = 0x0400'0130,

  // IPC
  REG_IPCSYNC = 0x0400'0180,
  REG_IPCFIFOCNT = 0x0400'0184,
  REG_IPCFIFOSEND = 0x0400'0188,
  REG_IPCFIFORECV = 0x0410'0000,

  // IRQ
  REG_IME = 0x0400'0208,
  REG_IE  = 0x0400'0210,
  REG_IF  = 0x0400'0214,

  // Memory control
  REG_VRAMCNT_A = 0x0400'0240,
  REG_VRAMCNT_B = 0x0400'0241,
  REG_VRAMCNT_C = 0x0400'0242,
  REG_VRAMCNT_D = 0x0400'0243,
  REG_VRAMCNT_E = 0x0400'0244,
  REG_VRAMCNT_F = 0x0400'0245,
  REG_VRAMCNT_G = 0x0400'0246,
  REG_WRAMCNT   = 0x0400'0247,
  REG_VRAMCNT_H = 0x0400'0248,
  REG_VRAMCNT_I = 0x0400'0249
};

auto ARM9MemoryBus::ReadByteIO(u32 address) -> u8 {
  switch (address) {
    // PPU engine A
    case REG_DISPSTAT|0:
      return video_unit.dispstat.ReadByte(0);
    case REG_DISPSTAT|1:
      return video_unit.dispstat.ReadByte(1);
    case REG_VCOUNT|0:
      return video_unit.vcount.ReadByte(0);
    case REG_VCOUNT|1:
      return video_unit.vcount.ReadByte(1);

    case REG_KEYINPUT|0:
      return keyinput.ReadByte(0);
    case REG_KEYINPUT|1:
      return keyinput.ReadByte(1);

    // IPC
    case REG_IPCSYNC|0:
      return ipc.ipcsync.ReadByte(IPC::Client::ARM9, 0);
    case REG_IPCSYNC|1:
      return ipc.ipcsync.ReadByte(IPC::Client::ARM9, 1);
    case REG_IPCFIFOCNT|0:
      return ipc.ipcfifocnt.ReadByte(IPC::Client::ARM9, 0);
    case REG_IPCFIFOCNT|1:
      return ipc.ipcfifocnt.ReadByte(IPC::Client::ARM9, 1);
    case REG_IPCFIFORECV|0:
      return ipc.ipcfiforecv.ReadByte(IPC::Client::ARM9, 0);
    case REG_IPCFIFORECV|1:
      return ipc.ipcfiforecv.ReadByte(IPC::Client::ARM9, 1);
    case REG_IPCFIFORECV|2:
      return ipc.ipcfiforecv.ReadByte(IPC::Client::ARM9, 2);
    case REG_IPCFIFORECV|3:
      return ipc.ipcfiforecv.ReadByte(IPC::Client::ARM9, 3);

    // IRQ
    case REG_IME|0:
      return irq9.ime.ReadByte(0);
    case REG_IME|1:
      return irq9.ime.ReadByte(1);
    case REG_IME|2:
      return irq9.ime.ReadByte(2);
    case REG_IME|3:
      return irq9.ime.ReadByte(3);
    case REG_IE|0:
      return irq9.ie.ReadByte(0);
    case REG_IE|1:
      return irq9.ie.ReadByte(1);
    case REG_IE|2:
      return irq9.ie.ReadByte(2);
    case REG_IE|3:
      return irq9.ie.ReadByte(3);
    case REG_IF|0:
      return irq9._if.ReadByte(0);
    case REG_IF|1:
      return irq9._if.ReadByte(1);
    case REG_IF|2:
      return irq9._if.ReadByte(2);
    case REG_IF|3:
      return irq9._if.ReadByte(3);

    case REG_WRAMCNT:
      return wramcnt.ReadByte();

    default:
      LOG_WARN("ARM9: MMIO: unhandled read from 0x{0:08X}", address);
  }

  return 0;
}

auto ARM9MemoryBus::ReadHalfIO(u32 address) -> u16 {
  switch (address) {
    case REG_IPCFIFORECV|0:
      return ipc.ipcfiforecv.ReadHalf(IPC::Client::ARM9, 0);
    case REG_IPCFIFORECV|2:
      return ipc.ipcfiforecv.ReadHalf(IPC::Client::ARM9, 2);
  }

  return (ReadByteIO(address | 0) << 0) |
         (ReadByteIO(address | 1) << 8);
}

auto ARM9MemoryBus::ReadWordIO(u32 address) -> u32 {
  switch (address) {
    case REG_IPCFIFORECV:
      return ipc.ipcfiforecv.ReadWord(IPC::Client::ARM9);
  }

  return (ReadByteIO(address | 0) <<  0) |
         (ReadByteIO(address | 1) <<  8) |
         (ReadByteIO(address | 2) << 16) |
         (ReadByteIO(address | 3) << 24);
}

void ARM9MemoryBus::WriteByteIO(u32 address,  u8 value) {
  switch (address) {
    // PPU engine A
    case REG_DISPSTAT|0:
      video_unit.dispstat.WriteByte(0, value);
      break;
    case REG_DISPSTAT|1:
      video_unit.dispstat.WriteByte(1, value);
      break;

    // IPC
    case REG_IPCSYNC|0:
      ipc.ipcsync.WriteByte(IPC::Client::ARM9, 0, value);
      break;
    case REG_IPCSYNC|1:
      ipc.ipcsync.WriteByte(IPC::Client::ARM9, 1, value);
      break;
    case REG_IPCFIFOCNT|0:
      ipc.ipcfifocnt.WriteByte(IPC::Client::ARM9, 0, value);
      break;
    case REG_IPCFIFOCNT|1:
      ipc.ipcfifocnt.WriteByte(IPC::Client::ARM9, 1, value);
      break;
    case REG_IPCFIFOSEND|0:
    case REG_IPCFIFOSEND|1:
    case REG_IPCFIFOSEND|2:
    case REG_IPCFIFOSEND|3:
      ipc.ipcfifosend.WriteByte(IPC::Client::ARM9, value);
      break;

    // IRQ
    case REG_IME|0:
      irq9.ime.WriteByte(0, value);
      break;
    case REG_IME|1:
      irq9.ime.WriteByte(1, value);
      break;
    case REG_IME|2:
      irq9.ime.WriteByte(2, value);
      break;
    case REG_IME|3:
      irq9.ime.WriteByte(3, value);
      break;
    case REG_IE|0:
      irq9.ie.WriteByte(0, value);
      break;
    case REG_IE|1:
      irq9.ie.WriteByte(1, value);
      break;
    case REG_IE|2:
      irq9.ie.WriteByte(2, value);
      break;
    case REG_IE|3:
      irq9.ie.WriteByte(3, value);
      break;
    case REG_IF|0:
      irq9._if.WriteByte(0, value);
      break;
    case REG_IF|1:
      irq9._if.WriteByte(1, value);
      break;
    case REG_IF|2:
      irq9._if.WriteByte(2, value);
      break;
    case REG_IF|3:
      irq9._if.WriteByte(3, value);
      break;

    // Memory control
    case REG_VRAMCNT_A:
      video_unit.vram.vramcnt_a.WriteByte(value);
      break;
    case REG_VRAMCNT_B:
      video_unit.vram.vramcnt_b.WriteByte(value);
      break;
    case REG_VRAMCNT_C:
      video_unit.vram.vramcnt_c.WriteByte(value);
      break;
    case REG_VRAMCNT_D:
      video_unit.vram.vramcnt_d.WriteByte(value);
      break;
    case REG_VRAMCNT_E:
      video_unit.vram.vramcnt_e.WriteByte(value);
      break;
    case REG_VRAMCNT_F:
      video_unit.vram.vramcnt_f.WriteByte(value);
      break;
    case REG_VRAMCNT_G:
      video_unit.vram.vramcnt_g.WriteByte(value);
      break;
    case REG_WRAMCNT:
      wramcnt.WriteByte(value);
      break;
    case REG_VRAMCNT_H:
      video_unit.vram.vramcnt_h.WriteByte(value);
      break;
    case REG_VRAMCNT_I:
      video_unit.vram.vramcnt_i.WriteByte(value);
      break;

    default:
      LOG_WARN("ARM9: MMIO: unhandled write to 0x{0:08X} = 0x{1:02X}", address, value);
  }
}

void ARM9MemoryBus::WriteHalfIO(u32 address, u16 value) {
  switch (address) {
    case REG_IPCFIFOSEND|0:
    case REG_IPCFIFOSEND|2:
      ipc.ipcfifosend.WriteHalf(IPC::Client::ARM9, value);
      break;
    default:
      WriteByteIO(address | 0, value & 0xFF);
      WriteByteIO(address | 1, value >> 8);
      break;
  }
}

void ARM9MemoryBus::WriteWordIO(u32 address, u32 value) {
  switch (address) {
    case REG_IPCFIFOSEND:
      ipc.ipcfifosend.WriteWord(IPC::Client::ARM9, value);
      break;
    default:
      WriteByteIO(address | 0, u8(value >>  0));
      WriteByteIO(address | 1, u8(value >>  8));
      WriteByteIO(address | 2, u8(value >> 16));
      WriteByteIO(address | 3, u8(value >> 24));
      break;
  }
}

} // namespace fauxDS::core
