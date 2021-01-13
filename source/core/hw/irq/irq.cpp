/*
 * Copyright (C) 2020 fleroviux
 */

#include <common/log.hpp>

#include "irq.hpp"

namespace Duality::core {

IRQ::IRQ() {
  Reset();
}

void IRQ::Reset() {
  ime = {};
  ie = {};
  _if = {};
}

void IRQ::Raise(Source source) {
  _if.value |= static_cast<u32>(source);
}

bool IRQ::IsEnabled() {
  return ime.enabled;
}

bool IRQ::HasPendingIRQ() {
  return (ie.value & _if.value) != 0;
}

auto IRQ::IME::ReadByte(uint offset) -> u8 {
  switch (offset) {
    case 0:
      return enabled ? 1 : 0;
    case 1:
    case 2:
    case 3:
      return 0;
  }

  UNREACHABLE;
}

void IRQ::IME::WriteByte(uint offset, u8 value) {
  switch (offset) {
    case 0:
      enabled = value & 1;
      break;
    case 1:
    case 2:
    case 3:
      break;
    default:
      UNREACHABLE;
  }
}

auto IRQ::IE::ReadByte(uint offset) -> u8 {
  if (offset >= 4) {
    UNREACHABLE;
  }

  return (value >> (offset * 8)) & 0xFF;
}

void IRQ::IE::WriteByte(uint offset, u8 value) {
  if (offset >= 4) {
    UNREACHABLE;
  }

  this->value &= ~(0xFF << (offset * 8));
  this->value |= value << (offset * 8);
}

auto IRQ::IF::ReadByte(uint offset) -> u8 {
  if (offset >= 4) {
    UNREACHABLE;
  }

  return (value >> (offset * 8)) & 0xFF;
}

void IRQ::IF::WriteByte(uint offset, u8 value) {
  if (offset >= 4) {
    UNREACHABLE;
  }

  this->value &= ~(value << (offset * 8));
}


} // namespace Duality::core
