/*
 * Copyright (C) 2021 fleroviux
 */

#pragma once

#include <string_view>

namespace Duality::core {

struct Core {
  Core(std::string_view rom_path);
 ~Core();

private:
  struct CoreImpl* pimpl = nullptr;
};

} // namespace Duality::core
