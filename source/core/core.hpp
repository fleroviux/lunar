/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <memory>

#include "interconnect.hpp"

namespace fauxDS {

struct Core {

private:
  std::unique_ptr<Interconnect> interconnect = std::make_unique<Interconnect>();
};

} // namespace fauxDS
