
// Copyright (C) 2022 fleroviux

#include <lunar/log.hpp>

#include "nds/keypad/keypad.hpp"

namespace lunar::nds {

KeyPad::KeyPad(IRQ& irq7, IRQ& irq9)
    : irq7(irq7)
    , irq9(irq9) {
  Reset();
}

void KeyPad::Reset() {
  input = {};

  control7 = {};
  control7.keypad = this;
  control7.irq = &irq7;

  control9 = {};
  control9.keypad = this;
  control9.irq = &irq9;
}

void KeyPad::SetInputDevice(InputDevice& input_device) {
  this->input_device = &input_device;
  
  input_device.SetOnChangeCallback(std::bind(&KeyPad::UpdateInput, this));
}

void KeyPad::UpdateInput() {
  u16 input = 0;
  u16 ext_input = 0x34;

  if (!input_device->Poll(Key::A)) input |= 1;
  if (!input_device->Poll(Key::B)) input |= 2;
  if (!input_device->Poll(Key::Select)) input |= 4;
  if (!input_device->Poll(Key::Start)) input |= 8;
  if (!input_device->Poll(Key::Right)) input |= 16;
  if (!input_device->Poll(Key::Left)) input |= 32;
  if (!input_device->Poll(Key::Up)) input |= 64;
  if (!input_device->Poll(Key::Down)) input |= 128;
  if (!input_device->Poll(Key::R)) input |= 256;
  if (!input_device->Poll(Key::L)) input |= 512;

  if (!input_device->Poll(Key::X)) ext_input |= 1;
  if (!input_device->Poll(Key::Y)) ext_input |= 2;
  if (!input_device->Poll(Key::TouchPen)) ext_input |= 64;

  this->input.value = input;
  this->ext_input.value = ext_input;

  UpdateIRQ(&control7, &irq7);
  UpdateIRQ(&control9, &irq9);
}

void KeyPad::UpdateIRQ(KeyControl* control, IRQ* irq) {
  if (control->interrupt) {
    auto not_input = ~input.value & 0x3FF;
    
    if (control->mode == KeyControl::Mode::LogicalAND) {
      if (control->mask == not_input) {
        irq->Raise(IRQ::Source::KeyPad);
      }
    } else if ((control->mask & not_input) != 0) {
      irq->Raise(IRQ::Source::KeyPad);
    }
  }
}

auto KeyPad::KeyInput::ReadByte(uint offset) -> u8 {
  switch (offset) {
    case 0:
      return u8(value);
    case 1:
      return u8(value >> 8);
  }

  UNREACHABLE;
}

auto KeyPad::ExtKeyInput::ReadByte() -> u8 {
  return value;
}

auto KeyPad::KeyControl::ReadByte(uint offset) -> u8 {
  switch (offset) {
    case 0: {
      return u8(mask);
    }
    case 1: {
      return ((mask >> 8) & 3) |
              (interrupt ? 64 : 0) |
              (int(mode) << 7);
    }
  }

  UNREACHABLE;
}

void KeyPad::KeyControl::WriteByte(uint offset, u8 value) {
  switch (offset) {
    case 0: {
      mask &= 0xFF00;
      mask |= value;
      break;
    }
    case 1: {
      mask &= 0x00FF;
      mask |= (value & 3) << 8;
      interrupt = value & 64;
      mode = Mode(value >> 7);
      break;
    }
    default: {
      UNREACHABLE;
    }
  }

  keypad->UpdateIRQ(this, irq);
}

void KeyPad::KeyControl::WriteHalf(u16 value) {
  mask = value & 0x03FF;
  interrupt = value & 0x4000;
  mode = Mode(value >> 15);
  keypad->UpdateIRQ(this, irq);
}

} // namespace lunar::nds
