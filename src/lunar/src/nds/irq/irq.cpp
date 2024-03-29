/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <atom/panic.hpp>

#include "irq.hpp"

namespace lunar::nds {

IRQ::IRQ() {
  Reset();
}

void IRQ::Reset() {
  ime = {};
  ie = {};
  _if = {};

  ime.irq = this;
   ie.irq = this;
  _if.irq = this;
  UpdateIRQLine();
}

void IRQ::Raise(Source source) {
  _if.value |= static_cast<u32>(source);
  UpdateIRQLine();
}

bool IRQ::IsEnabled() {
  return ime.enabled;
}

bool IRQ::HasPendingIRQ() {
  return (ie.value & _if.value) != 0;
}

void IRQ::UpdateIRQLine() {
  if (core != nullptr) {
    core->IRQLine() = IsEnabled() && HasPendingIRQ();
  }
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

  ATOM_UNREACHABLE();
}

void IRQ::IME::WriteByte(uint offset, u8 value) {
  switch (offset) {
    case 0:
      enabled = value & 1;
      if (irq != nullptr) {
        irq->UpdateIRQLine();
      }
      break;
    case 1:
    case 2:
    case 3:
      break;
    default:
      ATOM_UNREACHABLE();
  }
}

auto IRQ::IE::ReadByte(uint offset) -> u8 {
  if (offset >= 4) {
    ATOM_UNREACHABLE();
  }

  return (value >> (offset * 8)) & 0xFF;
}

void IRQ::IE::WriteByte(uint offset, u8 value) {
  if (offset >= 4) {
    ATOM_UNREACHABLE();
  }

  this->value &= ~(0xFF << (offset * 8));
  this->value |= value << (offset * 8);
  if (irq != nullptr) {
    irq->UpdateIRQLine();
  }
}

auto IRQ::IF::ReadByte(uint offset) -> u8 {
  if (offset >= 4) {
    ATOM_UNREACHABLE();
  }

  return (value >> (offset * 8)) & 0xFF;
}

void IRQ::IF::WriteByte(uint offset, u8 value) {
  if (offset >= 4) {
    ATOM_UNREACHABLE();
  }

  this->value &= ~(value << (offset * 8));
  if (irq != nullptr) {
    irq->UpdateIRQLine();
  }
}


} // namespace lunar::nds
