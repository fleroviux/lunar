/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <common/fifo.hpp>
#include <common/integer.hpp>

namespace fauxDS::core {

/// Inter-Process Communication hardware for ARM9 and ARM7
/// synchronization and message passing.
struct IPC {
  enum class Client : uint {
    ARM7 = 0,
    ARM9 = 1
  };

  enum class IRQ {
    SYNC,
    SEND_Empty,
    RECV_NotEmpty
  };

  IPC();

  void Reset();

  struct IPCSYNC {
    IPCSYNC(IPC& ipc) : ipc(ipc) {}

    auto ReadByte (Client client, uint offset) -> u8;
    void WriteByte(Client client, uint offset, u8 value);

  private:
    IPC& ipc;
  } ipcsync {*this};

private:
  friend struct IPCSYNC;

  static constexpr auto GetRemote(Client client) {
    if (client == Client::ARM9)
      return Client::ARM7;
    return Client::ARM9;
  }

  void RequestIRQ(Client client, IRQ reason);

  struct {
    u8 send = 0;
    bool enable_remote_irq = false;
  } sync[2];
};

} // namespace fauxDS::core
