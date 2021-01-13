/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <algorithm>
#include <array>
#include <common/integer.hpp>
#include <common/likely.hpp>
#include <common/meta.hpp>
#include <common/log.hpp>
#include <stddef.h>
#include <vector>

namespace Duality::core {

template<size_t page_count, u32 page_size = 16384>
struct Region {
  Region(size_t mask) : mask(mask) {}

  template<typename T>
  auto Read(u32 offset) const -> T {
    static_assert(common::is_one_of_v<T, u8, u16, u32, u64>, "T must be u8, u16, u32 or u64"); 

    auto const& desc = pages[(offset >> kPageShift) & mask];
    offset &= kPageMask & ~(sizeof(T) - 1);
    if (likely(desc.page != nullptr)) {
      return *reinterpret_cast<T*>(&desc.page[offset]);
    }
    // TODO: if the first check failed maybe this case is actually likely.
    if (unlikely(desc.pages != nullptr)) {
      T value = 0;
      for (u8* page : *desc.pages) // ???
        value |= *reinterpret_cast<T*>(&page[offset]);
      return value;
    }
    
    return 0;
  }

  template<typename T>
  void Write(u32 offset, T value) {
    static_assert(common::is_one_of_v<T, u8, u16, u32, u64>, "T must be u8, u16, u32 or u64"); 

    auto const& desc = pages[(offset >> kPageShift) & mask];
    offset &= kPageMask & ~(sizeof(T) - 1);
    if (likely(desc.page != nullptr)) {
      *reinterpret_cast<T*>(&desc.page[offset]) = value;
      return;
    }
    if (unlikely(desc.pages != nullptr)) {
      for (u8* page : *desc.pages) // ???
        *reinterpret_cast<T*>(&page[offset]) = value;
    }
  }

  template<typename T>
  auto GetUnsafePointer(u32 offset) const -> T const* {
    auto const& desc = pages[(offset >> kPageShift) & mask];
    offset &= kPageMask & ~(sizeof(T) - 1);
    if (likely(desc.page != nullptr)) {
      return reinterpret_cast<T*>(&desc.page[offset]);
    }
    return nullptr;
  }

  template<size_t bank_size>
  void Map(u32 offset, std::array<u8, bank_size>& bank, size_t size = bank_size) {
    auto id = static_cast<size_t>(offset >> kPageShift);
    auto final_id = id + (size >> kPageShift);
    auto data = bank.data();

    while (id < final_id) {
      auto& desc = pages.at(id++);

      if (unlikely(desc.page != nullptr)) {
        desc.pages = new std::vector<u8*>{};
        desc.pages->push_back(desc.page);
        desc.pages->push_back(data);
        desc.page = nullptr;
      } else if (unlikely(desc.pages != nullptr)) {
        desc.pages->push_back(data);
      } else {
        desc.page = data;
      }

      data += page_size;
    }
  }

  template<size_t bank_size>
  void Unmap(u32 offset, std::array<u8, bank_size> const& bank, size_t size = bank_size) {
    auto id = static_cast<size_t>(offset >> kPageShift);
    auto final_id = id + (size >> kPageShift);
    auto data = bank.data();

    while (id < final_id) {
      auto& desc = pages.at(id++);

      if (likely(desc.page == data)) {
        desc.page = nullptr;
      } else if (unlikely(desc.pages != nullptr)) {
        auto begin = desc.pages->begin();
        auto end = desc.pages->end();
        auto match = std::find(begin, end, data);
        if (match != end) {
          desc.pages->erase(match);
          if (desc.pages->size() == 1) {
            desc.page = (*desc.pages)[0]; // ???
            delete desc.pages;
            desc.pages = nullptr;
          }
        }
      }

      data += page_size;
    }
  }

private:
  /// VRAM page descriptor (default size is 16 KiB)
  struct PageDescriptor {
    /// Pointer to a single physical page (regular case).
    u8* page = nullptr;

    /// List of pointers to multiple physical pages (rare special case).
    std::vector<u8*>* pages = nullptr;
  };

  size_t mask;
  std::array<PageDescriptor, page_count> pages {};

  static constexpr int kPageShift = []() constexpr -> int {
    for (int i = 0; i < 32; i++)
      if (page_size == (1 << i))
        return i;
    return -1;
  }();

  static constexpr int kPageMask = page_size - 1;

  // Make sure that the provided page size actually is a power-of-two.
  static_assert(kPageShift != -1, "Region: page size must be a power-of-two.");
};

} // namespace Duality::core
