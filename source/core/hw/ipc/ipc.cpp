/*
 * Copyright (C) 2020 fleroviux
 */

#include <common/log.hpp>

#include "ipc.hpp"

namespace fauxDS::core {

IPC::IPC() {
  Reset();
}

void IPC::Reset() {
  sync[static_cast<uint>(Client::ARM7)] = {};
  sync[static_cast<uint>(Client::ARM9)] = {};
}

void IPC::RequestIRQ(Client client, IRQ reason) {
  ASSERT(false, "IPC: requested IRQ but this is unimplemented.");
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
        ipc.RequestIRQ(GetRemote(client), IRQ::SYNC);
      }
      break;
    default:
      UNREACHABLE;
  }
}

} // namespace fauxDS::core
