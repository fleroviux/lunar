/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <lunar/log.hpp>

#include "bus.hpp"

namespace lunar::nds {

// TODO: handle byte and hword GPU accesses, but how do they behave?

enum Registers {
  // PPU
  REG_DISPCNT_A = 0x0400'0000,
  REG_DISPCNT_B = 0x0400'1000,
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
  REG_BG2PA_A = 0x0400'0020,
  REG_BG2PA_B = 0x0400'1020,
  REG_BG2PB_A = 0x0400'0022,
  REG_BG2PB_B = 0x0400'1022,
  REG_BG2PC_A = 0x0400'0024,
  REG_BG2PC_B = 0x0400'1024,
  REG_BG2PD_A = 0x0400'0026,
  REG_BG2PD_B = 0x0400'1026,
  REG_BG2X_A = 0x0400'0028,
  REG_BG2X_B = 0x0400'1028,
  REG_BG2Y_A = 0x0400'002C,
  REG_BG2Y_B = 0x0400'102C,
  REG_BG3PA_A = 0x0400'0030,
  REG_BG3PA_B = 0x0400'1030,
  REG_BG3PB_A = 0x0400'0032,
  REG_BG3PB_B = 0x0400'1032,
  REG_BG3PC_A = 0x0400'0034,
  REG_BG3PC_B = 0x0400'1034,
  REG_BG3PD_A = 0x0400'0036,
  REG_BG3PD_B = 0x0400'1036,
  REG_BG3X_A = 0x0400'0038,
  REG_BG3X_B = 0x0400'1038,
  REG_BG3Y_A = 0x0400'003C,
  REG_BG3Y_B = 0x0400'103C,
  REG_WIN0H_A = 0x0400'0040,
  REG_WIN0H_B = 0x0400'1040,
  REG_WIN1H_A = 0x0400'0042,
  REG_WIN1H_B = 0x0400'1042,
  REG_WIN0V_A = 0x0400'0044,
  REG_WIN0V_B = 0x0400'1044,
  REG_WIN1V_A = 0x0400'0046,
  REG_WIN1V_B = 0x0400'1046,
  REG_WININ_A = 0x0400'0048,
  REG_WININ_B = 0x0400'1048,
  REG_WINOUT_A = 0x0400'004A,
  REG_WINOUT_B = 0x0400'104A,
  REG_MOSAIC_A = 0x0400'004C,
  REG_MOSAIC_B = 0x0400'104C,
  REG_BLDCNT_A = 0x0400'0050,
  REG_BLDCNT_B = 0x0400'1050,
  REG_BLDALPHA_A = 0x0400'0052,
  REG_BLDALPHA_B = 0x0400'1052,
  REG_BLDY_A = 0x0400'0054,
  REG_BLDY_B = 0x0400'1054,
  REG_DISPCAPCNT = 0x0400'0064,
  REG_MASTER_BRIGHT_A = 0x0400'006C,
  REG_MASTER_BRIGHT_B = 0x0400'106C,

  // DMA
  REG_DMA0SAD = 0x0400'00B0,
  REG_DMA0DAD = 0x0400'00B4,
  REG_DMA0CNT_L = 0x0400'00B8,
  REG_DMA0CNT_H = 0x0400'00BA,
  REG_DMA1SAD = 0x0400'00BC,
  REG_DMA1DAD = 0x0400'00C0,
  REG_DMA1CNT_L = 0x0400'00C4,
  REG_DMA1CNT_H = 0x0400'00C6,
  REG_DMA2SAD = 0x0400'00C8,
  REG_DMA2DAD = 0x0400'00CC,
  REG_DMA2CNT_L = 0x0400'00D0,
  REG_DMA2CNT_H = 0x0400'00D2,
  REG_DMA3SAD = 0x0400'00D4,
  REG_DMA3DAD = 0x0400'00D8,
  REG_DMA3CNT_L = 0x0400'00DC,
  REG_DMA3CNT_H = 0x0400'00DE,
  REG_DMA0FILL = 0x0400'00E0,
  REG_DMA1FILL = 0x0400'00E4,
  REG_DMA2FILL = 0x0400'00E8,
  REG_DMA3FILL = 0x0400'00EC,

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
  REG_KEYCNT = 0x0400'0132,

  // IPC
  REG_IPCSYNC = 0x0400'0180,
  REG_IPCFIFOCNT = 0x0400'0184,
  REG_IPCFIFOSEND = 0x0400'0188,
  REG_IPCFIFORECV = 0x0410'0000,

  // Cartridge interface
  REG_AUXSPICNT = 0x0400'01A0,
  REG_AUXSPIDATA = 0x0400'01A2,
  REG_ROMCTRL = 0x0400'01A4,
  REG_CARDCMD = 0x0400'01A8,
  REG_CARDDATA = 0x0410'0010,

  // IRQ
  REG_IME = 0x0400'0208,
  REG_IE  = 0x0400'0210,
  REG_IF  = 0x0400'0214,

  // Memory control
  REG_EXMEMCNT  = 0x0400'0204,
  REG_VRAMCNT_A = 0x0400'0240,
  REG_VRAMCNT_B = 0x0400'0241,
  REG_VRAMCNT_C = 0x0400'0242,
  REG_VRAMCNT_D = 0x0400'0243,
  REG_VRAMCNT_E = 0x0400'0244,
  REG_VRAMCNT_F = 0x0400'0245,
  REG_VRAMCNT_G = 0x0400'0246,
  REG_WRAMCNT   = 0x0400'0247,
  REG_VRAMCNT_H = 0x0400'0248,
  REG_VRAMCNT_I = 0x0400'0249,

  // Math engine
  REG_DIVCNT = 0x0400'0280,
  REG_DIV_NUMER = 0x0400'0290,
  REG_DIV_DENOM = 0x0400'0298,
  REG_DIV_RESULT = 0x0400'02A0,
  REG_DIVREM_RESULT = 0x0400'02A8,
  REG_SQRTCNT = 0x0400'02B0,
  REG_SQRT_RESULT = 0x0400'02B4,
  REG_SQRT_PARAM = 0x0400'02B8,

  REG_POSTFLG = 0x0400'0300,
  
  // GPU
  REG_DISP3DCNT = 0x0400'0060,
  REG_POWCNT1 = 0x0400'0304,
  REG_EDGE_COLOR_LO = 0x0400'0330,
  REG_EDGE_COLOR_HI = 0x0400'033F,
  REG_ALPHA_TEST_REF = 0x0400'0340,
  REG_CLEAR_COLOR = 0x0400'0350,
  REG_CLEAR_DEPTH = 0x0400'0354,
  REG_CLRIMAGE_OFFSET = 0x0400'0356,
  REG_FOG_COLOR = 0x0400'0358,
  REG_FOG_OFFSET = 0x0400'035C,
  REG_FOG_TABLE_LO = 0x0400'0360,
  REG_FOG_TABLE_HI = 0x0400'037F,
  REG_TOON_TABLE_LO = 0x0400'0380,
  REG_TOON_TABLE_HI = 0x0400'03BF,
  REG_GXFIFO_LO = 0x0400'0400,
  REG_GXFIFO_HI = 0x0400'043F,
  REG_GXCMDPORT_LO = 0x0400'0440,
  REG_GXCMDPORT_HI = 0x0400'05C8,
  REG_GXSTAT = 0x0400'0600,
  REG_CLIPMTX_RESULT_LO = 0x0400'0640,
  REG_CLIPMTX_RESULT_HI = 0x0400'067F
};

auto ARM9MemoryBus::ReadByteIO(u32 address) -> u8 {
  auto& gpu_io = video_unit.gpu;
  auto& ppu_io_a = video_unit.ppu_a.mmio;
  auto& ppu_io_b = video_unit.ppu_b.mmio;

  switch (address) {
    // PPU engine A
    case REG_DISPCNT_A|0:
      return ppu_io_a.dispcnt.ReadByte(0);
    case REG_DISPCNT_A|1:
      return ppu_io_a.dispcnt.ReadByte(1);
    case REG_DISPCNT_A|2:
      return ppu_io_a.dispcnt.ReadByte(2);
    case REG_DISPCNT_A|3:
      return ppu_io_a.dispcnt.ReadByte(3);
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
    case REG_BG1CNT_A|0:
      return ppu_io_a.bgcnt[1].ReadByte(0);
    case REG_BG1CNT_A|1:
      return ppu_io_a.bgcnt[1].ReadByte(1);
    case REG_BG2CNT_A|0:
      return ppu_io_a.bgcnt[2].ReadByte(0);
    case REG_BG2CNT_A|1:
      return ppu_io_a.bgcnt[2].ReadByte(1);
    case REG_BG3CNT_A|0:
      return ppu_io_a.bgcnt[3].ReadByte(0);
    case REG_BG3CNT_A|1:
      return ppu_io_a.bgcnt[3].ReadByte(1);
    case REG_WININ_A|0:
      return ppu_io_a.winin.ReadByte(0);
    case REG_WININ_A|1:
      return ppu_io_a.winin.ReadByte(1);
    case REG_WINOUT_A|0:
      return ppu_io_a.winout.ReadByte(0);
    case REG_WINOUT_A|1:
      return ppu_io_a.winout.ReadByte(1);
    case REG_BLDCNT_A|0:
      return ppu_io_a.bldcnt.ReadByte(0);
    case REG_BLDCNT_A|1:
      return ppu_io_a.bldcnt.ReadByte(1);
    case REG_BLDALPHA_A|0:
      return ppu_io_a.bldalpha.ReadByte(0);
    case REG_BLDALPHA_A|1:
      return ppu_io_a.bldalpha.ReadByte(1);
    case REG_DISPCAPCNT|0:
      return video_unit.dispcapcnt.ReadByte(0);
    case REG_DISPCAPCNT|1:
      return video_unit.dispcapcnt.ReadByte(1);
    case REG_DISPCAPCNT|2:
      return video_unit.dispcapcnt.ReadByte(2);
    case REG_DISPCAPCNT|3:
      return video_unit.dispcapcnt.ReadByte(3);
    case REG_MASTER_BRIGHT_A|0:
      return ppu_io_a.master_bright.ReadByte(0);
    case REG_MASTER_BRIGHT_A|1:
      return ppu_io_a.master_bright.ReadByte(1);

    // PPU engine B
    case REG_DISPCNT_B|0:
      return ppu_io_b.dispcnt.ReadByte(0);
    case REG_DISPCNT_B|1:
      return ppu_io_b.dispcnt.ReadByte(1);
    case REG_DISPCNT_B|2:
      return ppu_io_b.dispcnt.ReadByte(2);
    case REG_DISPCNT_B|3:
      return ppu_io_b.dispcnt.ReadByte(3);
    case REG_BG0CNT_B|0:
      return ppu_io_b.bgcnt[0].ReadByte(0);
    case REG_BG0CNT_B|1:
      return ppu_io_b.bgcnt[0].ReadByte(1);
    case REG_BG1CNT_B|0:
      return ppu_io_b.bgcnt[1].ReadByte(0);
    case REG_BG1CNT_B|1:
      return ppu_io_b.bgcnt[1].ReadByte(1);
    case REG_BG2CNT_B|0:
      return ppu_io_b.bgcnt[2].ReadByte(0);
    case REG_BG2CNT_B|1:
      return ppu_io_b.bgcnt[2].ReadByte(1);
    case REG_BG3CNT_B|0:
      return ppu_io_b.bgcnt[3].ReadByte(0);
    case REG_BG3CNT_B|1:
      return ppu_io_b.bgcnt[3].ReadByte(1);
    case REG_WININ_B|0:
      return ppu_io_b.winin.ReadByte(0);
    case REG_WININ_B|1:
      return ppu_io_b.winin.ReadByte(1);
    case REG_WINOUT_B|0:
      return ppu_io_b.winout.ReadByte(0);
    case REG_WINOUT_B|1:
      return ppu_io_b.winout.ReadByte(1);
    case REG_BLDCNT_B|0:
      return ppu_io_b.bldcnt.ReadByte(0);
    case REG_BLDCNT_B|1:
      return ppu_io_b.bldcnt.ReadByte(1);
    case REG_BLDALPHA_B|0:
      return ppu_io_b.bldalpha.ReadByte(0);
    case REG_BLDALPHA_B|1:
      return ppu_io_b.bldalpha.ReadByte(1);
    case REG_MASTER_BRIGHT_B|0:
      return ppu_io_b.master_bright.ReadByte(0);
    case REG_MASTER_BRIGHT_B|1:
      return ppu_io_b.master_bright.ReadByte(1);
    
    // DMA
    case REG_DMA0SAD|0:
      return dma.Read(0, 0);
    case REG_DMA0SAD|1:
      return dma.Read(0, 1);
    case REG_DMA0SAD|2:
      return dma.Read(0, 2);
    case REG_DMA0SAD|3:
      return dma.Read(0, 3);
    case REG_DMA0DAD|0:
      return dma.Read(0, 4);
    case REG_DMA0DAD|1:
      return dma.Read(0, 5);
    case REG_DMA0DAD|2:
      return dma.Read(0, 6);
    case REG_DMA0DAD|3:
      return dma.Read(0, 7);
    case REG_DMA0CNT_L|0:
      return dma.Read(0, 8);
    case REG_DMA0CNT_L|1:
      return dma.Read(0, 9);
    case REG_DMA0CNT_H|0:
      return dma.Read(0, 10);
    case REG_DMA0CNT_H|1:
      return dma.Read(0, 11);
    case REG_DMA1SAD|0:
      return dma.Read(1, 0);
    case REG_DMA1SAD|1:
      return dma.Read(1, 1);
    case REG_DMA1SAD|2:
      return dma.Read(1, 2);
    case REG_DMA1SAD|3:
      return dma.Read(1, 3);
    case REG_DMA1DAD|0:
      return dma.Read(1, 4);
    case REG_DMA1DAD|1:
      return dma.Read(1, 5);
    case REG_DMA1DAD|2:
      return dma.Read(1, 6);
    case REG_DMA1DAD|3:
      return dma.Read(1, 7);
    case REG_DMA1CNT_L|0:
      return dma.Read(1, 8);
    case REG_DMA1CNT_L|1:
      return dma.Read(1, 9);
    case REG_DMA1CNT_H|0:
      return dma.Read(1, 10);
    case REG_DMA1CNT_H|1:
      return dma.Read(1, 11);
    case REG_DMA2SAD|0:
      return dma.Read(2, 0);
    case REG_DMA2SAD|1:
      return dma.Read(2, 1);
    case REG_DMA2SAD|2:
      return dma.Read(2, 2);
    case REG_DMA2SAD|3:
      return dma.Read(2, 3);
    case REG_DMA2DAD|0:
      return dma.Read(2, 4);
    case REG_DMA2DAD|1:
      return dma.Read(2, 5);
    case REG_DMA2DAD|2:
      return dma.Read(2, 6);
    case REG_DMA2DAD|3:
      return dma.Read(2, 7);
    case REG_DMA2CNT_L|0:
      return dma.Read(2, 8);
    case REG_DMA2CNT_L|1:
      return dma.Read(2, 9);
    case REG_DMA2CNT_H|0:
      return dma.Read(2, 10);
    case REG_DMA2CNT_H|1:
      return dma.Read(2, 11);
    case REG_DMA3SAD|0:
      return dma.Read(3, 0);
    case REG_DMA3SAD|1:
      return dma.Read(3, 1);
    case REG_DMA3SAD|2:
      return dma.Read(3, 2);
    case REG_DMA3SAD|3:
      return dma.Read(3, 3);
    case REG_DMA3DAD|0:
      return dma.Read(3, 4);
    case REG_DMA3DAD|1:
      return dma.Read(3, 5);
    case REG_DMA3DAD|2:
      return dma.Read(3, 6);
    case REG_DMA3DAD|3:
      return dma.Read(3, 7);
    case REG_DMA3CNT_L|0:
      return dma.Read(3, 8);
    case REG_DMA3CNT_L|1:
      return dma.Read(3, 9);
    case REG_DMA3CNT_H|0:
      return dma.Read(3, 10);
    case REG_DMA3CNT_H|1:
      return dma.Read(3, 11);
    case REG_DMA0FILL|0:
      return dma.ReadFill(0);
    case REG_DMA0FILL|1:
      return dma.ReadFill(1);
    case REG_DMA0FILL|2:
      return dma.ReadFill(2);
    case REG_DMA0FILL|3:
      return dma.ReadFill(3);
    case REG_DMA1FILL|0:
      return dma.ReadFill(4);
    case REG_DMA1FILL|1:
      return dma.ReadFill(5);
    case REG_DMA1FILL|2:
      return dma.ReadFill(6);
    case REG_DMA1FILL|3:
      return dma.ReadFill(7);
    case REG_DMA2FILL|0:
      return dma.ReadFill(8);
    case REG_DMA2FILL|1:
      return dma.ReadFill(9);
    case REG_DMA2FILL|2:
      return dma.ReadFill(10);
    case REG_DMA2FILL|3:
      return dma.ReadFill(11);
    case REG_DMA3FILL|0:
      return dma.ReadFill(12);
    case REG_DMA3FILL|1:
      return dma.ReadFill(13);
    case REG_DMA3FILL|2:
      return dma.ReadFill(14);
    case REG_DMA3FILL|3:
      return dma.ReadFill(15);

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
      return keypad.input.ReadByte(0);
    case REG_KEYINPUT|1:
      return keypad.input.ReadByte(1);
    case REG_KEYCNT|0:
      return keypad.control9.ReadByte(0);
    case REG_KEYCNT|1:
      return keypad.control9.ReadByte(1);  

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
    case REG_AUXSPICNT|0:
      return cart.auxspicnt.ReadByte(0);
    case REG_AUXSPICNT|1:
      return cart.auxspicnt.ReadByte(1);
    case REG_AUXSPIDATA|0:
      return cart.ReadSPI();
    case REG_AUXSPIDATA|1:
      return 0;
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

    case REG_EXMEMCNT|0:
      return exmemcnt.ReadByte(0);
    case REG_EXMEMCNT|1:
      return exmemcnt.ReadByte(1);
    case REG_WRAMCNT:
      return swram.control.ReadByte();

    // Math engine
    case REG_DIVCNT|0:
      return math.divcnt.ReadByte(0);
    case REG_DIVCNT|1:
      return math.divcnt.ReadByte(1);
    case REG_DIV_NUMER|0:
      return math.div_numer.ReadByte(0);
    case REG_DIV_NUMER|1:
      return math.div_numer.ReadByte(1);
    case REG_DIV_NUMER|2:
      return math.div_numer.ReadByte(2);
    case REG_DIV_NUMER|3:
      return math.div_numer.ReadByte(3);
    case REG_DIV_NUMER|4:
      return math.div_numer.ReadByte(4);
    case REG_DIV_NUMER|5:
      return math.div_numer.ReadByte(5);
    case REG_DIV_NUMER|6:
      return math.div_numer.ReadByte(6);
    case REG_DIV_NUMER|7:
      return math.div_numer.ReadByte(7);
    case REG_DIV_DENOM|0:
      return math.div_denom.ReadByte(0);
    case REG_DIV_DENOM|1:
      return math.div_denom.ReadByte(1);
    case REG_DIV_DENOM|2:
      return math.div_denom.ReadByte(2);
    case REG_DIV_DENOM|3:
      return math.div_denom.ReadByte(3);
    case REG_DIV_DENOM|4:
      return math.div_denom.ReadByte(4);
    case REG_DIV_DENOM|5:
      return math.div_denom.ReadByte(5);
    case REG_DIV_DENOM|6:
      return math.div_denom.ReadByte(6);
    case REG_DIV_DENOM|7:
      return math.div_denom.ReadByte(7);
    case REG_DIV_RESULT|0:
      return math.div_result.ReadByte(0);
    case REG_DIV_RESULT|1:
      return math.div_result.ReadByte(1);
    case REG_DIV_RESULT|2:
      return math.div_result.ReadByte(2);
    case REG_DIV_RESULT|3:
      return math.div_result.ReadByte(3);
    case REG_DIV_RESULT|4:
      return math.div_result.ReadByte(4);
    case REG_DIV_RESULT|5:
      return math.div_result.ReadByte(5);
    case REG_DIV_RESULT|6:
      return math.div_result.ReadByte(6);
    case REG_DIV_RESULT|7:
      return math.div_result.ReadByte(7);
    case REG_DIVREM_RESULT|0:
      return math.div_remain.ReadByte(0);
    case REG_DIVREM_RESULT|1:
      return math.div_remain.ReadByte(1);
    case REG_DIVREM_RESULT|2:
      return math.div_remain.ReadByte(2);
    case REG_DIVREM_RESULT|3:
      return math.div_remain.ReadByte(3);
    case REG_DIVREM_RESULT|4:
      return math.div_remain.ReadByte(4);
    case REG_DIVREM_RESULT|5:
      return math.div_remain.ReadByte(5);
    case REG_DIVREM_RESULT|6:
      return math.div_remain.ReadByte(6);
    case REG_DIVREM_RESULT|7:
      return math.div_remain.ReadByte(7);
    case REG_SQRTCNT|0:
      return math.sqrtcnt.ReadByte(0);
    case REG_SQRTCNT|1:
      return math.sqrtcnt.ReadByte(1);
    case REG_SQRT_RESULT|0:
      return math.sqrt_result.ReadByte(0);
    case REG_SQRT_RESULT|1:
      return math.sqrt_result.ReadByte(1);
    case REG_SQRT_RESULT|2:
      return math.sqrt_result.ReadByte(2);
    case REG_SQRT_RESULT|3:
      return math.sqrt_result.ReadByte(3);
    case REG_SQRT_PARAM|0:
      return math.sqrt_param.ReadByte(0);
    case REG_SQRT_PARAM|1:
      return math.sqrt_param.ReadByte(1);
    case REG_SQRT_PARAM|2:
      return math.sqrt_param.ReadByte(2);
    case REG_SQRT_PARAM|3:
      return math.sqrt_param.ReadByte(3);
    case REG_SQRT_PARAM|4:
      return math.sqrt_param.ReadByte(4);
    case REG_SQRT_PARAM|5:
      return math.sqrt_param.ReadByte(5);
    case REG_SQRT_PARAM|6:
      return math.sqrt_param.ReadByte(6);
    case REG_SQRT_PARAM|7:
      return math.sqrt_param.ReadByte(7);
      
    // GPU
    case REG_DISP3DCNT|0:
      return gpu_io.disp3dcnt.ReadByte(0);
    case REG_DISP3DCNT|1:
      return gpu_io.disp3dcnt.ReadByte(1);
    case REG_POWCNT1|0:
      return video_unit.powcnt1.ReadByte(0);
    case REG_POWCNT1|1:
      return video_unit.powcnt1.ReadByte(1);
    case REG_GXSTAT|0:
      return gpu_io.gxstat.ReadByte(0);
    case REG_GXSTAT|1:
      return gpu_io.gxstat.ReadByte(1);
    case REG_GXSTAT|2:
      return gpu_io.gxstat.ReadByte(2);
    case REG_GXSTAT|3:
      return gpu_io.gxstat.ReadByte(3);
    case REG_CLIPMTX_RESULT_LO ... REG_CLIPMTX_RESULT_HI:
      return gpu_io.ReadClipMatrix<u8>(address - REG_CLIPMTX_RESULT_LO);

    case REG_POSTFLG:
      return postflag;

    default:
      LOG_WARN("ARM9: MMIO: unhandled read from 0x{0:08X}", address);
  }

  return 0;
}

auto ARM9MemoryBus::ReadHalfIO(u32 address) -> u16 {
  auto& gpu_io = video_unit.gpu;

  switch (address) {
    case REG_IPCFIFORECV|0:
      return ipc.ipcfiforecv.ReadHalf(IPC::Client::ARM9, 0);
    case REG_IPCFIFORECV|2:
      return ipc.ipcfiforecv.ReadHalf(IPC::Client::ARM9, 2);
    case REG_CLIPMTX_RESULT_LO ... REG_CLIPMTX_RESULT_HI:
      return gpu_io.ReadClipMatrix<u16>(address - REG_CLIPMTX_RESULT_LO);
  }

  return (ReadByteIO(address | 0) << 0) |
         (ReadByteIO(address | 1) << 8);
}

auto ARM9MemoryBus::ReadWordIO(u32 address) -> u32 {
  auto& gpu_io = video_unit.gpu;

  switch (address) {
    case REG_IPCFIFORECV:
      return ipc.ipcfiforecv.ReadWord(IPC::Client::ARM9);
    case REG_CARDDATA:
      return cart.ReadROM();
    case REG_CLIPMTX_RESULT_LO ... REG_CLIPMTX_RESULT_HI:
      return gpu_io.ReadClipMatrix<u32>(address - REG_CLIPMTX_RESULT_LO);
  }

  return (ReadByteIO(address | 0) <<  0) |
         (ReadByteIO(address | 1) <<  8) |
         (ReadByteIO(address | 2) << 16) |
         (ReadByteIO(address | 3) << 24);
}

void ARM9MemoryBus::WriteByteIO(u32 address,  u8 value) {
  auto& gpu_io = video_unit.gpu;
  auto& ppu_io_a = video_unit.ppu_a.mmio;
  auto& ppu_io_b = video_unit.ppu_b.mmio;

  switch (address) {
    // PPU engine A
    case REG_DISPCNT_A|0:
      ppu_io_a.dispcnt.WriteByte(0, value);
      break;
    case REG_DISPCNT_A|1:
      ppu_io_a.dispcnt.WriteByte(1, value);
      break;
    case REG_DISPCNT_A|2:
      ppu_io_a.dispcnt.WriteByte(2, value);
      break;
    case REG_DISPCNT_A|3:
      ppu_io_a.dispcnt.WriteByte(3, value);
      break;
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
    case REG_BG1CNT_A|0:
      ppu_io_a.bgcnt[1].WriteByte(0, value);
      break;
    case REG_BG1CNT_A|1:
      ppu_io_a.bgcnt[1].WriteByte(1, value);
      break;
    case REG_BG2CNT_A|0:
      ppu_io_a.bgcnt[2].WriteByte(0, value);
      break;
    case REG_BG2CNT_A|1:
      ppu_io_a.bgcnt[2].WriteByte(1, value);
      break;
    case REG_BG3CNT_A|0:
      ppu_io_a.bgcnt[3].WriteByte(0, value);
      break;
    case REG_BG3CNT_A|1:
      ppu_io_a.bgcnt[3].WriteByte(1, value);
      break;
    case REG_BG0HOFS_A|0:
      ppu_io_a.bghofs[0].WriteByte(0, value);
      break;
    case REG_BG0HOFS_A|1:
      ppu_io_a.bghofs[0].WriteByte(1, value);
      break;
    case REG_BG0VOFS_A|0:
      ppu_io_a.bgvofs[0].WriteByte(0, value);
      break;
    case REG_BG0VOFS_A|1:
      ppu_io_a.bgvofs[0].WriteByte(1, value);
      break;
    case REG_BG1HOFS_A|0:
      ppu_io_a.bghofs[1].WriteByte(0, value);
      break;
    case REG_BG1HOFS_A|1:
      ppu_io_a.bghofs[1].WriteByte(1, value);
      break;
    case REG_BG1VOFS_A|0:
      ppu_io_a.bgvofs[1].WriteByte(0, value);
      break;
    case REG_BG1VOFS_A|1:
      ppu_io_a.bgvofs[1].WriteByte(1, value);
      break;
    case REG_BG2HOFS_A|0:
      ppu_io_a.bghofs[2].WriteByte(0, value);
      break;
    case REG_BG2HOFS_A|1:
      ppu_io_a.bghofs[2].WriteByte(1, value);
      break;
    case REG_BG2VOFS_A|0:
      ppu_io_a.bgvofs[2].WriteByte(0, value);
      break;
    case REG_BG2VOFS_A|1:
      ppu_io_a.bgvofs[2].WriteByte(1, value);
      break;
    case REG_BG3HOFS_A|0:
      ppu_io_a.bghofs[3].WriteByte(0, value);
      break;
    case REG_BG3HOFS_A|1:
      ppu_io_a.bghofs[3].WriteByte(1, value);
      break;
    case REG_BG3VOFS_A|0:
      ppu_io_a.bgvofs[3].WriteByte(0, value);
      break;
    case REG_BG3VOFS_A|1:
      ppu_io_a.bgvofs[3].WriteByte(1, value);
      break;    
    case REG_BG2PA_A|0:
      ppu_io_a.bgpa[0].WriteByte(0, value);
      break;
    case REG_BG2PA_A|1:
      ppu_io_a.bgpa[0].WriteByte(1, value);
      break;
    case REG_BG2PB_A|0:
      ppu_io_a.bgpb[0].WriteByte(0, value);
      break;
    case REG_BG2PB_A|1:
      ppu_io_a.bgpb[0].WriteByte(1, value);
      break;
    case REG_BG2PC_A|0:
      ppu_io_a.bgpc[0].WriteByte(0, value);
      break;
    case REG_BG2PC_A|1:
      ppu_io_a.bgpc[0].WriteByte(1, value);
      break;
    case REG_BG2PD_A|0:
      ppu_io_a.bgpd[0].WriteByte(0, value);
      break;
    case REG_BG2PD_A|1:
      ppu_io_a.bgpd[0].WriteByte(1, value);
      break;
    case REG_BG2X_A|0:
      ppu_io_a.bgx[0].WriteByte(0, value);
      break;
    case REG_BG2X_A|1:
      ppu_io_a.bgx[0].WriteByte(1, value);
      break;
    case REG_BG2X_A|2:
      ppu_io_a.bgx[0].WriteByte(2, value);
      break;
    case REG_BG2X_A|3:
      ppu_io_a.bgx[0].WriteByte(3, value);
      break;
    case REG_BG2Y_A|0:
      ppu_io_a.bgy[0].WriteByte(0, value);
      break;
    case REG_BG2Y_A|1:
      ppu_io_a.bgy[0].WriteByte(1, value);
      break;
    case REG_BG2Y_A|2:
      ppu_io_a.bgy[0].WriteByte(2, value);
      break;
    case REG_BG2Y_A|3:
      ppu_io_a.bgy[0].WriteByte(3, value);
      break;
    case REG_BG3PA_A|0:
      ppu_io_a.bgpa[1].WriteByte(0, value);
      break;
    case REG_BG3PA_A|1:
      ppu_io_a.bgpa[1].WriteByte(1, value);
      break;
    case REG_BG3PB_A|0:
      ppu_io_a.bgpb[1].WriteByte(0, value);
      break;
    case REG_BG3PB_A|1:
      ppu_io_a.bgpb[1].WriteByte(1, value);
      break;
    case REG_BG3PC_A|0:
      ppu_io_a.bgpc[1].WriteByte(0, value);
      break;
    case REG_BG3PC_A|1:
      ppu_io_a.bgpc[1].WriteByte(1, value);
      break;
    case REG_BG3PD_A|0:
      ppu_io_a.bgpd[1].WriteByte(0, value);
      break;
    case REG_BG3PD_A|1:
      ppu_io_a.bgpd[1].WriteByte(1, value);
      break;
    case REG_BG3X_A|0:
      ppu_io_a.bgx[1].WriteByte(0, value);
      break;
    case REG_BG3X_A|1:
      ppu_io_a.bgx[1].WriteByte(1, value);
      break;
    case REG_BG3X_A|2:
      ppu_io_a.bgx[1].WriteByte(2, value);
      break;
    case REG_BG3X_A|3:
      ppu_io_a.bgx[1].WriteByte(3, value);
      break;
    case REG_BG3Y_A|0:
      ppu_io_a.bgy[1].WriteByte(0, value);
      break;
    case REG_BG3Y_A|1:
      ppu_io_a.bgy[1].WriteByte(1, value);
      break;
    case REG_BG3Y_A|2:
      ppu_io_a.bgy[1].WriteByte(2, value);
      break;
    case REG_BG3Y_A|3:
      ppu_io_a.bgy[1].WriteByte(3, value);
      break;
    case REG_WIN0H_A|0:
      ppu_io_a.winh[0].WriteByte(0, value);
      break;
    case REG_WIN0H_A|1:
      ppu_io_a.winh[0].WriteByte(1, value);
      break;
    case REG_WIN1H_A|0:
      ppu_io_a.winh[1].WriteByte(0, value);
      break;
    case REG_WIN1H_A|1:
      ppu_io_a.winh[1].WriteByte(1, value);
      break;
    case REG_WIN0V_A|0:
      ppu_io_a.winv[0].WriteByte(0, value);
      break;
    case REG_WIN0V_A|1:
      ppu_io_a.winv[0].WriteByte(1, value);
      break;
    case REG_WIN1V_A|0:
      ppu_io_a.winv[1].WriteByte(0, value);
      break;
    case REG_WIN1V_A|1:
      ppu_io_a.winv[1].WriteByte(1, value);
      break;
    case REG_WININ_A|0:
      ppu_io_a.winin.WriteByte(0, value);
      break;
    case REG_WININ_A|1:
      ppu_io_a.winin.WriteByte(1, value);
      break;
    case REG_WINOUT_A|0:
      ppu_io_a.winout.WriteByte(0, value);
      break;
    case REG_WINOUT_A|1:
      ppu_io_a.winout.WriteByte(1, value);
      break;
    case REG_MOSAIC_A|0:
      ppu_io_a.mosaic.WriteByte(0, value);
      break;
    case REG_MOSAIC_A|1:
      ppu_io_a.mosaic.WriteByte(1, value);
      break;
    case REG_MOSAIC_A|2:
    case REG_MOSAIC_A|3:
      break;
    case REG_BLDCNT_A|0:
      ppu_io_a.bldcnt.WriteByte(0, value);
      break;
    case REG_BLDCNT_A|1:
      ppu_io_a.bldcnt.WriteByte(1, value);
      break;
    case REG_BLDALPHA_A|0:
      ppu_io_a.bldalpha.WriteByte(0, value);
      break;
    case REG_BLDALPHA_A|1:
      ppu_io_a.bldalpha.WriteByte(1, value);
      break;
    case REG_BLDY_A|0:
      ppu_io_a.bldy.WriteByte(0, value);
      break;
    case REG_BLDY_A|1:
    case REG_BLDY_A|2:
    case REG_BLDY_A|3:
      break;
    case REG_DISPCAPCNT|0:
      video_unit.dispcapcnt.WriteByte(0, value);
      break;
    case REG_DISPCAPCNT|1:
      video_unit.dispcapcnt.WriteByte(1, value);
      break;
    case REG_DISPCAPCNT|2:
      video_unit.dispcapcnt.WriteByte(2, value);
      break;
    case REG_DISPCAPCNT|3:
      video_unit.dispcapcnt.WriteByte(3, value);
      break;
    case REG_MASTER_BRIGHT_A|0:
      ppu_io_a.master_bright.WriteByte(0, value);
      break;
    case REG_MASTER_BRIGHT_A|1:
      ppu_io_a.master_bright.WriteByte(1, value);
      break;

    // PPU engine B
    case REG_DISPCNT_B|0:
      ppu_io_b.dispcnt.WriteByte(0, value);
      break;
    case REG_DISPCNT_B|1:
      ppu_io_b.dispcnt.WriteByte(1, value);
      break;
    case REG_DISPCNT_B|2:
      ppu_io_b.dispcnt.WriteByte(2, value);
      break;
    case REG_DISPCNT_B|3:
      ppu_io_b.dispcnt.WriteByte(3, value);
      break;
    case REG_BG0CNT_B|0:
      ppu_io_b.bgcnt[0].WriteByte(0, value);
      break;
    case REG_BG0CNT_B|1:
      ppu_io_b.bgcnt[0].WriteByte(1, value);
      break;
    case REG_BG1CNT_B|0:
      ppu_io_b.bgcnt[1].WriteByte(0, value);
      break;
    case REG_BG1CNT_B|1:
      ppu_io_b.bgcnt[1].WriteByte(1, value);
      break;
    case REG_BG2CNT_B|0:
      ppu_io_b.bgcnt[2].WriteByte(0, value);
      break;
    case REG_BG2CNT_B|1:
      ppu_io_b.bgcnt[2].WriteByte(1, value);
      break;
    case REG_BG3CNT_B|0:
      ppu_io_b.bgcnt[3].WriteByte(0, value);
      break;
    case REG_BG3CNT_B|1:
      ppu_io_b.bgcnt[3].WriteByte(1, value);
      break;
    case REG_BG0HOFS_B|0:
      ppu_io_b.bghofs[0].WriteByte(0, value);
      break;
    case REG_BG0HOFS_B|1:
      ppu_io_b.bghofs[0].WriteByte(1, value);
      break;
    case REG_BG0VOFS_B|0:
      ppu_io_b.bgvofs[0].WriteByte(0, value);
      break;
    case REG_BG0VOFS_B|1:
      ppu_io_b.bgvofs[0].WriteByte(1, value);
      break;
    case REG_BG1HOFS_B|0:
      ppu_io_b.bghofs[1].WriteByte(0, value);
      break;
    case REG_BG1HOFS_B|1:
      ppu_io_b.bghofs[1].WriteByte(1, value);
      break;
    case REG_BG1VOFS_B|0:
      ppu_io_b.bgvofs[1].WriteByte(0, value);
      break;
    case REG_BG1VOFS_B|1:
      ppu_io_b.bgvofs[1].WriteByte(1, value);
      break;
    case REG_BG2HOFS_B|0:
      ppu_io_b.bghofs[2].WriteByte(0, value);
      break;
    case REG_BG2HOFS_B|1:
      ppu_io_b.bghofs[2].WriteByte(1, value);
      break;
    case REG_BG2VOFS_B|0:
      ppu_io_b.bgvofs[2].WriteByte(0, value);
      break;
    case REG_BG2VOFS_B|1:
      ppu_io_b.bgvofs[2].WriteByte(1, value);
      break;
    case REG_BG3HOFS_B|0:
      ppu_io_b.bghofs[3].WriteByte(0, value);
      break;
    case REG_BG3HOFS_B|1:
      ppu_io_b.bghofs[3].WriteByte(1, value);
      break;
    case REG_BG3VOFS_B|0:
      ppu_io_b.bgvofs[3].WriteByte(0, value);
      break;
    case REG_BG3VOFS_B|1:
      ppu_io_b.bgvofs[3].WriteByte(1, value);
      break;
    case REG_BG2PA_B|0:
      ppu_io_b.bgpa[0].WriteByte(0, value);
      break;
    case REG_BG2PA_B|1:
      ppu_io_b.bgpa[0].WriteByte(1, value);
      break;
    case REG_BG2PB_B|0:
      ppu_io_b.bgpb[0].WriteByte(0, value);
      break;
    case REG_BG2PB_B|1:
      ppu_io_b.bgpb[0].WriteByte(1, value);
      break;
    case REG_BG2PC_B|0:
      ppu_io_b.bgpc[0].WriteByte(0, value);
      break;
    case REG_BG2PC_B|1:
      ppu_io_b.bgpc[0].WriteByte(1, value);
      break;
    case REG_BG2PD_B|0:
      ppu_io_b.bgpd[0].WriteByte(0, value);
      break;
    case REG_BG2PD_B|1:
      ppu_io_b.bgpd[0].WriteByte(1, value);
      break;
    case REG_BG2X_B|0:
      ppu_io_b.bgx[0].WriteByte(0, value);
      break;
    case REG_BG2X_B|1:
      ppu_io_b.bgx[0].WriteByte(1, value);
      break;
    case REG_BG2X_B|2:
      ppu_io_b.bgx[0].WriteByte(2, value);
      break;
    case REG_BG2X_B|3:
      ppu_io_b.bgx[0].WriteByte(3, value);
      break;
    case REG_BG2Y_B|0:
      ppu_io_b.bgy[0].WriteByte(0, value);
      break;
    case REG_BG2Y_B|1:
      ppu_io_b.bgy[0].WriteByte(1, value);
      break;
    case REG_BG2Y_B|2:
      ppu_io_b.bgy[0].WriteByte(2, value);
      break;
    case REG_BG2Y_B|3:
      ppu_io_b.bgy[0].WriteByte(3, value);
      break;
    case REG_BG3PA_B|0:
      ppu_io_b.bgpa[1].WriteByte(0, value);
      break;
    case REG_BG3PA_B|1:
      ppu_io_b.bgpa[1].WriteByte(1, value);
      break;
    case REG_BG3PB_B|0:
      ppu_io_b.bgpb[1].WriteByte(0, value);
      break;
    case REG_BG3PB_B|1:
      ppu_io_b.bgpb[1].WriteByte(1, value);
      break;
    case REG_BG3PC_B|0:
      ppu_io_b.bgpc[1].WriteByte(0, value);
      break;
    case REG_BG3PC_B|1:
      ppu_io_b.bgpc[1].WriteByte(1, value);
      break;
    case REG_BG3PD_B|0:
      ppu_io_b.bgpd[1].WriteByte(0, value);
      break;
    case REG_BG3PD_B|1:
      ppu_io_b.bgpd[1].WriteByte(1, value);
      break;
    case REG_BG3X_B|0:
      ppu_io_b.bgx[1].WriteByte(0, value);
      break;
    case REG_BG3X_B|1:
      ppu_io_b.bgx[1].WriteByte(1, value);
      break;
    case REG_BG3X_B|2:
      ppu_io_b.bgx[1].WriteByte(2, value);
      break;
    case REG_BG3X_B|3:
      ppu_io_b.bgx[1].WriteByte(3, value);
      break;
    case REG_BG3Y_B|0:
      ppu_io_b.bgy[1].WriteByte(0, value);
      break;
    case REG_BG3Y_B|1:
      ppu_io_b.bgy[1].WriteByte(1, value);
      break;
    case REG_BG3Y_B|2:
      ppu_io_b.bgy[1].WriteByte(2, value);
      break;
    case REG_BG3Y_B|3:
      ppu_io_b.bgy[1].WriteByte(3, value);
      break;
    case REG_WIN0H_B|0:
      ppu_io_b.winh[0].WriteByte(0, value);
      break;
    case REG_WIN0H_B|1:
      ppu_io_b.winh[0].WriteByte(1, value);
      break;
    case REG_WIN1H_B|0:
      ppu_io_b.winh[1].WriteByte(0, value);
      break;
    case REG_WIN1H_B|1:
      ppu_io_b.winh[1].WriteByte(1, value);
      break;
    case REG_WIN0V_B|0:
      ppu_io_b.winv[0].WriteByte(0, value);
      break;
    case REG_WIN0V_B|1:
      ppu_io_b.winv[0].WriteByte(1, value);
      break;
    case REG_WIN1V_B|0:
      ppu_io_b.winv[1].WriteByte(0, value);
      break;
    case REG_WIN1V_B|1:
      ppu_io_b.winv[1].WriteByte(1, value);
      break;
    case REG_WININ_B|0:
      ppu_io_b.winin.WriteByte(0, value);
      break;
    case REG_WININ_B|1:
      ppu_io_b.winin.WriteByte(1, value);
      break;
    case REG_WINOUT_B|0:
      ppu_io_b.winout.WriteByte(0, value);
      break;
    case REG_WINOUT_B|1:
      ppu_io_b.winout.WriteByte(1, value);
      break;
    case REG_MOSAIC_B|0:
      ppu_io_b.mosaic.WriteByte(0, value);
      break;
    case REG_MOSAIC_B|1:
      ppu_io_b.mosaic.WriteByte(1, value);
      break;
    case REG_MOSAIC_B|2:
    case REG_MOSAIC_B|3:
      break;
    case REG_BLDCNT_B|0:
      ppu_io_b.bldcnt.WriteByte(0, value);
      break;
    case REG_BLDCNT_B|1:
      ppu_io_b.bldcnt.WriteByte(1, value);
      break;
    case REG_BLDALPHA_B|0:
      ppu_io_b.bldalpha.WriteByte(0, value);
      break;
    case REG_BLDALPHA_B|1:
      ppu_io_b.bldalpha.WriteByte(1, value);
      break;
    case REG_BLDY_B|0:
      ppu_io_b.bldy.WriteByte(0, value);
      break;
    case REG_BLDY_B|1:
    case REG_BLDY_B|2:
    case REG_BLDY_B|3:
      break;
    case REG_MASTER_BRIGHT_B|0:
      ppu_io_b.master_bright.WriteByte(0, value);
      break;
    case REG_MASTER_BRIGHT_B|1:
      ppu_io_b.master_bright.WriteByte(1, value);
      break;

    // DMA
    case REG_DMA0SAD|0:
      dma.Write(0, 0, value);
      break;
    case REG_DMA0SAD|1:
      dma.Write(0, 1, value);
      break;
    case REG_DMA0SAD|2:
      dma.Write(0, 2, value);
      break;
    case REG_DMA0SAD|3:
      dma.Write(0, 3, value);
      break;
    case REG_DMA0DAD|0:
      dma.Write(0, 4, value);
      break;
    case REG_DMA0DAD|1:
      dma.Write(0, 5, value);
      break;
    case REG_DMA0DAD|2:
      dma.Write(0, 6, value);
      break;
    case REG_DMA0DAD|3:
      dma.Write(0, 7, value);
      break;
    case REG_DMA0CNT_L|0:
      dma.Write(0, 8, value);
      break;
    case REG_DMA0CNT_L|1:
      dma.Write(0, 9, value);
      break;
    case REG_DMA0CNT_H|0:
      dma.Write(0, 10, value);
      break;
    case REG_DMA0CNT_H|1:
      dma.Write(0, 11, value);
      break;
    case REG_DMA1SAD|0:
      dma.Write(1, 0, value);
      break;
    case REG_DMA1SAD|1:
      dma.Write(1, 1, value);
      break;
    case REG_DMA1SAD|2:
      dma.Write(1, 2, value);
      break;
    case REG_DMA1SAD|3:
      dma.Write(1, 3, value);
      break;
    case REG_DMA1DAD|0:
      dma.Write(1, 4, value);
      break;
    case REG_DMA1DAD|1:
      dma.Write(1, 5, value);
      break;
    case REG_DMA1DAD|2:
      dma.Write(1, 6, value);
      break;
    case REG_DMA1DAD|3:
      dma.Write(1, 7, value);
      break;
    case REG_DMA1CNT_L|0:
      dma.Write(1, 8, value);
      break;
    case REG_DMA1CNT_L|1:
      dma.Write(1, 9, value);
      break;
    case REG_DMA1CNT_H|0:
      dma.Write(1, 10, value);
      break;
    case REG_DMA1CNT_H|1:
      dma.Write(1, 11, value);
      break;
    case REG_DMA2SAD|0:
      dma.Write(2, 0, value);
      break;
    case REG_DMA2SAD|1:
      dma.Write(2, 1, value);
      break;
    case REG_DMA2SAD|2:
      dma.Write(2, 2, value);
      break;
    case REG_DMA2SAD|3:
      dma.Write(2, 3, value);
      break;
    case REG_DMA2DAD|0:
      dma.Write(2, 4, value);
      break;
    case REG_DMA2DAD|1:
      dma.Write(2, 5, value);
      break;
    case REG_DMA2DAD|2:
      dma.Write(2, 6, value);
      break;
    case REG_DMA2DAD|3:
      dma.Write(2, 7, value);
      break;
    case REG_DMA2CNT_L|0:
      dma.Write(2, 8, value);
      break;
    case REG_DMA2CNT_L|1:
      dma.Write(2, 9, value);
      break;
    case REG_DMA2CNT_H|0:
      dma.Write(2, 10, value);
      break;
    case REG_DMA2CNT_H|1:
      dma.Write(2, 11, value);
      break;
    case REG_DMA3SAD|0:
      dma.Write(3, 0, value);
      break;
    case REG_DMA3SAD|1:
      dma.Write(3, 1, value);
      break;
    case REG_DMA3SAD|2:
      dma.Write(3, 2, value);
      break;
    case REG_DMA3SAD|3:
      dma.Write(3, 3, value);
      break;
    case REG_DMA3DAD|0:
      dma.Write(3, 4, value);
      break;
    case REG_DMA3DAD|1:
      dma.Write(3, 5, value);
      break;
    case REG_DMA3DAD|2:
      dma.Write(3, 6, value);
      break;
    case REG_DMA3DAD|3:
      dma.Write(3, 7, value);
      break;
    case REG_DMA3CNT_L|0:
      dma.Write(3, 8, value);
      break;
    case REG_DMA3CNT_L|1:
      dma.Write(3, 9, value);
      break;
    case REG_DMA3CNT_H|0:
      dma.Write(3, 10, value);
      break;
    case REG_DMA3CNT_H|1:
      dma.Write(3, 11, value);
      break;
    case REG_DMA0FILL|0:
      dma.WriteFill(0, value);
      break;
    case REG_DMA0FILL|1:
      dma.WriteFill(1, value);
      break;
    case REG_DMA0FILL|2:
      dma.WriteFill(2, value);
      break;
    case REG_DMA0FILL|3:
      dma.WriteFill(3, value);
      break;
    case REG_DMA1FILL|0:
      dma.WriteFill(4, value);
      break;
    case REG_DMA1FILL|1:
      dma.WriteFill(5, value);
      break;
    case REG_DMA1FILL|2:
      dma.WriteFill(6, value);
      break;
    case REG_DMA1FILL|3:
      dma.WriteFill(7, value);
      break;
    case REG_DMA2FILL|0:
      dma.WriteFill(8, value);
      break;
    case REG_DMA2FILL|1:
      dma.WriteFill(9, value);
      break;
    case REG_DMA2FILL|2:
      dma.WriteFill(10, value);
      break;
    case REG_DMA2FILL|3:
      dma.WriteFill(11, value);
      break;
    case REG_DMA3FILL|0:
      dma.WriteFill(12, value);
      break;
    case REG_DMA3FILL|1:
      dma.WriteFill(13, value);
      break;
    case REG_DMA3FILL|2:
      dma.WriteFill(14, value);
      break;
    case REG_DMA3FILL|3:
      dma.WriteFill(15, value);
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

    // Input
    case REG_KEYCNT|0:
      keypad.control9.WriteByte(0, value);
      break;
    case REG_KEYCNT|1:
      keypad.control9.WriteByte(1, value);
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
    case REG_AUXSPICNT|0:
      cart.auxspicnt.WriteByte(0, value);
      break;
    case REG_AUXSPICNT|1:
      cart.auxspicnt.WriteByte(1, value);
      break;
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
    case REG_EXMEMCNT|0:
      exmemcnt.WriteByte(0, value);
      break;
    case REG_EXMEMCNT|1:
      exmemcnt.WriteByte(1, value);
      break;
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
      swram.control.WriteByte(value);
      break;
    case REG_VRAMCNT_H:
      vram.vramcnt_h.WriteByte(value);
      break;
    case REG_VRAMCNT_I:
      vram.vramcnt_i.WriteByte(value);
      break;

    // Math engine
    case REG_DIVCNT|0:
      math.divcnt.WriteByte(0, value);
      break;
    case REG_DIVCNT|1:
      math.divcnt.WriteByte(1, value);
      break;
    case REG_DIV_NUMER|0:
      math.div_numer.WriteByte(0, value);
      break;
    case REG_DIV_NUMER|1:
      math.div_numer.WriteByte(1, value);
      break;
    case REG_DIV_NUMER|2:
      math.div_numer.WriteByte(2, value);
      break;
    case REG_DIV_NUMER|3:
      math.div_numer.WriteByte(3, value);
      break;
    case REG_DIV_NUMER|4:
      math.div_numer.WriteByte(4, value);
      break;
    case REG_DIV_NUMER|5:
      math.div_numer.WriteByte(5, value);
      break;
    case REG_DIV_NUMER|6:
      math.div_numer.WriteByte(6, value);
      break;
    case REG_DIV_NUMER|7:
      math.div_numer.WriteByte(7, value);
      break;
    case REG_DIV_DENOM|0:
      math.div_denom.WriteByte(0, value);
      break;
    case REG_DIV_DENOM|1:
      math.div_denom.WriteByte(1, value);
      break;
    case REG_DIV_DENOM|2:
      math.div_denom.WriteByte(2, value);
      break;
    case REG_DIV_DENOM|3:
      math.div_denom.WriteByte(3, value);
      break;
    case REG_DIV_DENOM|4:
      math.div_denom.WriteByte(4, value);
      break;
    case REG_DIV_DENOM|5:
      math.div_denom.WriteByte(5, value);
      break;
    case REG_DIV_DENOM|6:
      math.div_denom.WriteByte(6, value);
      break;
    case REG_DIV_DENOM|7:
      math.div_denom.WriteByte(7, value);
      break;
    case REG_SQRTCNT|0:
      math.sqrtcnt.WriteByte(0, value);
      break;
    case REG_SQRTCNT|1:
      math.sqrtcnt.WriteByte(1, value);
      break;
    case REG_SQRT_PARAM|0:
      math.sqrt_param.WriteByte(0, value);
      break;
    case REG_SQRT_PARAM|1:
      math.sqrt_param.WriteByte(1, value);
      break;
    case REG_SQRT_PARAM|2:
      math.sqrt_param.WriteByte(2, value);
      break;
    case REG_SQRT_PARAM|3:
      math.sqrt_param.WriteByte(3, value);
      break;
    case REG_SQRT_PARAM|4:
      math.sqrt_param.WriteByte(4, value);
      break;
    case REG_SQRT_PARAM|5:
      math.sqrt_param.WriteByte(5, value);
      break;
    case REG_SQRT_PARAM|6:
      math.sqrt_param.WriteByte(6, value);
      break;
    case REG_SQRT_PARAM|7:
      math.sqrt_param.WriteByte(7, value);
      break;

    // GPU
    case REG_DISP3DCNT|0:
      gpu_io.disp3dcnt.WriteByte(0, value);
      break;
    case REG_DISP3DCNT|1:
      gpu_io.disp3dcnt.WriteByte(1, value);
      break;
    case REG_POWCNT1|0:
      video_unit.powcnt1.WriteByte(0, value);
      break;
    case REG_POWCNT1|1:
      video_unit.powcnt1.WriteByte(1, value);
      break;
    case REG_EDGE_COLOR_LO ... REG_EDGE_COLOR_HI:
      gpu_io.WriteEdgeColorTable(address & 0xF, value);
      break;
    case REG_ALPHA_TEST_REF:
      gpu_io.alpha_test_ref.WriteByte(value);
      break;
    case REG_CLEAR_COLOR|0:
      gpu_io.clear_color.WriteByte(0, value);
      break;
    case REG_CLEAR_COLOR|1:
      gpu_io.clear_color.WriteByte(1, value);
      break;
    case REG_CLEAR_COLOR|2:
      gpu_io.clear_color.WriteByte(2, value);
      break;
    case REG_CLEAR_COLOR|3:
      gpu_io.clear_color.WriteByte(3, value);
      break;
    case REG_CLEAR_DEPTH|0:
      gpu_io.clear_depth.WriteByte(0, value);
      break;
    case REG_CLEAR_DEPTH|1:
      gpu_io.clear_depth.WriteByte(1, value);
      break;
    case REG_CLRIMAGE_OFFSET|0:
      gpu_io.clrimage_offset.WriteByte(0, value);
      break;
    case REG_CLRIMAGE_OFFSET|1:
      gpu_io.clrimage_offset.WriteByte(1, value);
      break;
    case REG_FOG_COLOR|0:
      gpu_io.fog_color.WriteByte(0, value);
      break;
    case REG_FOG_COLOR|1:
      gpu_io.fog_color.WriteByte(1, value);
      break;
    case REG_FOG_COLOR|2:
      gpu_io.fog_color.WriteByte(2, value);
      break;
    case REG_FOG_COLOR|3:
      break;
    case REG_FOG_OFFSET|0:
      gpu_io.fog_offset.WriteByte(0, value);
      break;
    case REG_FOG_OFFSET|1:
      gpu_io.fog_offset.WriteByte(1, value);
      break;
    case REG_FOG_TABLE_LO ... REG_FOG_TABLE_HI:
      gpu_io.WriteFogDensityTable(address & 0x1F, value);
      break;
    case REG_TOON_TABLE_LO ... REG_TOON_TABLE_HI:
      gpu_io.WriteToonTable(address & 0x3F, value);
      break;
    case REG_GXSTAT|0:
      gpu_io.gxstat.WriteByte(0, value);
      break;
    case REG_GXSTAT|1:
      gpu_io.gxstat.WriteByte(1, value);
      break;
    case REG_GXSTAT|2:
      gpu_io.gxstat.WriteByte(2, value);
      break;
    case REG_GXSTAT|3:
      gpu_io.gxstat.WriteByte(3, value);
      break;

    case REG_POSTFLG:
      // Bit 0: post boot flag (write-once, cannot be unset)
      // Bit 1: undocumented (read- and writeable)
      postflag = (postflag & 1) | (value & 3);
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
    case REG_KEYCNT:
      keypad.control9.WriteHalf(value);
      break;
    default:
      WriteByteIO(address | 0, value & 0xFF);
      WriteByteIO(address | 1, value >> 8);
      break;
  }
}

void ARM9MemoryBus::WriteWordIO(u32 address, u32 value) {
  switch (address) {
    // IPC
    case REG_IPCFIFOSEND:
      ipc.ipcfifosend.WriteWord(IPC::Client::ARM9, value);
      break;
      
    // GPU
    case REG_GXFIFO_LO ... REG_GXFIFO_HI:
      video_unit.gpu.WriteGXFIFO(value);
      break;
    case REG_GXCMDPORT_LO ... REG_GXCMDPORT_HI:
      video_unit.gpu.WriteCommandPort(address & 0x1FF, value);
      break;
    default:
      WriteByteIO(address | 0, u8(value >>  0));
      WriteByteIO(address | 1, u8(value >>  8));
      WriteByteIO(address | 2, u8(value >> 16));
      WriteByteIO(address | 3, u8(value >> 24));
      break;
  }
}

} // namespace lunar::nds
