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
  LOG_ERROR("IPC: requested IRQ but this is unimplemented.");
}

auto IPC::IPCSYNC::ReadByte(Client client, uint offset) -> u8 {
  switch (offset) {
    case 0:
      return 0;
    case 1:
      return 0;
  }

  UNREACHABLE;
}

void IPC::IPCSYNC::WriteByte(Client client, uint offset, u8 value) {
  switch (offset) {
    case 0:
      break;
    case 1:
      break;
    default:
      UNREACHABLE;
  }
}

} // namespace fauxDS::core
