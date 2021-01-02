/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include "matrix.hpp"

namespace fauxDS::core {

// TODO: figure out how over- and underflow work on hardware.
template<int capacity>
struct MatrixStack {
  void Reset() {
    error = false;
    index = 0;
    current.LoadIdentity();
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
  
  Matrix4x4 current;

private:
  int index;
  
  Matrix4x4 stack[capacity];
};

} // namespace fauxDS::core

