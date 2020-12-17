/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include "matrix.hpp"

namespace fauxDS::core {

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
    
    // TODO: this is pretty much guessed... how does it really work?
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
    } else {
      stack[address] = current;
    }
  }
  
  void Restore(int address) {
    if (capacity == 1) {
      current = stack[0];
    } else {
      current = stack[address];
    }
  }
  
  bool error;
  Matrix4x4 current;

private:
  int index;
  Matrix4x4 stack[capacity];
};

} // namespace fauxDS::core

