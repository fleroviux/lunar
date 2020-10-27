/*
 * Copyright (C) 2020 fleroviux
 */

#include "irq.hpp"

namespace fauxDS::core {

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


} // namespace fauxDS::core
