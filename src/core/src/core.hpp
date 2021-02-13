/*
 * Copyright (C) 2020 fleroviux
 */

#pragma once

#include <memory>

#include "interconnect.hpp"

namespace Duality {

struct Core {

private:
  std::unique_ptr<Interconnect> interconnect = std::make_unique<Interconnect>();
};

} // namespace Duality
