/*
 * Copyright (C) 2021 fleroviux
 */

#include <util/log.hpp>

#include "eeprom.hpp"

namespace Duality::Core {

EEPROM::EEPROM(std::string const& save_path, Size size_hint)
    : save_path(save_path)
    , size_hint(size_hint) {
  Reset();
}

void EEPROM::Reset() {
  static const std::vector<size_t> kBackupSize { 
    8192, 32768, 65536 };

  auto size = kBackupSize[static_cast<int>(size_hint)];

  file = BackupFile::OpenOrCreate(save_path, kBackupSize, size);
  mask = size - 1U;
  Deselect();

  if (size == 8192) {
    page_mask = 31;
  } else if (size == 65536) {
    page_mask = 127;
  } else {
    // FRAM writes are unlimited
    page_mask = mask;
  }

  write_enable_latch = false;
  write_protect_mode = 0;
  status_reg_write_disable = false;
}

void EEPROM::Select() {
  if (state == State::Deselected) {
    state = State::ReceiveCommand;
  }
}

void EEPROM::Deselect() {
  if (current_cmd == Command::Write ||
      current_cmd == Command::WriteStatus) {
    write_enable_latch = false;
  }

  state = State::Deselected;
}

auto EEPROM::Transfer(u8 data) -> u8 {
  switch (state) {
    case State::ReceiveCommand: {
      ParseCommand(static_cast<Command>(data));
      break;
    }
    case State::ReadStatus: {
      return (write_enable_latch ? 2 : 0) |
             (write_protect_mode << 2) | 
             (status_reg_write_disable ? 128 : 0);
    }
    case State::WriteStatus: {
      // TODO: figure out how to apply the SRWD bit.
      write_enable_latch = data & 2;
      write_protect_mode = (data >> 2) & 3;
      status_reg_write_disable = data & 128;
      state = State::Idle;
      break;
    }
    case State::ReadAddressHI: {
      address = data << 8;
      state = State::ReadAddressLO;
      break;
    }
    case State::ReadAddressLO: {
      address |= data;
      if (current_cmd == Command::Read) {
        state = State::Read;
      } else {
        state = State::Write;
      }
      break;
    }
    case State::Read: {
      return file->Read(address++ & mask);
    }
    case State::Write: {
      switch (write_protect_mode) {
        case 1: if (address >= 0xC000) return 0xFF;
        case 2: if (address >= 0x8000) return 0xFF;
        case 3: return 0xFF;
      }

      file->Write(address, data);
      address = (address & ~page_mask) | ((address + 1) & page_mask);
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

void EEPROM::ParseCommand(Command cmd) {
  current_cmd = cmd;

  switch (cmd) {
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
      state = State::ReadAddressHI;
      break;
    }
    case Command::Write: {
      if (write_enable_latch) {
        state = State::ReadAddressHI;
      } else {
        state = State::Idle;
      }
      break;
    }
    default: {
      ASSERT(false, "EEPROM: unhandled command 0x{0:02X}", cmd);
    }
  }
}

} // namespace Duality::Core
