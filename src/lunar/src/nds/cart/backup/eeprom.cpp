/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <lunar/log.hpp>

#include "eeprom.hpp"

namespace lunar::nds {

EEPROM::EEPROM(std::string const& save_path, Size size_hint, bool fram)
    : save_path(save_path)
    , size_hint(size_hint)
    , fram(fram) {
  Reset();
}

void EEPROM::Reset() {
  static const std::vector<size_t> kBackupSize { 
    8192, 32768, 65536, 131072 };

  auto bytes = kBackupSize[static_cast<int>(size_hint)];

  file = BackupFile::OpenOrCreate(save_path, kBackupSize, bytes);
  mask = bytes - 1U;
  Deselect();

  switch (bytes) {
    case 8192: {
      size = Size::_8K;
      page_mask = 31;
      address_upper_half = 0x1000;
      address_upper_quarter = 0x1800;
      break;
    }
    case 32768: {
      // Note: there is no known use of EEPROM with this size (only FRAM).
      size = Size::_32K;
      page_mask = 63;
      address_upper_half = 0x4000;
      address_upper_quarter = 0x6000;
      break;
    }
    case 65536: {
      size = Size::_64K;
      page_mask = 127;
      address_upper_half = 0x8000;
      address_upper_quarter = 0xC000;
      break;
    }
    case 131072: {
      size = Size::_128K;
      page_mask = 255;
      address_upper_half = 0x10000;
      address_upper_quarter = 0x18000;
      break;
    }
  }

  // FRAM consists of a single page spanning the full address space.
  if (fram) {
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
    case State::ReadAddress0: {
      address = data << 16;
      state = State::ReadAddress1;
      break;
    }
    case State::ReadAddress1: {
      address |= data << 8;
      state = State::ReadAddress2;
      break;
    }
    case State::ReadAddress2: {
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
        case 1: if (address >= address_upper_quarter) return 0xFF;
        case 2: if (address >= address_upper_half) return 0xFF;
        case 3: return 0xFF;
      }

      file->Write(address & mask, data);
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
      address = 0;
      if (size == Size::_128K) {
        state = State::ReadAddress0;
      } else {
        state = State::ReadAddress1;
      }
      break;
    }
    case Command::Write: {
      if (write_enable_latch) {
        address = 0;
        if (size == Size::_128K) {
          state = State::ReadAddress0;
        } else {
          state = State::ReadAddress1;
        }
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

} // namespace lunar::nds
