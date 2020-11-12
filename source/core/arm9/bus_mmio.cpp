/*
 * Copyright (C) 2020 fleroviux
 */

#include <common/log.hpp>

#include "bus.hpp"

namespace fauxDS::core {

enum Registers {
  // PPU
  REG_DISPSTAT = 0x0400'0004,
  REG_VCOUNT = 0x0400'0006,
  REG_BG0CNT_A = 0x0400'0008,
  REG_BG0CNT_B = 0x0400'1008,
  REG_BG1CNT_A = 0x0400'000A,
  REG_BG1CNT_B = 0x0400'100A,
  REG_BG2CNT_A = 0x0400'000C,
  REG_BG2CNT_B = 0x0400'100C,
  REG_BG3CNT_A = 0x0400'000E,
  REG_BG3CNT_B = 0x0400'100E,
  REG_BG0HOFS_A = 0x0400'0010,
  REG_BG0HOFS_B = 0x0400'1010,
  REG_BG0VOFS_A = 0x0400'0012,
  REG_BG0VOFS_B = 0x0400'1012,
  REG_BG1HOFS_A = 0x0400'0014,
  REG_BG1HOFS_B = 0x0400'1014,
  REG_BG1VOFS_A = 0x0400'0016,
  REG_BG1VOFS_B = 0x0400'1016,
  REG_BG2HOFS_A = 0x0400'0018,
  REG_BG2HOFS_B = 0x0400'1018,
  REG_BG2VOFS_A = 0x0400'001A,
  REG_BG2VOFS_B = 0x0400'101A,
  REG_BG3HOFS_A = 0x0400'001C,
  REG_BG3HOFS_B = 0x0400'101C,
  REG_BG3VOFS_A = 0x0400'001E,
  REG_BG3VOFS_B = 0x0400'101E,

  // Timers
  REG_TM0CNT_L = 0x0400'0100,
  REG_TM0CNT_H = 0x0400'0102,
  REG_TM1CNT_L = 0x0400'0104,
  REG_TM1CNT_H = 0x0400'0106,
  REG_TM2CNT_L = 0x0400'0108,
  REG_TM2CNT_H = 0x0400'010A,
  REG_TM3CNT_L = 0x0400'010C,
  REG_TM3CNT_H = 0x0400'010E,

  // Input
  REG_KEYINPUT = 0x0400'0130,

  // IPC
  REG_IPCSYNC = 0x0400'0180,
  REG_IPCFIFOCNT = 0x0400'0184,
  REG_IPCFIFOSEND = 0x0400'0188,
  REG_IPCFIFORECV = 0x0410'0000,

  // Cartridge interface
  REG_ROMCTRL = 0x0400'01A4,
  REG_CARDCMD = 0x0400'01A8,
  REG_CARDDATA = 0x0410'0010,

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
  auto& ppu_io_a = video_unit.ppu_a.mmio;
  auto& ppu_io_b = video_unit.ppu_b.mmio;

  switch (address) {
    // PPU engine A / B
    case REG_DISPSTAT|0:
      return video_unit.dispstat9.ReadByte(0);
    case REG_DISPSTAT|1:
      return video_unit.dispstat9.ReadByte(1);
    case REG_VCOUNT|0:
      return video_unit.vcount.ReadByte(0);
    case REG_VCOUNT|1:
      return video_unit.vcount.ReadByte(1);
    case REG_BG0CNT_A|0:
      return ppu_io_a.bgcnt[0].ReadByte(0);
    case REG_BG0CNT_A|1:
      return ppu_io_a.bgcnt[0].ReadByte(1);
    case REG_BG0CNT_B|0:
      return ppu_io_b.bgcnt[0].ReadByte(0);
    case REG_BG0CNT_B|1:
      return ppu_io_b.bgcnt[0].ReadByte(1);
    case REG_BG1CNT_A|0:
      return ppu_io_a.bgcnt[1].ReadByte(0);
    case REG_BG1CNT_A|1:
      return ppu_io_a.bgcnt[1].ReadByte(1);
    case REG_BG1CNT_B|0:
      return ppu_io_b.bgcnt[1].ReadByte(0);
    case REG_BG1CNT_B|1:
      return ppu_io_b.bgcnt[1].ReadByte(1);
    case REG_BG2CNT_A|0:
      return ppu_io_a.bgcnt[2].ReadByte(0);
    case REG_BG2CNT_A|1:
      return ppu_io_a.bgcnt[2].ReadByte(1);
    case REG_BG2CNT_B|0:
      return ppu_io_b.bgcnt[2].ReadByte(0);
    case REG_BG2CNT_B|1:
      return ppu_io_b.bgcnt[2].ReadByte(1);
    case REG_BG3CNT_A|0:
      return ppu_io_a.bgcnt[3].ReadByte(0);
    case REG_BG3CNT_A|1:
      return ppu_io_a.bgcnt[3].ReadByte(1);
    case REG_BG3CNT_B|0:
      return ppu_io_b.bgcnt[3].ReadByte(0);
    case REG_BG3CNT_B|1:
      return ppu_io_b.bgcnt[3].ReadByte(1);

    // Timers
    case REG_TM0CNT_L|0:
      return timer.Read(0, 0);
    case REG_TM0CNT_L|1:
      return timer.Read(0, 1);
    case REG_TM0CNT_H|0:
      return timer.Read(0, 2);
    case REG_TM0CNT_H|1:
      return timer.Read(0, 3);
    case REG_TM1CNT_L|0:
      return timer.Read(1, 0);
    case REG_TM1CNT_L|1:
      return timer.Read(1, 1);
    case REG_TM1CNT_H|0:
      return timer.Read(1, 2);
    case REG_TM1CNT_H|1:
      return timer.Read(1, 3);
    case REG_TM2CNT_L|0:
      return timer.Read(2, 0);
    case REG_TM2CNT_L|1:
      return timer.Read(2, 1);
    case REG_TM2CNT_H|0:
      return timer.Read(2, 2);
    case REG_TM2CNT_H|1:
      return timer.Read(2, 3);
    case REG_TM3CNT_L|0:
      return timer.Read(3, 0);
    case REG_TM3CNT_L|1:
      return timer.Read(3, 1);
    case REG_TM3CNT_H|0:
      return timer.Read(3, 2);
    case REG_TM3CNT_H|1:
      return timer.Read(3, 3);

    // Input
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

    // Cartridge interface
    case REG_ROMCTRL|0:
      return cart.romctrl.ReadByte(0);
    case REG_ROMCTRL|1:
      return cart.romctrl.ReadByte(1);
    case REG_ROMCTRL|2:
      return cart.romctrl.ReadByte(2);
    case REG_ROMCTRL|3:
      return cart.romctrl.ReadByte(3);
    case REG_CARDCMD|0:
      return cart.cardcmd.ReadByte(0);
    case REG_CARDCMD|1:
      return cart.cardcmd.ReadByte(1);
    case REG_CARDCMD|2:
      return cart.cardcmd.ReadByte(2);
    case REG_CARDCMD|3:
      return cart.cardcmd.ReadByte(3);
    case REG_CARDCMD|4:
      return cart.cardcmd.ReadByte(4);
    case REG_CARDCMD|5:
      return cart.cardcmd.ReadByte(5);
    case REG_CARDCMD|6:
      return cart.cardcmd.ReadByte(6);
    case REG_CARDCMD|7:
      return cart.cardcmd.ReadByte(7);
    case REG_CARDDATA|0:
    case REG_CARDDATA|1:
    case REG_CARDDATA|2:
    case REG_CARDDATA|3:
      ASSERT(false, "ARM9: unhandled byte read from REG_CARDDATA");
      return 0;

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
    case REG_CARDDATA:
      return cart.ReadData();
  }

  return (ReadByteIO(address | 0) <<  0) |
         (ReadByteIO(address | 1) <<  8) |
         (ReadByteIO(address | 2) << 16) |
         (ReadByteIO(address | 3) << 24);
}

void ARM9MemoryBus::WriteByteIO(u32 address,  u8 value) {
  auto& ppu_io_a = video_unit.ppu_a.mmio;
  auto& ppu_io_b = video_unit.ppu_b.mmio;

  switch (address) {
    // PPU engine A / B
    case REG_DISPSTAT|0:
      video_unit.dispstat9.WriteByte(0, value);
      break;
    case REG_DISPSTAT|1:
      video_unit.dispstat9.WriteByte(1, value);
      break;
    case REG_BG0CNT_A|0:
      ppu_io_a.bgcnt[0].WriteByte(0, value);
      break;
    case REG_BG0CNT_A|1:
      ppu_io_a.bgcnt[0].WriteByte(1, value);
      break;
    case REG_BG0CNT_B|0:
      ppu_io_b.bgcnt[0].WriteByte(0, value);
      break;
    case REG_BG0CNT_B|1:
      ppu_io_b.bgcnt[0].WriteByte(1, value);
      break;
    case REG_BG1CNT_A|0:
      ppu_io_a.bgcnt[1].WriteByte(0, value);
      break;
    case REG_BG1CNT_A|1:
      ppu_io_a.bgcnt[1].WriteByte(1, value);
      break;
    case REG_BG1CNT_B|0:
      ppu_io_b.bgcnt[1].WriteByte(0, value);
      break;
    case REG_BG1CNT_B|1:
      ppu_io_b.bgcnt[1].WriteByte(1, value);
      break;
    case REG_BG2CNT_A|0:
      ppu_io_a.bgcnt[2].WriteByte(0, value);
      break;
    case REG_BG2CNT_A|1:
      ppu_io_a.bgcnt[2].WriteByte(1, value);
      break;
    case REG_BG2CNT_B|0:
      ppu_io_b.bgcnt[2].WriteByte(0, value);
      break;
    case REG_BG2CNT_B|1:
      ppu_io_b.bgcnt[2].WriteByte(1, value);
      break;
    case REG_BG3CNT_A|0:
      ppu_io_a.bgcnt[3].WriteByte(0, value);
      break;
    case REG_BG3CNT_A|1:
      ppu_io_a.bgcnt[3].WriteByte(1, value);
      break;
    case REG_BG3CNT_B|0:
      ppu_io_b.bgcnt[3].WriteByte(0, value);
      break;
    case REG_BG3CNT_B|1:
      ppu_io_b.bgcnt[3].WriteByte(1, value);
      break;
    case REG_BG0HOFS_A|0:
      ppu_io_a.bghofs[0].WriteByte(0, value);
      break;
    case REG_BG0HOFS_A|1:
      ppu_io_a.bghofs[0].WriteByte(1, value);
      break;
    case REG_BG0HOFS_B|0:
      ppu_io_b.bghofs[0].WriteByte(0, value);
      break;
    case REG_BG0HOFS_B|1:
      ppu_io_b.bghofs[0].WriteByte(1, value);
      break;
    case REG_BG0VOFS_A|0:
      ppu_io_a.bgvofs[0].WriteByte(0, value);
      break;
    case REG_BG0VOFS_A|1:
      ppu_io_a.bgvofs[0].WriteByte(1, value);
      break;
    case REG_BG0VOFS_B|0:
      ppu_io_b.bgvofs[0].WriteByte(0, value);
      break;
    case REG_BG0VOFS_B|1:
      ppu_io_b.bgvofs[0].WriteByte(1, value);
      break;
    case REG_BG1HOFS_A|0:
      ppu_io_a.bghofs[1].WriteByte(0, value);
      break;
    case REG_BG1HOFS_A|1:
      ppu_io_a.bghofs[1].WriteByte(1, value);
      break;
    case REG_BG1HOFS_B|0:
      ppu_io_b.bghofs[1].WriteByte(0, value);
      break;
    case REG_BG1HOFS_B|1:
      ppu_io_b.bghofs[1].WriteByte(1, value);
      break;
    case REG_BG1VOFS_A|0:
      ppu_io_a.bgvofs[1].WriteByte(0, value);
      break;
    case REG_BG1VOFS_A|1:
      ppu_io_a.bgvofs[1].WriteByte(1, value);
      break;
    case REG_BG1VOFS_B|0:
      ppu_io_b.bgvofs[1].WriteByte(0, value);
      break;
    case REG_BG1VOFS_B|1:
      ppu_io_b.bgvofs[1].WriteByte(1, value);
      break;
    case REG_BG2HOFS_A|0:
      ppu_io_a.bghofs[2].WriteByte(0, value);
      break;
    case REG_BG2HOFS_A|1:
      ppu_io_a.bghofs[2].WriteByte(1, value);
      break;
    case REG_BG2HOFS_B|0:
      ppu_io_b.bghofs[2].WriteByte(0, value);
      break;
    case REG_BG2HOFS_B|1:
      ppu_io_b.bghofs[2].WriteByte(1, value);
      break;
    case REG_BG2VOFS_A|0:
      ppu_io_a.bgvofs[2].WriteByte(0, value);
      break;
    case REG_BG2VOFS_A|1:
      ppu_io_a.bgvofs[2].WriteByte(1, value);
      break;
    case REG_BG2VOFS_B|0:
      ppu_io_b.bgvofs[2].WriteByte(0, value);
      break;
    case REG_BG2VOFS_B|1:
      ppu_io_b.bgvofs[2].WriteByte(1, value);
      break;
    case REG_BG3HOFS_A|0:
      ppu_io_a.bghofs[3].WriteByte(0, value);
      break;
    case REG_BG3HOFS_A|1:
      ppu_io_a.bghofs[3].WriteByte(1, value);
      break;
    case REG_BG3HOFS_B|0:
      ppu_io_b.bghofs[3].WriteByte(0, value);
      break;
    case REG_BG3HOFS_B|1:
      ppu_io_b.bghofs[3].WriteByte(1, value);
      break;
    case REG_BG3VOFS_A|0:
      ppu_io_a.bgvofs[3].WriteByte(0, value);
      break;
    case REG_BG3VOFS_A|1:
      ppu_io_a.bgvofs[3].WriteByte(1, value);
      break;
    case REG_BG3VOFS_B|0:
      ppu_io_b.bgvofs[3].WriteByte(0, value);
      break;
    case REG_BG3VOFS_B|1:
      ppu_io_b.bgvofs[3].WriteByte(1, value);
      break;

    // Timers
    case REG_TM0CNT_L|0:
      timer.Write(0, 0, value);
      break;
    case REG_TM0CNT_L|1:
      timer.Write(0, 1, value);
      break;
    case REG_TM0CNT_H|0:
      timer.Write(0, 2, value);
      break;
    case REG_TM0CNT_H|1:
      timer.Write(0, 3, value);
      break;
    case REG_TM1CNT_L|0:
      timer.Write(1, 0, value);
      break;
    case REG_TM1CNT_L|1:
      timer.Write(1, 1, value);
      break;
    case REG_TM1CNT_H|0:
      timer.Write(1, 2, value);
      break;
    case REG_TM1CNT_H|1:
      timer.Write(1, 3, value);
      break;
    case REG_TM2CNT_L|0:
      timer.Write(2, 0, value);
      break;
    case REG_TM2CNT_L|1:
      timer.Write(2, 1, value);
      break;
    case REG_TM2CNT_H|0:
      timer.Write(2, 2, value);
      break;
    case REG_TM2CNT_H|1:
      timer.Write(2, 3, value);
      break;
    case REG_TM3CNT_L|0:
      timer.Write(3, 0, value);
      break;
    case REG_TM3CNT_L|1:
      timer.Write(3, 1, value);
      break;
    case REG_TM3CNT_H|0:
      timer.Write(3, 2, value);
      break;
    case REG_TM3CNT_H|1:
      timer.Write(3, 3, value);
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

    // Cartridge interface
    case REG_ROMCTRL|0:
      cart.romctrl.WriteByte(0, value);
      break;
    case REG_ROMCTRL|1:
      cart.romctrl.WriteByte(1, value);
      break;
    case REG_ROMCTRL|2:
      cart.romctrl.WriteByte(2, value);
      break;
    case REG_ROMCTRL|3:
      cart.romctrl.WriteByte(3, value);
      break;
    case REG_CARDCMD|0:
      cart.cardcmd.WriteByte(0, value);
      break;
    case REG_CARDCMD|1:
      cart.cardcmd.WriteByte(1, value);
      break;
    case REG_CARDCMD|2:
      cart.cardcmd.WriteByte(2, value);
      break;
    case REG_CARDCMD|3:
      cart.cardcmd.WriteByte(3, value);
      break;
    case REG_CARDCMD|4:
      cart.cardcmd.WriteByte(4, value);
      break;
    case REG_CARDCMD|5:
      cart.cardcmd.WriteByte(5, value);
      break;
    case REG_CARDCMD|6:
      cart.cardcmd.WriteByte(6, value);
      break;
    case REG_CARDCMD|7:
      cart.cardcmd.WriteByte(7, value);
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
      vram.vramcnt_a.WriteByte(value);
      break;
    case REG_VRAMCNT_B:
      vram.vramcnt_b.WriteByte(value);
      break;
    case REG_VRAMCNT_C:
      vram.vramcnt_c.WriteByte(value);
      break;
    case REG_VRAMCNT_D:
      vram.vramcnt_d.WriteByte(value);
      break;
    case REG_VRAMCNT_E:
      vram.vramcnt_e.WriteByte(value);
      break;
    case REG_VRAMCNT_F:
      vram.vramcnt_f.WriteByte(value);
      break;
    case REG_VRAMCNT_G:
      vram.vramcnt_g.WriteByte(value);
      break;
    case REG_WRAMCNT:
      wramcnt.WriteByte(value);
      break;
    case REG_VRAMCNT_H:
      vram.vramcnt_h.WriteByte(value);
      break;
    case REG_VRAMCNT_I:
      vram.vramcnt_i.WriteByte(value);
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
