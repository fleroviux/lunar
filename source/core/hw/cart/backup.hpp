/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <common/integer.hpp>
#include <common/log.hpp>

namespace fauxDS::core {

/// This is mostly a stub now, do properly at a later point.
struct Backup {
  Backup() {
    // TODO: backup should be deselected by default...
    state = State::ReceiveCommand;
  }

  void Select() {
    // TODO!
  }

  void Deselect() {
    // TODO: this is likely not correct.
    // It is only there to account for the fact that I don't know,
    // when the backup device is selected.
    state = State::ReceiveCommand;
  }

  auto Transfer(u8 data) -> u8 {
    LOG_TRACE("Cartridge: SPI: send value 0x{0:02X}", data);

    switch (state) {
      case State::ReceiveCommand:
        ParseCommand(data);
        break;
      case State::ReadAddress0:
        address = data << 16;
        state = State::ReadAddress1;
        break;
      case State::ReadAddress1:
        address |= data << 8;
        state = State::ReadAddress2;
        break;
      case State::ReadAddress2:
        address |= data;
        LOG_INFO("Cartridge: SPI: read address completed, address = 0x{0:06X}", address);
        if (command == Command::ReadData) {
          state = State::ReadData;
        } else {
          ASSERT(false, "Cartridge:SPI: no possible state transition from ReadAddress2");
        }
        break;
      case State::ReadData:
        LOG_INFO("Cartridge: SPI: read 0x{0:02X} at address 0x{1:06X}", save[address], address);
        return save[address++ & 0x7FFFF];
      case State::ReadStatus:
        LOG_TRACE("Cartridge: SPI: read status register!");
        return 0;
      default:
        UNREACHABLE;
    }

    return 0;
  }

private:
  enum class Command : u8 {
    WriteEnable   = 0x06, // WREM
    WriteDisable  = 0x04, // WRDI
    ReadJEDEC     = 0x96, // RDID
    ReadStatus    = 0x05, // RDSR
    ReadData      = 0x03, // READ
    ReadDataFast  = 0x0B, // FAST
    PageWrite     = 0x0A, // PW
    PageProgram   = 0x02, // PP
    PageErase     = 0xDB, // PE
    SectorErase   = 0xD8, // SE
    DeepPowerDown = 0xB9, // DP
    ReleaseDeepPowerDown = 0xAB // RDP
  };

  enum class State {
    ReceiveCommand,
    ReadAddress0,
    ReadAddress1,
    ReadAddress2,
    ReadData,
    ReadStatus
  };

  State state;
  Command command;
  u32 address;
  bool enable_write = false;

  // Reeeally ugly hack...
  u8 save[512*1024] {0};

  void ParseCommand(u8 command) {
    this->command = static_cast<Command>(command);

    // This is a hack to ignore IR chipselect commands, for now.
    if (command == 0 || command == 8)
      return;

    switch (static_cast<Command>(command)) {
      case Command::ReadData:
        state = State::ReadAddress0;
        break;
      case Command::ReadStatus:
        state = State::ReadStatus;
        break;
      default:
        ASSERT(false, "Cartridge: SPI: unhandled command 0x{0:02X}", command);
    }
  }
};

} // namespace fauxDS::core
