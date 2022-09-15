
// Copyright (C) 2022 fleroviux

#include <lunar/log.hpp>

#include "eeprom512b.hpp"

namespace lunar::nds {

EEPROM512B::EEPROM512B(std::string const& save_path)
    : save_path(save_path) {
  Reset();
}

void EEPROM512B::Reset() {
  static const std::vector<size_t> kBackupSize { 512 };

  size_t size = 512;
  file = BackupFile::OpenOrCreate(save_path, kBackupSize, size);
  Deselect();

  write_enable_latch = false;
  write_protect_mode = 0;
}

void EEPROM512B::Select() {
  if (state == State::Deselected) {
    state = State::ReceiveCommand;
  }
}

void EEPROM512B::Deselect() {
  if (current_cmd == Command::Write ||
      current_cmd == Command::WriteStatus) {
    write_enable_latch = false;
  }

  state = State::Deselected;
}

auto EEPROM512B::Transfer(u8 data) -> u8 {
  switch (state) {
    case State::ReceiveCommand: {
      ParseCommand(data);
      break;
    }
    case State::ReadStatus: {
      return (write_enable_latch ? 2 : 0) |
             (write_protect_mode << 2) | 0xF0;
    }
    case State::WriteStatus: {
      write_enable_latch = data & 2;
      write_protect_mode = (data >> 2) & 3;
      state = State::Idle;
      break;
    }
    case State::ReadAddress: {
      address |= data;
      if (current_cmd == Command::Read) {
        state = State::Read;
      } else {
        state = State::Write;
      }
      break;
    }
    case State::Read: {
      return file->Read(address++ & 0x1FF);
    }
    case State::Write: {
      switch (write_protect_mode) {
        case 1: if (address >= 0x180) return 0xFF;
        case 2: if (address >= 0x100) return 0xFF;
        case 3: return 0xFF;
      }

      file->Write(address, data);
      address = (address & ~15) | ((address + 1) & 15);
      break;
    }
    case State::Idle: {
      break;
    }
    default: {
      UNREACHABLE;
    }
  }

  return 0xFF;
}

void EEPROM512B::ParseCommand(u8 cmd) {
  auto a8 = cmd & 8;

  if ((cmd & 0x80) == 0) cmd &= ~8;

  current_cmd = static_cast<Command>(cmd);

  switch (current_cmd) {
    case Command::WriteEnable: {
      write_enable_latch = true;
      state = State::Idle;
      break;
    }
    case Command::WriteDisable: {
      write_enable_latch = false;
      state = State::Idle;
      break;
    }
    case Command::ReadStatus: {
      state = State::ReadStatus;
      break;
    }
    case Command::WriteStatus: {
      if (write_enable_latch) {
        state = State::WriteStatus;
      } else {
        state = State::Idle;
      }
      break;
    }
    case Command::Read: {
      address = a8 << 5;
      state = State::ReadAddress;
      break;
    }
    case Command::Write: {
      if (write_enable_latch) {
        address = a8 << 5;
        state = State::ReadAddress;
      } else {
        state = State::Idle;
      }
      break;
    }
    default: {
      // ASSERT(false, "EEPROM: unhandled command 0x{0:02X}", cmd);
    }
  }
}

} // namespace lunar::nds
