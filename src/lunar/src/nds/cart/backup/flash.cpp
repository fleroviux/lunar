/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <atom/panic.hpp>

#include "flash.hpp"

/*
 * TODO: 
 * - figure out the state after a command completed
 * - figure out what happens if /CS is not driven high after sending a command byte.
 * - generate a plausible manufacturer and device id instead of always using the same one.
 */

namespace lunar::nds {

FLASH::FLASH(std::string const& save_path, Size size_hint)
    : save_path(save_path)
    , size_hint(size_hint) {
  Reset();
}

void FLASH::Reset() {
  static const std::vector<size_t> kBackupSize { 
    0x40000, 0x80000, 0x100000 };

  auto size = kBackupSize[static_cast<int>(size_hint)];

  file = BackupFile::OpenOrCreate(save_path, kBackupSize, size);
  mask = size - 1U;
  Deselect();

  write_enable_latch = false;
  deep_power_down = false;
}

void FLASH::Select() {
  if (state == State::Deselected) {
    state = State::ReceiveCommand;
  }
}

void FLASH::Deselect() {
  if (current_cmd == Command::PageWrite ||
      current_cmd == Command::PageProgram ||
      current_cmd == Command::PageErase ||
      current_cmd == Command::SectorErase) {
    write_enable_latch = false;
  }

  state = State::Deselected;
}

auto FLASH::Transfer(u8 data) -> u8 {
  switch (state) {
    case State::ReceiveCommand: {
      ParseCommand(static_cast<Command>(data));
      break;
    }
    case State::ReadJEDEC: {
      data = jedec_id[address];
      if (++address == 3) {
        state = State::Idle;
      }
      break;
    }
    case State::ReadStatus: {
      return write_enable_latch ? 2 : 0;
    }
    case State::SendAddress0: {
      address  = data << 16;
      state = State::SendAddress1;
      break;
    }
    case State::SendAddress1: {
      address |= data << 8;
      state = State::SendAddress2;
      break;
    }
    case State::SendAddress2: {
      address |= data;
      address &= mask;

      // TODO: this is a bit messy still, try to refactor it away.
      switch (current_cmd) {
        case Command::ReadData:     state = State::ReadData;    break;
        case Command::ReadDataFast: state = State::DummyByte;   break;
        case Command::PageWrite:    state = State::PageWrite;   break;
        case Command::PageProgram:  state = State::PageProgram; break;
        case Command::PageErase:    state = State::PageErase;   break;
        case Command::SectorErase:  state = State::SectorErase; break;
      }
      break;
    }
    case State::DummyByte: {
      state = State::ReadData;
      break;
    }
    case State::ReadData: {
      return file->Read(address++ & mask);
    }
    case State::PageWrite: {
      file->Write(address, data);
      address = (address & ~0xFF) | ((address + 1) & 0xFF);
      break;
    }
    case State::PageProgram: {
      // TODO: confirm that page program actually is a bitwise-AND operation.
      file->Write(address, file->Read(address) & data);
      address = (address & ~0xFF) | ((address + 1) & 0xFF);
      break;
    }
    case State::PageErase: {
      address &= ~0xFF;
      for (uint i = 0; i < 256; i++) {
        file->Write(address + i, 0xFF);
      }
      state = State::Idle;
      break;
    }
    case State::SectorErase: {
      address &= ~0xFFFF;
      for (uint i = 0; i < 0x10000; i++) {
        file->Write(address + i, 0xFF);
      }
      state = State::Idle;
      break;
    }
    case State::Idle: {
      break;
    }
    default: {
      ATOM_UNREACHABLE();
    }
  }

  return 0xFF;
}

void FLASH::ParseCommand(Command cmd) {
  if (deep_power_down) {
    if (cmd == Command::ReleaseDeepPowerDown) {
      deep_power_down = false;
      state = State::Idle;
    }
    return;
  }

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
    case Command::ReadJEDEC: {
      address = 0;
      state = State::ReadJEDEC;
      break;
    }
    case Command::ReadStatus: {
      state = State::ReadStatus;
      break;
    }
    case Command::ReadData:
    case Command::ReadDataFast: {
      state = State::SendAddress0;
      break;
    }
    case Command::PageWrite:
    case Command::PageProgram:
    case Command::PageErase:
    case Command::SectorErase: {
      if (write_enable_latch) {
        state = State::SendAddress0;
      } else {
        state = State::Idle;
      }
      break;
    }
    case Command::DeepPowerDown: {
      deep_power_down = true;
      break;
    }
    default: {
      // ATOM_PANIC("FLASH: unhandled command 0x{0:02X}", cmd);
    }
  }
}

} // namespace lunar::nds