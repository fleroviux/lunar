/*
 * Copyright (C) 2020 fleroviux
 */

#include <common/log.hpp>

#include "dma.hpp"

namespace fauxDS::core {

void DMA::Reset() {
  for (auto& channel : channels) {
    channel = {};
  }
}

} // namespace fauxDS::core
