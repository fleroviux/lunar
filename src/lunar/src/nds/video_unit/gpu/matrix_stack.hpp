/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include "matrix.hpp"
#include "fixed_point.hpp"

namespace lunar::nds {

// TODO: figure out how over- and underflow work on hardware.
template<int capacity>
struct MatrixStack {
  void Reset() {
    error = false;
    index = 0;
    current = Matrix4<Fixed20x12>::Identity();
  }
  
  void Push() {
    if (index == capacity) {
      error = true;
      return;
    }
    
    stack[index++] = current;
  }
  
  void Pop(int offset) {
    index -= offset;
    
    if (index < 0) {
      index = 0;
      error = true;
    } else if (index >= capacity) {
      index = capacity - 1;
      error = true;
    }
    
    current = stack[index];
  }
  
  void Store(int address) {
    if (capacity == 1) {
      stack[0] = current;
    } else if (address < capacity) {
      stack[address] = current;
    } else {
      error = true;
    }
  }
  
  void Restore(int address) {
    if (capacity == 1) {
      current = stack[0];
    } else if (address < capacity) {
      current = stack[address];
    } else {
      error = true;
    }
  }
  
  bool error;
  
  Matrix4<Fixed20x12> current;

private:
  int index;
  
  Matrix4<Fixed20x12> stack[capacity];
};

} // namespace lunar::nds

