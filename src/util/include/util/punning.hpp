/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <util/integer.hpp>
#include <string.h>

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
