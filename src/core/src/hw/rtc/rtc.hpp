/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <util/integer.hpp>

namespace Duality::Core {

struct RTC {
  // enum class Port {
  //   SIO,
  //   SCK,
  //   CS
  // };

  enum class State {
    Command,
    Sending,
    Receiving,
    Complete
  };

  enum class Register {
    /*ForceReset = 0,
    DateTime = 2,
    ForceIRQ = 3,
    Control = 4,
    Time = 6,
    Free = 7*/

    Stat1 = 0,
    INT1_01 = 1,
    DateTime = 2,
    ClockAdjustment = 3,
    Stat2 = 4,
    INT2_AlarmTime = 5,
    Time = 6
  };

  RTC() {
    Reset();
  }

  void Reset();
  auto Read() -> u8;
  void Write(u8 value);

protected:
  auto ReadPort() -> u8;
  void WritePort(u8 value);

private:
  bool ReadSIO();
  void ReceiveCommandSIO();
  void ReceiveBufferSIO();
  void TransmitBufferSIO();
  void ReadRegister();
  void WriteRegister();

  static auto ConvertDecimalToBCD(u8 x) -> u8 {
    u8 y = 0;
    u8 e = 1;

    while (x > 0) {
      y += (x % 10) * e;
      e *= 16;
      x /= 10; 
    }

    return y;
  }

  int current_bit;
  int current_byte;

  Register reg;
  u8 data;
  u8 buffer[7];

  struct PortData {
    int sio = 0;
    int sck = 0;
    int cs = 0;

    bool write_sio = false;
    bool write_sck = false;
    bool write_cs = false;
  } port;

  State state;

  // struct ControlRegister {
  //   bool unknown;
  //   bool per_minute_irq;
  //   bool mode_24h;
  //   bool poweroff;

  //   void Reset() {
  //     unknown = false;
  //     per_minute_irq = false;
  //     mode_24h = false;
  //     poweroff = false;
  //   }
  // } control;

  u8 stat1;
  u8 stat2;

  static constexpr int s_argument_count[8] = {
    /*0, // ForceReset
    0, // Unused?
    7, // DateTime
    0, // ForceIRQ
    1, // Control
    0, // Unused?
    3, // Time
    0  // FreeRegister*/
    1, // Stat1
    3, // INT1 frequency duty / alarm1 time
    7, // DateTime
    1, // ClockAdjustment
    1, // Stat2
    3, // INT2 alarm2 time
    6, // Time
    0
  };
};

} // namespace Duality::Core
