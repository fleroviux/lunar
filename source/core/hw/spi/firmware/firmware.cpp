/*
 * Copyright (C) fleroviux
 */

#include <common/log.hpp>

#include "firmware.hpp"

namespace fauxDS::core {

void Firmware::Reset() {
  if (file.is_open()) {
    file.close();
  }
  file.open("firmware.bin", std::ios::in | std::ios::out | std::ios::binary);
  ASSERT(file.good(), "Firmware: failed to open firmware.bin");
  state = State::Deselected;
  address = 0;
  enable_write = false;
}

void Firmware::Select() {
  LOG_INFO("SPI: FIRM: command start");
  state = State::ReceiveCommand;
}

void Firmware::Deselect() {
  LOG_INFO("SPI: FIRM: deselected!");
  state = State::Deselected;
}

auto Firmware::Transfer(u8 data) -> u8 {
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
      LOG_INFO("SPI: FIRM: read address completed, address = 0x{0:06X}", address);
      if (command == Command::ReadData) {
        state = State::ReadData;
      } else {
        ASSERT(false, "SPI: FIRM: no possible state transition from ReadAddress2");
      }
      break;
    case State::ReadData:
      // TODO: this is not a very efficient implementation.
      // TODO: make sure the address is valid, possibly force firmware size and use wraparound?
      file.seekg(address++);
      file.read((char*)&data, 1);
      LOG_INFO("SPI: FIRM: read 0x{0:02X} at address 0x{1:06X}", data, address - 1);
      ASSERT(file.good(), "SPI: FIRM: firmware byte read went rougue");
      return data;
    case State::ReadStatus:
      // TODO: write/program/erase in progress
      return (enable_write ? 2 : 0);
    case State::Deselected:
      ASSERT(false, "SPI: FIRM: attempted to access deselected device.");
    default:
      ASSERT(false, "SPI: FIRM: unhandled state: {0}", state);
  }

  return 0;
}

void Firmware::ParseCommand(u8 command) {
  this->command = static_cast<Command>(command);

  switch (static_cast<Command>(command)) {
    case Command::ReadData:
      state = State::ReadAddress0;
      break;
    case Command::ReadStatus:
      state = State::ReadStatus;
      break;
    case Command::WriteEnable:
      enable_write = true;
      break;
    default:
      ASSERT(false, "SPI: FIRM: unimplemented command: 0x{0:02X}", command);
  }
}

} // namespace fauxDS::core

