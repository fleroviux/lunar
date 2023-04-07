/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <lunar/device/input_device.hpp>
#include <memory>

#include "nds/irq/irq.hpp"

namespace lunar::nds {

class KeyPad {
  public:
    KeyPad(IRQ& irq7, IRQ& irq9);

    void Reset();
    void SetInputDevice(InputDevice& input_device);

    struct KeyInput {
      u16 value = 0x3FF;

      auto ReadByte(uint offset) -> u8;
    } input;

    struct ExtKeyInput {
      u16 value = 0x77;

      auto ReadByte() -> u8;
    } ext_input;

    struct KeyControl {
      u16 mask;
      bool interrupt;

      enum class Mode {
        LogicalOR  = 0,
        LogicalAND = 1
      } mode = Mode::LogicalOR;

      KeyPad* keypad;
      IRQ* irq;

      auto ReadByte(uint offset) -> u8;
      void WriteByte(uint offset, u8 value);
      void WriteHalf(u16 value);
    } control7, control9;

  private:
    using Key = InputDevice::Key;

    void UpdateInput();
    void UpdateIRQ(KeyControl* control, IRQ* irq);

    IRQ& irq7;
    IRQ& irq9;
    InputDevice* input_device = nullptr;
};

} // namespace lunar::nds
