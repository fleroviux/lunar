
// Copyright (C) 2022 fleroviux

#pragma once

#include <lunar/integer.hpp>
#include <string.h>

namespace lunar {

template<typename T>
auto read(void* data, uint offset) -> T {
  T value;
  memcpy(&value, (u8*)data + offset, sizeof(T));
  return value;
}

template<typename T>
void write(void* data, uint offset, T value) {
  memcpy((u8*)data + offset, &value, sizeof(T));
}

} // namespace lunar
