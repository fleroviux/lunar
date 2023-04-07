/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <cstddef>

namespace lunar {

template <typename T, std::size_t size>
class FIFO {
  public:
    FIFO() {
      Reset();
    }

    void Reset(bool clear_buffer = true) {
      rd_ptr = 0;
      wr_ptr = 0;
      count  = 0;
      if (clear_buffer) {
        for (std::size_t i = 0; i < size; i++) {
          data[i] = {};
        }
      }
    }

    auto Count() const -> std::size_t { return count; }
    bool IsEmpty() { return count == 0; }
    bool IsFull() { return count == size; }

    auto Peek() -> T {
      return data[rd_ptr];
    }

    auto Read() -> T {
      auto value = data[rd_ptr];

      if (!IsEmpty()) {
        rd_ptr = (rd_ptr + 1) % size;
        count--;
      }

      return value;
    }

    void Write(T value) {
      if (IsFull()) {
        return;
      }

      data[wr_ptr] = value;
      wr_ptr = (wr_ptr + 1) % size;
      count++;
    }

  private:
    std::size_t rd_ptr{};
    std::size_t wr_ptr{};
    std::size_t count{};
    T data[size];
};

} // namespace lunar
