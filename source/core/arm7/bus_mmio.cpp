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

  // SPI
  REG_SPICNT = 0x0400'01C0,
  REG_SPIDATA = 0x0400'01C2,

  // IRQ
  REG_IME = 0x0400'0208,
  REG_IE  = 0x0400'0210,
  REG_IF  = 0x0400'0214,

  REG_VRAMSTAT = 0x0400'0240,
  REG_WRAMSTAT = 0x0400'0241,

  REG_HALTCNT = 0x0400'0301
};

static bool gIssuedHaltcntWarning = false;

auto ARM7MemoryBus::ReadByteIO(u32 address) -> u8 {
  switch (address) {
    // PPU engine A
    case REG_DISPSTAT|0:
      return video_unit.dispstat7.ReadByte(0);
    case REG_DISPSTAT|1:
      return video_unit.dispstat7.ReadByte(1);
    case REG_VCOUNT|0:
      return video_unit.vcount.ReadByte(0);
    case REG_VCOUNT|1:
      return video_unit.vcount.ReadByte(1);

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
      return ipc.ipcsync.ReadByte(IPC::Client::ARM7, 0);
    case REG_IPCSYNC|1:
      return ipc.ipcsync.ReadByte(IPC::Client::ARM7, 1);
    case REG_IPCFIFOCNT|0:
      return ipc.ipcfifocnt.ReadByte(IPC::Client::ARM7, 0);
    case REG_IPCFIFOCNT|1:
      return ipc.ipcfifocnt.ReadByte(IPC::Client::ARM7, 1);
    case REG_IPCFIFORECV|0:
      return ipc.ipcfiforecv.ReadByte(IPC::Client::ARM7, 0);
    case REG_IPCFIFORECV|1:
      return ipc.ipcfiforecv.ReadByte(IPC::Client::ARM7, 1);
    case REG_IPCFIFORECV|2:
      return ipc.ipcfiforecv.ReadByte(IPC::Client::ARM7, 2);
    case REG_IPCFIFORECV|3:
      return ipc.ipcfiforecv.ReadByte(IPC::Client::ARM7, 3);

    // SPI
    case REG_SPICNT|0:
      return spi.spicnt.ReadByte(0);
    case REG_SPICNT|1:
      return spi.spicnt.ReadByte(1);
    case REG_SPIDATA|0:
      return spi.spidata.ReadByte();
    case REG_SPIDATA|1:
      // not functional/used but accessed anyways.
      return 0;

    // IRQ
    case REG_IME|0:
      return irq7.ime.ReadByte(0);
    case REG_IME|1:
      return irq7.ime.ReadByte(1);
    case REG_IME|2:
      return irq7.ime.ReadByte(2);
    case REG_IME|3:
      return irq7.ime.ReadByte(3);
    case REG_IE|0:
      return irq7.ie.ReadByte(0);
    case REG_IE|1:
      return irq7.ie.ReadByte(1);
    case REG_IE|2:
      return irq7.ie.ReadByte(2);
    case REG_IE|3:
      return irq7.ie.ReadByte(3);
    case REG_IF|0:
      return irq7._if.ReadByte(0);
    case REG_IF|1:
      return irq7._if.ReadByte(1);
    case REG_IF|2:
      return irq7._if.ReadByte(2);
    case REG_IF|3:
      return irq7._if.ReadByte(3);

    case REG_VRAMSTAT:
      return vram.vramstat.ReadByte();
    case REG_WRAMSTAT:
      return wramcnt.ReadByte();

    default:
      LOG_WARN("ARM7: MMIO: unhandled read from 0x{0:08X}", address);
  }

  return 0;
}

auto ARM7MemoryBus::ReadHalfIO(u32 address) -> u16 {
  switch (address) {
    case REG_IPCFIFORECV|0:
      return ipc.ipcfiforecv.ReadHalf(IPC::Client::ARM7, 0);
    case REG_IPCFIFORECV|2:
      return ipc.ipcfiforecv.ReadHalf(IPC::Client::ARM7, 2);
  }

  return (ReadByteIO(address | 0) << 0) |
         (ReadByteIO(address | 1) << 8);
}

auto ARM7MemoryBus::ReadWordIO(u32 address) -> u32 {
  switch (address) {
    case REG_IPCFIFORECV:
      return ipc.ipcfiforecv.ReadWord(IPC::Client::ARM7);
  }

  return (ReadByteIO(address | 0) <<  0) |
         (ReadByteIO(address | 1) <<  8) |
         (ReadByteIO(address | 2) << 16) |
         (ReadByteIO(address | 3) << 24);
}

void ARM7MemoryBus::WriteByteIO(u32 address,  u8 value) {
  switch (address) {
    // PPU engine A
    case REG_DISPSTAT|0:
      video_unit.dispstat7.WriteByte(0, value);
      break;
    case REG_DISPSTAT|1:
      video_unit.dispstat7.WriteByte(1, value);
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
      ipc.ipcsync.WriteByte(IPC::Client::ARM7, 0, value);
      break;
    case REG_IPCSYNC|1:
      ipc.ipcsync.WriteByte(IPC::Client::ARM7, 1, value);
      break;
    case REG_IPCFIFOCNT|0:
      ipc.ipcfifocnt.WriteByte(IPC::Client::ARM7, 0, value);
      break;
    case REG_IPCFIFOCNT|1:
      ipc.ipcfifocnt.WriteByte(IPC::Client::ARM7, 1, value);
      break;
    case REG_IPCFIFOSEND|0:
    case REG_IPCFIFOSEND|1:
    case REG_IPCFIFOSEND|2:
    case REG_IPCFIFOSEND|3:
      ipc.ipcfifosend.WriteByte(IPC::Client::ARM7, value);
      break;

    // SPI
    case REG_SPICNT|0:
      spi.spicnt.WriteByte(0, value);
      break;
    case REG_SPICNT|1:
      spi.spicnt.WriteByte(1, value);
      break;
    case REG_SPIDATA|0:
      spi.spidata.WriteByte(value);
      break;
    case REG_SPIDATA|1:
      // not functional/used but accessed anyways.
      break;

    // IRQ
    case REG_IME|0:
      irq7.ime.WriteByte(0, value);
      break;
    case REG_IME|1:
      irq7.ime.WriteByte(1, value);
      break;
    case REG_IME|2:
      irq7.ime.WriteByte(2, value);
      break;
    case REG_IME|3:
      irq7.ime.WriteByte(3, value);
      break;
    case REG_IE|0:
      irq7.ie.WriteByte(0, value);
      break;
    case REG_IE|1:
      irq7.ie.WriteByte(1, value);
      break;
    case REG_IE|2:
      irq7.ie.WriteByte(2, value);
      break;
    case REG_IE|3:
      irq7.ie.WriteByte(3, value);
      break;
    case REG_IF|0:
      irq7._if.WriteByte(0, value);
      break;
    case REG_IF|1:
      irq7._if.WriteByte(1, value);
      break;
    case REG_IF|2:
      irq7._if.WriteByte(2, value);
      break;
    case REG_IF|3:
      irq7._if.WriteByte(3, value);
      break;

    case REG_HALTCNT:
      if (!gIssuedHaltcntWarning)
        LOG_WARN("ARM7: MMIO: unhandled write to HALTCNT. will not report about this again.");
      gIssuedHaltcntWarning = true;
      break;

    default:
      LOG_WARN("ARM7: MMIO: unhandled write to 0x{0:08X} = 0x{1:02X}", address, value);
  }
}

void ARM7MemoryBus::WriteHalfIO(u32 address, u16 value) {
  switch (address) {
    case REG_IPCFIFOSEND|0:
    case REG_IPCFIFOSEND|2:
      ipc.ipcfifosend.WriteHalf(IPC::Client::ARM7, value);
      break;
    default:
      WriteByteIO(address | 0, value & 0xFF);
      WriteByteIO(address | 1, value >> 8);
      break;
  }
}

void ARM7MemoryBus::WriteWordIO(u32 address, u32 value) {
  switch (address) {
    case REG_IPCFIFOSEND:
      ipc.ipcfifosend.WriteWord(IPC::Client::ARM7, value);
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
