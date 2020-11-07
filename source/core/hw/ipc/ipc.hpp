/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <common/fifo.hpp>
#include <common/integer.hpp>
#include <core/hw/irq/irq.hpp>

namespace fauxDS::core {

/// Inter-Process Communication hardware for ARM9 and ARM7
/// synchronization and message passing.
struct IPC {
  enum class Client : uint {
    ARM7 = 0,
    ARM9 = 1
  };

  IPC(IRQ& irq7, IRQ& irq9);

  void Reset();

  struct IPCSYNC {
    IPCSYNC(IPC& ipc) : ipc(ipc) {}

    auto ReadByte (Client client, uint offset) -> u8;
    void WriteByte(Client client, uint offset, u8 value);

  private:
    IPC& ipc;
  } ipcsync {*this};

  struct IPCFIFOCNT {
    IPCFIFOCNT(IPC& ipc) : ipc(ipc) {}

    auto ReadByte (Client client, uint offset) -> u8;
    void WriteByte(Client client, uint offset, u8 value);

  private:
    IPC& ipc;
  } ipcfifocnt {*this};

  struct IPCFIFOSEND {
    IPCFIFOSEND(IPC& ipc) : ipc(ipc) {}

    void WriteByte(Client client,  u8 value);
    void WriteHalf(Client client, u16 value);
    void WriteWord(Client client, u32 value);

  private:
    IPC& ipc;
  } ipcfifosend {*this};

  struct IPCFIFORECV {
    IPCFIFORECV(IPC& ipc) : ipc(ipc) {}

    auto ReadByte(Client client, uint offset) ->  u8;
    auto ReadHalf(Client client, uint offset) -> u16;
    auto ReadWord(Client client) -> u32;

  private:
    IPC& ipc;
  } ipcfiforecv {*this};

private:
  friend struct IPCSYNC;
  friend struct IPCFIFOCNT;
  friend struct IPCFIFOSEND;
  friend struct IPCFIFORECV;

  static constexpr auto GetRemote(Client client) {
    if (client == Client::ARM9)
      return Client::ARM7;
    return Client::ARM9;
  }

  void RequestIRQ(Client client, IRQ::Source reason);

  struct {
    u8 send = 0;
    bool enable_remote_irq = false;
  } sync[2];

  struct {
    common::FIFO<u32, 16> send;
    bool enable_send_irq = false;
    bool enable_recv_irq = false;
    bool error = false;
    bool enable = false;
  } fifo[2];

  IRQ* irq[2];
};

} // namespace fauxDS::core