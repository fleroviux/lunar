/*
 * Copyright (C) 2020 fleroviux
 */

#include <util/log.hpp>

#include "ipc.hpp"

namespace Duality::core {

IPC::IPC(IRQ& irq7, IRQ& irq9) {
  irq[static_cast<uint>(Client::ARM7)] = &irq7;
  irq[static_cast<uint>(Client::ARM9)] = &irq9;
  Reset();
}

void IPC::Reset() {
  sync[static_cast<uint>(Client::ARM7)] = {};
  sync[static_cast<uint>(Client::ARM9)] = {};
  fifo[static_cast<uint>(Client::ARM7)] = {};
  fifo[static_cast<uint>(Client::ARM9)] = {};
}

void IPC::RequestIRQ(Client client, IRQ::Source reason) {
  irq[static_cast<uint>(client)]->Raise(reason);
}

auto IPC::IPCSYNC::ReadByte(Client client, uint offset) -> u8 {
  auto& sync_tx = ipc.sync[static_cast<uint>(client)];
  auto& sync_rx = ipc.sync[static_cast<uint>(GetRemote(client))];

  switch (offset) {
    case 0:
      return sync_rx.send & 0xF;
    case 1:
      return (sync_tx.send & 0xF) |
             (sync_tx.enable_remote_irq ? 64 : 0);
  }

  UNREACHABLE;
}

void IPC::IPCSYNC::WriteByte(Client client, uint offset, u8 value) {
  auto& sync_tx = ipc.sync[static_cast<uint>(client)];
  auto& sync_rx = ipc.sync[static_cast<uint>(GetRemote(client))];

  switch (offset) {
    case 0:
      break;
    case 1:
      sync_tx.send = value & 0xF;
      sync_tx.enable_remote_irq = value & 64;
      if ((value & 32) && sync_rx.enable_remote_irq) {
        ipc.RequestIRQ(GetRemote(client), IRQ::Source::IPC_Sync);
      }
      break;
    default:
      UNREACHABLE;
  }
}

auto IPC::IPCFIFOCNT::ReadByte(Client client, uint offset) -> u8 {
  auto& fifo_tx = ipc.fifo[static_cast<uint>(client)];
  auto& fifo_rx = ipc.fifo[static_cast<uint>(GetRemote(client))];

  switch (offset) {
    case 0:
      return (fifo_tx.send.IsEmpty()  ? 1 : 0) |
             (fifo_tx.send.IsFull()   ? 2 : 0) |
             (fifo_tx.enable_send_irq ? 4 : 0);
    case 1:
      return (fifo_rx.send.IsEmpty()  ? 1 : 0) |
             (fifo_rx.send.IsFull()   ? 2 : 0) |
             (fifo_tx.enable_recv_irq ? 4 : 0) |
             (fifo_tx.error  ?  64 : 0) |
             (fifo_tx.enable ? 128 : 0);
  }

  UNREACHABLE;
}

void IPC::IPCFIFOCNT::WriteByte(Client client, uint offset, u8 value) {
  auto& fifo_tx = ipc.fifo[static_cast<uint>(client)];
  auto& fifo_rx = ipc.fifo[static_cast<uint>(GetRemote(client))];

  bool enable_send_irq_old = fifo_tx.enable_send_irq;
  bool enable_recv_irq_old = fifo_tx.enable_recv_irq;

  switch (offset) {
    case 0:
      fifo_tx.enable_send_irq = value & 4;
      if (!enable_send_irq_old && fifo_tx.enable_send_irq && fifo_tx.send.IsEmpty()) {
        ipc.RequestIRQ(client, IRQ::Source::IPC_SendEmpty);
      }
      if (value & 8) {
        fifo_tx.send.Reset();
      }
      break;
    case 1:
      fifo_tx.enable_recv_irq = value & 4;
      if (!enable_recv_irq_old && fifo_tx.enable_recv_irq && !fifo_rx.send.IsEmpty()) {
        ipc.RequestIRQ(client, IRQ::Source::IPC_ReceiveNotEmpty);
      }
      if (value & 64) {
        fifo_tx.error = false;
      }
      fifo_tx.enable = value & 128;
      break;
    default:
      UNREACHABLE;
  }
}

void IPC::IPCFIFOSEND::WriteByte(Client client, u8 value) {
  WriteWord(client, value * 0x01010101);
}

void IPC::IPCFIFOSEND::WriteHalf(Client client, u16 value) {
  WriteWord(client, value * 0x00010001);
}

void IPC::IPCFIFOSEND::WriteWord(Client client, u32 value) {
  auto& fifo_tx = ipc.fifo[static_cast<uint>(client)];
  auto& fifo_rx = ipc.fifo[static_cast<uint>(GetRemote(client))];

  if (!fifo_tx.enable) {
    LOG_ERROR("IPC[{0}]: attempted write FIFO but FIFOs are disabled.", client);
    return;
  }
  
  if (fifo_tx.send.IsFull()) {
    fifo_tx.error = true;
    LOG_ERROR("IPC[{0}]: attempted to write to already full FIFO.", client);
    return;
  }

  if (fifo_rx.enable_recv_irq && fifo_tx.send.IsEmpty()) {
    ipc.RequestIRQ(GetRemote(client), IRQ::Source::IPC_ReceiveNotEmpty);
  }

  fifo_tx.send.Write(value);
}


auto IPC::IPCFIFORECV::ReadByte(Client client, uint offset) -> u8 {
  if (offset >= 4) {
    UNREACHABLE;
  }

  return ReadWord(client) >> (offset * 8);
}

auto IPC::IPCFIFORECV::ReadHalf(Client client, uint offset) -> u16 {
  if (offset >= 4) {
    UNREACHABLE;
  }

  return ReadWord(client) >> (offset * 8);
}

auto IPC::IPCFIFORECV::ReadWord(Client client) -> u32 {
  auto& fifo_tx = ipc.fifo[static_cast<uint>(client)];
  auto& fifo_rx = ipc.fifo[static_cast<uint>(GetRemote(client))];

  if (!fifo_tx.enable) {
    LOG_ERROR("IPC[{0}]: attempted to read FIFO but FIFOs are disabled.", client);
    // TODO: figure out if this read should update the latch.
    return fifo_rx.send.Peek();
  }

  if (fifo_rx.send.IsEmpty()) {
    fifo_tx.error = true;
    LOG_ERROR("IPC[{0}]: attempted to read empty FIFO.", client);
    return fifo_tx.latch;
  }

  if (fifo_rx.enable_send_irq && fifo_rx.send.Count() == 1) {
    ipc.RequestIRQ(GetRemote(client), IRQ::Source::IPC_SendEmpty);
  }

  fifo_tx.latch = fifo_rx.send.Read();
  return fifo_tx.latch;
}

} // namespace Duality::core
