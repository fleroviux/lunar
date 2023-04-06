/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <lunar/log.hpp>
#include <ctime>

#include "rtc.hpp"

namespace lunar::nds {

constexpr int RTC::s_argument_count[8];

void RTC::Reset() {
  current_bit = 0;
  current_byte = 0;
  data = 0;
  port = {};
  state = State::Command;

  stat1 = 0;
  stat2 = 0;
}

auto RTC::Read() -> u8 {
  return (port.sio << 0) |
         (port.sck << 1) |
         (port.cs  << 2) |
         (port.write_sio ? 16 : 0) |
         (port.write_sck ? 32 : 0) |
         (port.write_cs  ? 64 : 0);
}

void RTC::Write(u8 value) {
  port.write_sio = value & 16;
  port.write_sck = value & 32;
  port.write_cs  = value & 64;

  WritePort(value);
}

bool RTC::ReadSIO() {
  data &= ~(1 << current_bit);
  data |= port.sio << current_bit;

  if (++current_bit == 8) {
    current_bit = 0;
    return true;
  }

  return false;
}

void RTC::WritePort(u8 value) {
  int old_sck = port.sck;
  int old_cs  = port.cs;

  if (port.write_sio) port.sio = (value >> 0) & 1;
  if (port.write_sck) port.sck = (value >> 1) & 1;
  if (port.write_cs ) port.cs  = (value >> 2) & 1;

  if (!old_cs && port.cs) {
    state = State::Command;
    current_bit  = 0;
    current_byte = 0;
  }

  if (!port.cs || !(old_sck && !port.sck)) {
    return;
  }

  switch (state) {
    case State::Command: {
      ReceiveCommandSIO();
      break;
    }
    case State::Receiving: {
      ReceiveBufferSIO();
      break;
    }
    case State::Sending: {
      TransmitBufferSIO();
      break;
    }
  }
}

void RTC::ReceiveCommandSIO() {
  bool completed = ReadSIO();

  if (!completed) {
    return;
  }

  // Check whether the command should be interpreted MSB-first or LSB-first.
  if ((data >> 4) == 6) {
    // Fast bit-reversal
    data = (data << 4) | (data >> 4);
    data = ((data & 0x33) << 2) | ((data & 0xCC) >> 2);
    data = ((data & 0x55) << 1) | ((data & 0xAA) >> 1);
    LOG_TRACE("RTC: received command in REV format, data=0x{0:X}", data);
  } else if ((data & 15) != 6) {
    LOG_ERROR("RTC: received command in unknown format, data=0x{0:X}", data);
    return;
  }

  reg = static_cast<Register>((data >> 4) & 7);
  current_bit  = 0;
  current_byte = 0;

  // data[7:] determines whether the RTC register will be read or written.
  if (data & 0x80) {
    ReadRegister();

    if (s_argument_count[(int)reg] > 0) {
      state = State::Sending;
    } else {
      state = State::Command;
    }
  } else {
    if (s_argument_count[(int)reg] > 0) {
      state = State::Receiving;
    } else {
      WriteRegister();
      state = State::Command;
    }
  }
}

void RTC::ReceiveBufferSIO() {
  if (current_byte < s_argument_count[(int)reg] && ReadSIO()) {
    buffer[current_byte] = data;

    if (++current_byte == s_argument_count[(int)reg]) {
      WriteRegister();

      // TODO: does the chip accept more commands or
      // must it be reenabled before sending the next command?
      state = State::Command;
    }
  } 
}

void RTC::TransmitBufferSIO() {
  port.sio = buffer[current_byte] & 1;
  buffer[current_byte] >>= 1;

  if (++current_bit == 8) {
    current_bit = 0;
    if (++current_byte == s_argument_count[(int)reg]) {
      // TODO: does the chip accept more commands or
      // must it be reenabled before sending the next command?
      state = State::Command;
    }
  }
}

void RTC::ReadRegister() {
  LOG_TRACE("RTC: read register {}", reg);

  switch (reg) {
    case Register::Stat1: {
      buffer[0] = stat1;
      break;
    }
    case Register::Stat2: {
      buffer[0] = stat2;
      break;
    }
    case Register::Time: {
      auto timestamp = std::time(nullptr);
      auto time = std::localtime(&timestamp);
      buffer[0] = ConvertDecimalToBCD(time->tm_hour);
      buffer[1] = ConvertDecimalToBCD(time->tm_min);
      buffer[2] = ConvertDecimalToBCD(time->tm_sec);
      break;
    }
    case Register::DateTime: {
      auto timestamp = std::time(nullptr);
      auto time = std::localtime(&timestamp);
      buffer[0] = ConvertDecimalToBCD(time->tm_year - 100);
      buffer[1] = ConvertDecimalToBCD(1 + time->tm_mon);
      buffer[2] = ConvertDecimalToBCD(time->tm_mday);
      buffer[3] = ConvertDecimalToBCD(time->tm_wday);
      buffer[4] = ConvertDecimalToBCD(time->tm_hour);
      buffer[5] = ConvertDecimalToBCD(time->tm_min);
      buffer[6] = ConvertDecimalToBCD(time->tm_sec);
      break;
    }
    case Register::INT1_01: {
      // TODO
      buffer[0] = 0;
      buffer[1] = 0;
      buffer[2] = 0;
      break;
    }
    case Register::INT2_AlarmTime: {
      // TODO
      buffer[0] = 0;
      buffer[1] = 0;
      buffer[2] = 0;
      break;
    }
    case Register::ClockAdjustment: {
      // TODO
      buffer[0] = 0;
      break;
    }

    default: {
      ASSERT(false, "RTC: unhandled register read: {}", (int)reg);
    }
  }
}

void RTC::WriteRegister() {
  LOG_TRACE("RTC: write register {}", reg);

  switch (reg) {
    case Register::Stat1: {
      stat1 &= stat1 & ~0b1110;
      stat1 |= buffer[0] & 0b1110;

      if (buffer[0] & 1) {
        // TODO: reset RTC
      }
      break;
    }
    case Register::Stat2: {
      // TODO: update what register 0x01 maps to.
      stat2 = buffer[0];
      break;
    }
    case Register::DateTime:
    case Register::Time: {
      // TODO
      LOG_WARN("RTC: write to the (date)time register is unsupported.");
      break;
    }
    case Register::INT1_01: {
      // TODO
      break;
    }
    case Register::INT2_AlarmTime: {
      // TODO
      break;
    }
    case Register::ClockAdjustment: {
      // TODO
      break;
    }

    default: {
      ASSERT(false, "RTC: unhandled register write: {}", (int)reg);
    }
  }
}

} // namespace lunar::nds
