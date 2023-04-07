/*
 * Copyright (C) 2022 fleroviux.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#pragma once

#include <cstddef>
#include <utility>
#include <initializer_list>

namespace lunar {

template <typename T, std::size_t capacity>
class StaticVec {
  public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = T&;
    using const_reference = T const&;
    using pointer = T*;
    using const_pointer = T const*;

    using iterator = T*;
    using const_iterator = T const*;

    constexpr StaticVec() = default;

    constexpr StaticVec(std::initializer_list<T> values) {
      for (auto& value : values) push_back(value);
    }

    constexpr auto operator[](std::size_t index) -> T& {
      return m_data[index];
    }

    constexpr auto operator[](std::size_t index) const -> T const& {
      return m_data[index];
    }

    constexpr void clear() {
      m_size = 0;
    }

    constexpr void push_back(T const& value) {
      m_data[m_size++] = value;
    }

    constexpr void push_back(T&& value) {
      m_data[m_size++] = std::move(value);
    }

    constexpr void pop_back() {
      m_size--;
    }

    constexpr void erase(const_iterator it) {
      auto copy_it = (iterator)it;

      while (copy_it != end()) {
        *copy_it = std::move(*(copy_it + 1));
        copy_it++;
      }

      m_size--;
    }

    constexpr auto insert(const_iterator it, T const& value) -> iterator {
      iterator copy_it = end();

      while (copy_it != it) {
        *copy_it = std::move(*(copy_it - 1));
        copy_it--;
      }

      *(iterator)it = value;

      m_size++;

      return (iterator)it;
    }

    constexpr auto insert(const_iterator it, T&& value) -> iterator {
      iterator copy_it = end();

      while (copy_it != it) {
        *copy_it = std::move(*(copy_it - 1));
        copy_it--;
      }

      *(iterator)it = std::move(value);

      m_size++;

      return (iterator)it;
    }

    constexpr auto front() -> T& {
      return m_data[0];
    }

    constexpr auto front() const -> T const& {
      return m_data[0];
    }

    constexpr auto back() -> T& {
      return m_data[m_size - 1];
    }

    constexpr auto back() const -> T const& {
      return m_data[m_size - 1];
    }

    constexpr auto begin() -> iterator {
      return (iterator)&m_data[0];
    }

    constexpr auto cbegin() const -> const_iterator {
      return (const_iterator)&m_data[0];
    }

    constexpr auto end() -> iterator {
      return (iterator)&m_data[m_size];
    }

    constexpr auto cend() const -> const_iterator {
      return (const_iterator)&m_data[m_size];
    }

    constexpr bool empty() const {
      return m_size == 0;
    }

    constexpr auto size() const -> size_t {
      return m_size;
    }

    constexpr auto data() -> T* {
      return m_data;
    }

    constexpr auto data() const -> T const* {
      return m_data;
    }

  private:
    T m_data[capacity];
    size_t m_size = 0;
};

} // namespace lunar
